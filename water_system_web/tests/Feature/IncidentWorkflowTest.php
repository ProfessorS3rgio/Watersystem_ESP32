<?php

namespace Tests\Feature;

use App\Models\Customer;
use App\Models\IncidentEvent;
use App\Models\ServiceIncident;
use App\Models\User;
use App\Services\AuditLogService;
use Illuminate\Support\Facades\DB;
use Illuminate\Support\Str;
use Tests\TestCase;

class IncidentWorkflowTest extends TestCase
{
    private Customer $customer;

    protected function setUp(): void
    {
        parent::setUp();
        $this->artisan('migrate', ['--force' => true])->assertExitCode(0);
        $this->seed([\Database\Seeders\BarangaySequenceSeeder::class, \Database\Seeders\CustomerTypeSeeder::class, \Database\Seeders\SettingsSeeder::class]);
        $this->actingAs(User::factory()->create(['name' => 'DMBC operator']));
        $this->customer = Customer::create(['account_no' => 'M-001', 'customer_name' => 'Test household', 'address' => 'Makilas', 'type_id' => 1, 'brgy_id' => 1, 'previous_reading' => 0, 'status' => 'active']);
    }

    private function payload(array $extra = []): array
    {
        return [...['client_token' => (string) Str::uuid(), 'customer_id' => $this->customer->customer_id, 'title' => 'Leaking service connection', 'description' => 'Water is leaking at the meter connection.', 'location' => 'Beside the household gate', 'category' => 'leak', 'priority' => 'high', 'pipe_section' => 'meter', 'assigned_to' => null], ...$extra];
    }

    private function report(array $extra = []): array { return $this->postJson('/incidents', $this->payload($extra))->assertCreated()->json('data'); }

    public function test_creation_is_idempotent_and_keeps_customer_and_actor_snapshots(): void
    {
        $payload = $this->payload();
        $first = $this->postJson('/incidents', $payload)->assertCreated()->json('data');
        $this->postJson('/incidents', $payload)->assertOk()->assertJsonPath('data.id', $first['id']);
        $this->assertDatabaseCount('service_incidents', 1);
        $this->assertDatabaseCount('incident_events', 1);
        $this->assertDatabaseHas('audit_logs', ['action' => 'incident_reported']);
        $this->assertStringStartsWith('INC-', $first['reference_number']);
        $this->assertArrayNotHasKey('client_token', $first);
        $this->assertArrayNotHasKey('request_hash', $first);
        $this->customer->update(['customer_name' => 'Changed name', 'account_no' => 'M-099']);
        $this->getJson('/incidents/'.$first['id'])->assertOk()->assertJsonPath('data.customer_name', 'Test household')->assertJsonPath('data.account_no', 'M-001')->assertJsonPath('data.events.0.actor_name', 'DMBC operator');
        $this->postJson('/incidents', [...$payload, 'title' => 'Different request'])->assertConflict();
    }

    public function test_repair_workflow_requires_a_resolution_and_records_reopening(): void
    {
        $incident = $this->report(); $url = '/incidents/'.$incident['id'];
        $this->patchJson($url, ['version' => 1, 'status' => 'closed'])->assertUnprocessable();
        $this->patchJson($url, ['version' => 1, 'status' => 'in_progress', 'assigned_to' => 'Repair team A', 'note' => 'Inspection started.'])->assertOk()->assertJsonPath('data.version', 2);
        $this->patchJson($url, ['version' => 2, 'status' => 'resolved'])->assertUnprocessable()->assertJsonValidationErrors('resolution');
        $this->patchJson($url, ['version' => 2, 'status' => 'resolved', 'resolution' => 'Replaced the coupling and checked for leaks.'])->assertOk()->assertJsonPath('data.status', 'resolved')->assertJsonPath('data.version', 3);
        $this->patchJson($url, ['version' => 3, 'status' => 'closed'])->assertOk()->assertJsonPath('data.version', 4);
        $this->patchJson($url, ['version' => 4, 'title' => 'Rewrite closed report'])->assertUnprocessable();
        $this->patchJson($url, ['version' => 4, 'status' => 'in_progress'])->assertUnprocessable()->assertJsonValidationErrors('note');
        $result = $this->patchJson($url, ['version' => 4, 'status' => 'in_progress', 'note' => 'Leak returned after the repair.'])->assertOk()->assertJsonPath('data.reopened_count', 1)->assertJsonPath('data.resolution', null)->assertJsonPath('data.resolved_at', null)->assertJsonPath('data.closed_at', null)->json('data');
        $this->assertSame('reopened', $result['events'][0]['kind']);
        $this->assertDatabaseHas('audit_logs', ['action' => 'incident_reopened']);
        $this->assertTrue(collect($result['events'])->contains(fn ($event) => str_contains($event['message'] ?? '', 'Replaced the coupling')));
    }

