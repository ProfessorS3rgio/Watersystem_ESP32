<?php

namespace Tests\Feature;

use App\Models\AuditLog;
use App\Models\Customer;
use App\Models\User;
use App\Services\AuditLogService;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\DB;
use Illuminate\Support\Facades\Schema;
use Illuminate\Support\Str;
use Tests\TestCase;

class AuditHistoryTest extends TestCase
{
    protected function setUp(): void
    {
        parent::setUp();
        (require database_path('migrations/2026_10_16_000003_create_audit_logs_table.php'))->up();
        Schema::create('users', function (Blueprint $table) { $table->id(); $table->string('name'); });
        $this->actingAs(User::factory()->make(['id' => 7, 'name' => 'DMBC operator']));
    }

    private function event(array $attributes = []): AuditLog
    {
        return AuditLog::create([...['actor_type' => 'user', 'actor_id' => 7, 'action' => 'customer_updated', 'auditable_type' => Customer::class, 'auditable_id' => 1], ...$attributes]);
    }

    public function test_capture_snapshots_actor_identity_and_redacts_nested_secrets(): void
    {
        $service = app(AuditLogService::class);
        $log = $service->record('test_event', 'Example', ['password' => 'old'], ['password' => 'new'], ['connection' => ['token' => 'secret', 'device_wifi_token' => 'pair code'], 'device_uid' => 'device-1']);
        $this->assertSame('[REDACTED]', $log->before['password']);
        $this->assertSame('[REDACTED]', $log->metadata['connection']['token']);
        $this->assertSame('[REDACTED]', $log->metadata['connection']['device_wifi_token']);
        $this->assertSame('DMBC operator', $log->metadata['actor_name']);
        $this->assertTrue(Str::isUuid($log->metadata['request_id']));
        $second = $service->record('second_event', 'Example');
        $this->assertSame($log->metadata['request_id'], $second->metadata['request_id']);
    }

    public function test_new_events_do_not_shift_an_existing_pagination_snapshot(): void
    {
        for ($i = 0; $i < 31; $i++) $this->event();
        $first = $this->getJson('/audit-logs?per_page=15')->assertOk()->json();
        $this->event();
        $second = $this->getJson('/audit-logs?per_page=15&page=2&snapshot_id='.$first['snapshot_id'])->assertOk()->json();
        $third = $this->getJson('/audit-logs?per_page=15&page=3&snapshot_id='.$first['snapshot_id'])->assertOk()->json();
        $ids = [...array_column($first['data'], 'id'), ...array_column($second['data'], 'id'), ...array_column($third['data'], 'id')];
        $this->assertSame(range(31, 1), $ids);
        $this->assertSame(31, $second['total']);
        $this->getJson('/audit-logs')->assertJsonPath('total', 32);
    }

    public function test_filters_are_combined_and_date_ranges_use_philippine_days(): void
    {
        $run = (string) Str::uuid();
        $wanted = $this->event(['created_at' => '2026-10-23 16:30:00', 'metadata' => ['sync_run_id' => $run, 'actor_name' => 'Operator'], 'after' => ['account_no' => 'M-002']]);
        $this->event(['action' => 'bill_voided', 'created_at' => '2026-10-23 16:30:00', 'after' => ['account_no' => 'M-002']]);
        $this->event(['created_at' => '2026-10-23 15:59:00', 'after' => ['account_no' => 'M-002']]);
        $this->getJson('/audit-logs?action=customer_updated&q=M-002&date_from=2026-10-24&date_to=2026-10-24&sync_run_id='.$run)
            ->assertOk()->assertJsonPath('total', 1)->assertJsonPath('data.0.id', $wanted->id);
        $this->getJson('/audit-logs?date_to=2026-10-24')->assertOk();
        $this->getJson('/audit-logs?date_from=2026-10-24&date_to=2026-10-23')->assertUnprocessable();
    }

