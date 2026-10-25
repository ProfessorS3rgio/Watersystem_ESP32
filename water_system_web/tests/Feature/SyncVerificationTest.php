<?php
namespace Tests\Feature;

use App\Models\{Customer, SyncReview, User};
use App\Services\SyncVerificationService;
use Illuminate\Support\Facades\DB;
use Tests\TestCase;

class SyncVerificationTest extends TestCase
{
    private User $operator;
    protected function setUp(): void {
        parent::setUp();
        $this->artisan('migrate', ['--force' => true])->assertExitCode(0);
        $this->seed([\Database\Seeders\BarangaySequenceSeeder::class, \Database\Seeders\CustomerTypeSeeder::class, \Database\Seeders\SettingsSeeder::class]);
        $this->operator = User::factory()->create(['name' => 'Sync reviewer']);
        $this->actingAs($this->operator);
        Customer::create(['account_no' => 'M-001', 'customer_name' => 'Test member', 'address' => 'Makilas', 'brgy_id' => 1, 'type_id' => 1, 'previous_reading' => 0]);
    }
    private function batch(): array {
        return [
            'device' => ['device_uid' => 'MASTER-1', 'brgy_id' => 1, 'firmware_version' => 'test', 'verified_sync_ack' => 1],
            'readings' => [['reading_id' => 7, 'device_uid' => 'MASTER-1', 'customer_account_number' => 'M-001', 'previous_reading' => 0, 'current_reading' => 10, 'usage_m3' => 10, 'reading_at' => now('Asia/Manila')->startOfDay()->timestamp]],
            'bills' => [['bill_id' => 5, 'reading_id' => 7, 'device_uid' => 'MASTER-1', 'customer_account_number' => 'M-001', 'reference_number' => 'MASTER-BILL-5', 'bill_date' => now('Asia/Manila')->toDateString(), 'rate_per_m3' => 10, 'charges' => 100, 'penalty' => 0, 'total_due' => 100, 'status' => 'Paid']],
            'transactions' => [['bill_transaction_id' => 3, 'bill_reference_number' => 'MASTER-BILL-5', 'processed_by_device_uid' => 'MASTER-1', 'type' => 'payment', 'source' => 'Device', 'amount' => 100, 'cash_received' => 120, 'change' => 20, 'transaction_date' => now('Asia/Manila')->startOfDay()->format('Y-m-d H:i:s'), 'payment_method' => 'Cash', 'notes' => 'Received at household']],
        ];
    }
    private function stage(?array $batch = null): array { return $this->postJson('/sync-reviews', $batch ?? $this->batch())->assertCreated()->json('review'); }
    private function approve(string $id, bool $warnings = false) { return $this->postJson('/sync-reviews/'.$id.'/approve', ['device_uid' => 'MASTER-1', 'accept_warnings' => $warnings]); }