    public function test_stale_updates_are_rejected_and_notes_are_append_only(): void
    {
        $incident = $this->report(); $url = '/incidents/'.$incident['id'];
        $this->patchJson($url, ['version' => 1, 'priority' => 'urgent'])->assertOk()->assertJsonPath('data.version', 2);
        $this->patchJson($url, ['version' => 1, 'assigned_to' => 'Old window edit'])->assertConflict();
        $this->assertDatabaseHas('service_incidents', ['id' => $incident['id'], 'priority' => 'urgent', 'assigned_to' => null, 'version' => 2]);
        $this->assertDatabaseCount('incident_events', 2);
        $this->patchJson($url, ['version' => 2, 'note' => 'Waiting for a replacement fitting.'])->assertOk()->assertJsonPath('data.events.0.kind', 'note_added');
        $event = IncidentEvent::latest('id')->firstOrFail();
        try { $event->update(['message' => 'Rewritten']); $this->fail('Expected append-only history'); } catch (\LogicException $error) { $this->assertStringContainsString('append-only', $error->getMessage()); }
        try { $event->delete(); $this->fail('Expected append-only history'); } catch (\LogicException $error) { $this->assertStringContainsString('append-only', $error->getMessage()); }
    }

    public function test_audit_failure_rolls_back_incident_and_timeline_writes(): void
    {
        $this->mock(AuditLogService::class)->shouldReceive('record')->once()->andThrow(new \RuntimeException('Audit unavailable'));
        $this->postJson('/incidents', $this->payload())->assertServerError();
        $this->assertDatabaseCount('service_incidents', 0);
        $this->assertDatabaseCount('incident_events', 0);
    }

    public function test_update_audit_failure_keeps_the_previous_version_and_status(): void
    {
        $incident = $this->report();
        $this->mock(AuditLogService::class)->shouldReceive('record')->once()->andThrow(new \RuntimeException('Audit unavailable'));
        $this->patchJson('/incidents/'.$incident['id'], ['version' => 1, 'status' => 'in_progress'])->assertServerError();
        $this->assertDatabaseHas('service_incidents', ['id' => $incident['id'], 'status' => 'reported', 'version' => 1]);
        $this->assertDatabaseCount('incident_events', 1);
    }

    public function test_search_filters_and_existing_report_warnings_use_the_correct_account(): void
    {
        $normal = $this->report(['priority' => 'normal']);
        $urgent = $this->report(['priority' => 'urgent']);
        $this->getJson('/incidents?q=M-001')->assertOk()->assertJsonPath('total', 2)->assertJsonPath('data.0.id', $urgent['id'])->assertJsonPath('summary.urgent', 1);
        $this->getJson('/incidents?q=household&priority=urgent&category=leak')->assertOk()->assertJsonPath('total', 1);
        $this->getJson('/incidents?q=does-not-exist')->assertOk()->assertJsonPath('total', 0);
        $this->getJson('/incidents/customers?q=M-001')->assertOk()->assertJsonPath('data.0.open_incidents_count', 2);
        $this->getJson('/incidents/'.$normal['id'])->assertOk()->assertJsonPath('data.other_open_incidents.0.id', $urgent['id']);
        $this->patchJson('/incidents/'.$urgent['id'], ['version' => 1, 'status' => 'cancelled', 'note' => 'Duplicate report for the same leaking joint.'])->assertOk();
        $this->getJson('/incidents')->assertOk()->assertJsonPath('total', 1);
        $this->getJson('/incidents?status=all')->assertOk()->assertJsonPath('total', 2);
        $this->getJson('/incidents/customers?id='.$this->customer->customer_id)->assertOk()->assertJsonPath('data.0.open_incidents_count', 1);
    }

    public function test_deleting_an_unbilled_customer_keeps_the_incident_context(): void
    {
        $incident = $this->report();
        $this->deleteJson('/customers/'.$this->customer->customer_id)->assertOk();
        $this->getJson('/incidents/'.$incident['id'])->assertOk()->assertJsonPath('data.customer_id', null)->assertJsonPath('data.customer', null)->assertJsonPath('data.customer_name', 'Test household')->assertJsonPath('data.events_count', 1);
        $this->assertSame([], DB::select('PRAGMA foreign_key_check'));
    }