    public function test_details_redact_legacy_secrets_and_list_payloads_omit_large_snapshots(): void
    {
        $log = $this->event(['before' => ['password' => 'legacy secret'], 'metadata' => ['actor_name' => 'Original operator', 'imported_rows' => [['account_no' => 'M-001']], 'token' => 'legacy token']]);
        $list = $this->getJson('/audit-logs')->assertOk()->json('data.0');
        $this->assertArrayNotHasKey('before', $list);
        $this->assertArrayNotHasKey('imported_rows', $list['metadata']);
        $this->assertSame(1, $list['imported_row_count']);
        $this->getJson('/audit-logs/'.$log->id)->assertOk()->assertJsonPath('data.before.password', '[REDACTED]')
            ->assertJsonPath('data.metadata.token', '[REDACTED]')->assertJsonPath('data.actor_name', 'Original operator');
    }

    public function test_audit_models_reject_changes_and_deletion(): void
    {
        $log = $this->event();
        try { $log->update(['action' => 'rewritten']); $this->fail('Expected immutable event'); } catch (\LogicException $error) { $this->assertStringContainsString('append-only', $error->getMessage()); }
        try { $log->delete(); $this->fail('Expected immutable event'); } catch (\LogicException $error) { $this->assertStringContainsString('append-only', $error->getMessage()); }
        $this->assertDatabaseHas('audit_logs', ['id' => $log->id, 'action' => 'customer_updated']);
    }

    public function test_restore_preserves_live_audit_history_instead_of_replacing_it(): void
    {
        Schema::create('backup_snapshots', function (Blueprint $table) { $table->id(); $table->string('label'); $table->integer('row_count'); $table->text('payload'); });
        $existing = $this->event();
        $customer = $this->customer();
        $id = DB::table('backup_snapshots')->insertGetId(['label' => 'Test', 'row_count' => 1, 'payload' => json_encode(['format' => 'watersystem-backup', 'tables' => ['customer' => [['customer_id' => $customer->customer_id, 'account_no' => 'RESTORED-001', 'customer_name' => 'Restored member']], 'audit_logs' => [['id' => 999, 'action' => 'old_snapshot_event']]]])]);
        $this->postJson('/backup/restore-points/'.$id.'/restore')->assertOk();
        $this->assertDatabaseHas('audit_logs', ['id' => $existing->id]);
        $this->assertDatabaseMissing('audit_logs', ['id' => 999]);
        $this->assertDatabaseHas('audit_logs', ['action' => 'restore_point_restored']);
        $this->assertDatabaseHas('customer', ['account_no' => 'RESTORED-001']);
    }

    private function customer(): Customer
    {
        Schema::create('customer', function (Blueprint $table) { $table->id('customer_id'); $table->string('account_no'); $table->string('customer_name'); $table->timestamps(); });
        return Customer::create(['account_no' => 'M-001', 'customer_name' => 'Test member']);
    }

    public function test_customer_deletion_records_the_pre_delete_snapshot(): void
    {
        $customer = $this->customer();
        $this->deleteJson('/customers/'.$customer->customer_id)->assertOk();
        $log = AuditLog::where('action', 'customer_deleted')->firstOrFail();
        $this->assertSame('M-001', $log->before['account_no']);
        $this->assertNull($log->after);
    }

    public function test_customer_deletion_rolls_back_if_audit_capture_fails(): void
    {
        $customer = $this->customer();
        $this->mock(AuditLogService::class)->shouldReceive('record')->once()->andThrow(new \RuntimeException('Audit unavailable'));
        $this->deleteJson('/customers/'.$customer->customer_id)->assertServerError();
        $this->assertDatabaseHas('customer', ['customer_id' => $customer->customer_id]);
    }

    public function test_audit_history_requires_authentication(): void
    {
        auth()->forgetGuards();
        $this->getJson('/audit-logs')->assertUnauthorized();
    }
}