    public function test_preview_is_read_only_and_approval_saves_the_complete_batch_once(): void {
        $review = $this->stage();
        $this->assertTrue($review['preview']['can_approve']);
        $this->assertSame(3, $review['preview']['summary']['ready']);
        foreach (['reading', 'bill', 'bill_transaction', 'device'] as $table) $this->assertDatabaseCount($table, 0);
        $receipt = $this->approve($review['id'])->assertOk()->json('receipt');
        $this->assertSame('Sync reviewer', $receipt['approved_by']);
        foreach (['reading', 'bill', 'bill_transaction', 'device'] as $table) $this->assertDatabaseCount($table, 1);
        $this->assertDatabaseHas('bill', ['status' => 'Paid', 'amount_paid' => 100, 'balance_due' => 0]);
        $this->assertDatabaseHas('bill_transaction', ['device_transaction_id' => 3]);
        $this->approve($review['id'])->assertOk()->assertJsonPath('receipt.review_id', $review['id']);
        $this->assertDatabaseCount('bill_transaction', 1);
        $this->assertNull(SyncReview::findOrFail($review['id'])->payload);
        $this->assertDatabaseHas('audit_logs', ['action' => 'sync_batch_approved']);
        $this->assertSame([], DB::select('PRAGMA foreign_key_check'));
        $this->postJson('/sync-reviews/'.$review['id'].'/acknowledged')->assertOk();
        $this->getJson('/sync-reviews')->assertOk()->assertJsonPath('reviews.0.id', $review['id']);
    }
    public function test_cancelling_keeps_business_rows_untouched(): void {
        $run = $this->postJson('/sync-runs')->assertCreated()->json('run_id');
        $review = $this->stage();
        $this->postJson('/sync-reviews/'.$review['id'].'/cancel')->assertOk();
        $this->approve($review['id'])->assertConflict();
        foreach (['reading', 'bill', 'bill_transaction'] as $table) $this->assertDatabaseCount($table, 0);
        $this->assertNull(SyncReview::findOrFail($review['id'])->payload);
        $this->postJson('/sync-runs/'.$run.'/finish', ['status' => 'cancelled'])->assertOk();
        $this->assertDatabaseHas('audit_logs', ['action' => 'sync_cancelled']);
    }
    public function test_missing_accounts_bad_readings_and_bad_totals_block_the_whole_batch(): void {
        $batch = $this->batch();
        $batch['readings'][0]['customer_account_number'] = 'MISSING';
        $batch['readings'][0]['current_reading'] = -1;
        $batch['bills'][0]['total_due'] = 99;
        $review = $this->stage($batch);
        $this->assertFalse($review['preview']['can_approve']);
        $this->assertGreaterThanOrEqual(2, $review['preview']['summary']['blocked']);
        $this->approve($review['id'], true)->assertUnprocessable();
        foreach (['reading', 'bill', 'bill_transaction'] as $table) $this->assertDatabaseCount($table, 0);
    }
    public function test_high_usage_requires_explicit_warning_acceptance(): void {
        $batch = $this->batch(); $batch['readings'][0]['current_reading'] = 100; $batch['readings'][0]['usage_m3'] = 100;
        $review = $this->stage($batch);
        $this->assertGreaterThan(0, $review['preview']['summary']['warnings']);
        $this->approve($review['id'])->assertUnprocessable();
        $this->approve($review['id'], true)->assertOk()->assertJsonPath('receipt.accepted_warnings', true);
    }
    public function test_changed_website_state_requires_a_fresh_review(): void {
        $review = $this->stage();
        DB::table('customer')->where('account_no', 'M-001')->update(['customer_name' => 'Changed while reviewing']);
        $this->approve($review['id'])->assertConflict();
        $this->assertDatabaseCount('reading', 0);
        $fresh = $this->stage(); $this->approve($fresh['id'])->assertOk();
    }
    public function test_already_saved_records_are_recognised_and_conflicts_are_never_overwritten(): void {
        $review = $this->stage(); $this->approve($review['id'])->assertOk();
        $same = $this->stage();
        $this->assertSame(3, $same['preview']['summary']['already_saved']);
        $this->approve($same['id'])->assertOk();
        $this->assertDatabaseCount('bill_transaction', 1);
        $batch = $this->batch(); $batch['readings'][0]['current_reading'] = 11; $batch['readings'][0]['usage_m3'] = 11;
        $conflict = $this->stage($batch); $this->approve($conflict['id'], true)->assertUnprocessable();
        $this->assertDatabaseHas('reading', ['current_reading' => 10]);
    }
    public function test_an_import_failure_rolls_back_every_business_row_and_receipt(): void {
        $review = $this->stage();
        $this->mock(SyncVerificationService::class, function ($mock) {
            $mock->makePartial();
            $mock->shouldReceive('apply')->once()->andReturnUsing(function () {
                DB::table('customer')->where('account_no', 'M-001')->update(['customer_name' => 'Must roll back']);
                throw new \RuntimeException('Simulated database failure');
            });
        });
        $this->approve($review['id'])->assertStatus(500);
        $this->assertDatabaseHas('customer', ['customer_name' => 'Test member']);
        $this->assertDatabaseCount('device', 0);
        $this->assertDatabaseHas('sync_reviews', ['id' => $review['id'], 'status' => 'pending', 'receipt' => null]);
        $this->assertDatabaseMissing('audit_logs', ['action' => 'sync_batch_approved']);
    }
    public function test_review_ownership_expiry_and_device_identity_are_enforced(): void {
        $review = $this->stage();
        $this->actingAs(User::factory()->create()); $this->approve($review['id'])->assertNotFound();
        $this->postJson('/sync-reviews/'.$review['id'].'/cancel')->assertNotFound();
        $this->getJson('/sync-reviews')->assertJsonCount(0, 'reviews');
        $this->actingAs($this->operator);
        $this->postJson('/sync-reviews/'.$review['id'].'/approve', ['device_uid' => 'OTHER', 'accept_warnings' => true])->assertConflict();
        SyncReview::findOrFail($review['id'])->update(['expires_at' => now()->subMinute()]);
        $this->approve($review['id'])->assertConflict();
        $this->assertDatabaseCount('reading', 0);
    }
    public function test_old_import_endpoints_cannot_bypass_review_and_old_firmware_cannot_acknowledge(): void {
        foreach (['/readings/sync', '/bills/sync', '/bill-transactions/sync'] as $url) $this->postJson($url, [])->assertConflict();
        $batch = $this->batch(); unset($batch['device']['verified_sync_ack']);
        $review = $this->stage($batch); $this->assertFalse($review['preview']['can_approve']);
        $this->approve($review['id'], true)->assertUnprocessable();
    }
    public function test_duplicate_device_ids_and_payment_cash_errors_are_blocked(): void {
        $batch = $this->batch(); $batch['readings'][] = $batch['readings'][0]; $batch['transactions'][0]['change'] = 0;
        $review = $this->stage($batch);
        $this->assertGreaterThanOrEqual(3, $review['preview']['summary']['blocked']);
        $this->approve($review['id'], true)->assertUnprocessable();
    }
    public function test_device_payment_can_settle_an_existing_unpaid_bill_without_double_counting(): void {
        $initial = $this->batch(); $initial['bills'][0]['status'] = 'Pending'; $initial['transactions'] = [];
        $review = $this->stage($initial); $this->approve($review['id'])->assertOk();
        $payment = $this->batch(); $payment['readings'] = [];
        $review = $this->stage($payment); $this->approve($review['id'])->assertOk();
        $this->assertDatabaseCount('bill', 1);
        $this->assertDatabaseHas('bill', ['amount_paid' => 100, 'balance_due' => 0, 'status' => 'Paid']);
        $retry = $this->stage($payment); $this->assertSame(2, $retry['preview']['summary']['already_saved']);
        $this->approve($retry['id'])->assertOk();
        $this->assertDatabaseCount('bill_transaction', 1);
    }
    public function test_missing_history_for_an_already_paid_bill_requires_a_warning_and_preserves_its_balance(): void {
        $initial = $this->batch(); $initial['transactions'] = [];
        $review = $this->stage($initial); $this->approve($review['id'], true)->assertOk();
        $history = $this->batch(); $history['readings'] = []; $history['bills'] = [];
        $review = $this->stage($history);
        $this->assertSame(1, $review['preview']['summary']['warnings']);
        $this->approve($review['id'])->assertUnprocessable();
        $this->approve($review['id'], true)->assertOk();
        $this->assertDatabaseHas('bill', ['amount_paid' => 100, 'balance_due' => 0]);
        $this->assertDatabaseCount('bill_transaction', 1);
    }
    public function test_separate_partial_payments_and_payment_only_batches_update_the_balance_once(): void {
        $batch = $this->batch(); $batch['bills'][0]['status'] = 'Partially Paid';
        $batch['transactions'][0]['amount'] = 40; $batch['transactions'][0]['cash_received'] = 50; $batch['transactions'][0]['change'] = 10;
        $review = $this->stage($batch); $this->approve($review['id'])->assertOk();
        $next = $batch; $next['readings'] = []; $next['bills'] = [];
        $next['transactions'][0]['bill_transaction_id'] = 4;
        $next['transactions'][0]['amount'] = 60; $next['transactions'][0]['cash_received'] = 60; $next['transactions'][0]['change'] = 0;
        $review = $this->stage($next); $this->approve($review['id'])->assertOk();
        $this->assertDatabaseHas('bill', ['amount_paid' => 100, 'balance_due' => 0, 'status' => 'Paid']);
        $this->assertDatabaseCount('bill_transaction', 2);
        $review = $this->stage($next); $this->approve($review['id'])->assertOk();
        $this->assertDatabaseHas('bill', ['amount_paid' => 100]);
    }
    public function test_repeated_transitions_billing_months_and_conflicting_payment_identities_block_approval(): void {
        $batch = $this->batch();
        $batch['readings'][] = [...$batch['readings'][0], 'reading_id' => 8];
        $batch['bills'][] = [...$batch['bills'][0], 'bill_id' => 6, 'reference_number' => 'ANOTHER-BILL', 'reading_id' => 8];
        $batch['transactions'][] = [...$batch['transactions'][0], 'bill_transaction_id' => 4];
        $review = $this->stage($batch);
        $this->assertGreaterThanOrEqual(6, $review['preview']['summary']['blocked']);
        $this->approve($review['id'], true)->assertUnprocessable();
    }
    public function test_meter_continuity_and_changed_settings_are_checked_before_saving(): void {
        $review = $this->stage(); $this->approve($review['id'])->assertOk();
        $batch = $this->batch(); $batch['bills'] = []; $batch['transactions'] = [];
        $batch['readings'][0] = [...$batch['readings'][0], 'reading_id' => 8, 'previous_reading' => 11, 'current_reading' => 21, 'reading_at' => $batch['readings'][0]['reading_at'] + 86400];
        $review = $this->stage($batch); $this->approve($review['id'], true)->assertUnprocessable();
        $batch['readings'][0]['previous_reading'] = 10; $batch['readings'][0]['current_reading'] = 20;
        $review = $this->stage($batch);
        DB::table('settings')->update(['bill_due_days' => 19]);
        $this->approve($review['id'])->assertConflict();
        $this->assertDatabaseCount('reading', 1);
    }
    public function test_void_bills_cannot_receive_payments_and_do_not_create_collectable_balances(): void {
        $batch = $this->batch(); $batch['bills'][0]['status'] = 'Void';
        $review = $this->stage($batch); $this->approve($review['id'], true)->assertUnprocessable();
        $batch['transactions'] = [];
        $review = $this->stage($batch); $this->approve($review['id'])->assertOk();
        $this->assertDatabaseHas('bill', ['status' => 'Void', 'balance_due' => 0]);
    }
}