    public function test_restore_keeps_newer_incidents_and_relinks_only_the_same_account(): void
    {
        $snapshot = $this->postJson('/backup/restore-points', ['label' => 'Before the report'])->assertCreated()->json('id');
        $incident = $this->report();
        $this->patchJson('/incidents/'.$incident['id'], ['version' => 1, 'note' => 'Keep this new repair note.'])->assertOk();
        $this->postJson('/backup/restore-points/'.$snapshot.'/restore')->assertOk();
        $this->getJson('/incidents/'.$incident['id'])->assertOk()->assertJsonPath('data.customer_id', $this->customer->customer_id)->assertJsonPath('data.events_count', 2);
        $this->assertSame([], DB::select('PRAGMA foreign_key_check'));
    }

    public function test_restore_unlinks_a_missing_account_without_erasing_its_report(): void
    {
        $snapshot = $this->postJson('/backup/restore-points', ['label' => 'Before a new account'])->assertCreated()->json('id');
        $new = Customer::create(['account_no' => 'M-002', 'customer_name' => 'New household', 'address' => 'Makilas', 'type_id' => 1, 'brgy_id' => 1, 'previous_reading' => 0, 'status' => 'active']);
        $incident = $this->report(['customer_id' => $new->customer_id]);
        $this->postJson('/backup/restore-points/'.$snapshot.'/restore')->assertOk();
        $this->getJson('/incidents/'.$incident['id'])->assertOk()->assertJsonPath('data.customer_id', null)->assertJsonPath('data.customer_name', 'New household')->assertJsonPath('data.version', 2)->assertJsonPath('data.events.0.kind', 'account_unlinked');
        $this->assertSame([], DB::select('PRAGMA foreign_key_check'));
    }

    public function test_history_pages_stay_stable_when_new_notes_arrive(): void
    {
        $incident = $this->report();
        for ($i = 0; $i < 35; $i++) IncidentEvent::create(['incident_id' => $incident['id'], 'actor_id' => auth()->id(), 'actor_name' => 'Operator', 'kind' => 'note_added', 'message' => 'Test note '.$i, 'created_at' => now()]);
        $detail = $this->getJson('/incidents/'.$incident['id'])->assertOk()->assertJsonCount(30, 'data.events')->json('data');
        IncidentEvent::create(['incident_id' => $incident['id'], 'actor_name' => 'Operator', 'kind' => 'note_added', 'message' => 'New event after the first page', 'created_at' => now()]);
        $older = $this->getJson('/incidents/'.$incident['id'].'/history?before_id='.$detail['more_history_before'])->assertOk()->assertJsonCount(6, 'data')->json('data');
        $all = [...array_column($detail['events'], 'id'), ...array_column($older, 'id')];
        $this->assertCount(36, array_unique($all));
    }

    public function test_incident_repairs_do_not_change_bills_readings_or_service_status(): void
    {
        DB::table('device')->insert(['brgy_id' => 1, 'device_uid' => 'TEST', 'device_mac' => '00:11:22:33:44:55', 'firmware_version' => 'test']);
        $reading = DB::table('reading')->insertGetId(['customer_id' => $this->customer->customer_id, 'customer_account_number' => 'M-001', 'device_uid' => 'TEST', 'previous_reading' => 0, 'current_reading' => 10, 'usage_m3' => 10, 'reading_at' => now()]);
        DB::table('bill')->insert(['reference_number' => 'TEST-BILL', 'customer_id' => $this->customer->customer_id, 'customer_account_number' => 'M-001', 'reading_id' => $reading, 'device_uid' => 'TEST', 'bill_date' => now()->toDateString(), 'rate_per_m3' => 10, 'charges' => 100, 'penalty' => 0, 'total_due' => 100, 'amount_paid' => 40, 'balance_due' => 60, 'status' => 'Partially Paid']);
        $before = (array) DB::table('bill')->first();
        $incident = $this->report();
        $this->patchJson('/incidents/'.$incident['id'], ['version' => 1, 'status' => 'resolved', 'resolution' => 'Repaired the pipe and checked flow.'])->assertOk();
        $this->assertSame($before, (array) DB::table('bill')->first());
        $this->assertDatabaseHas('reading', ['reading_id' => $reading, 'current_reading' => 10]);
        $this->assertDatabaseHas('customer', ['customer_id' => $this->customer->customer_id, 'status' => 'active']);
        $this->assertDatabaseCount('bill_transaction', 0);
    }

    public function test_invalid_and_unauthenticated_requests_do_not_create_reports(): void
    {
        $this->postJson('/incidents', $this->payload(['customer_id' => 999999, 'priority' => 'unrecognized']))->assertUnprocessable();
        $this->assertDatabaseCount('service_incidents', 0);
        auth()->forgetGuards();
        $this->getJson('/incidents')->assertUnauthorized();
        $this->getJson('/incidents/customers?q=M-001')->assertUnauthorized();
        $this->postJson('/incidents', $this->payload())->assertUnauthorized();
    }
}
