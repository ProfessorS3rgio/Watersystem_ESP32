<?php

namespace Tests\Feature;

use App\Models\User;
use Illuminate\Support\Facades\DB;
use Tests\TestCase;
use ZipArchive;

class DesktopSqliteTest extends TestCase
{
    protected function setUp(): void
    {
        parent::setUp();
        $this->artisan('migrate', ['--force' => true])->assertExitCode(0);
        foreach ([\Database\Seeders\BarangaySequenceSeeder::class, \Database\Seeders\CustomerTypeSeeder::class, \Database\Seeders\SettingsSeeder::class] as $seeder) $this->seed($seeder);
        $this->actingAs(User::factory()->create());
    }

    private function bill(): int
    {
        $barangay = DB::table('barangay_sequence')->value('brgy_id');
        $type = DB::table('customer_type')->value('type_id');
        DB::table('device')->insert(['brgy_id' => $barangay, 'device_uid' => 'SQLITE-TEST', 'device_mac' => '00:11:22:33:44:55', 'firmware_version' => 'test']);
        $customer = DB::table('customer')->insertGetId(['account_no' => 'SQLITE-001', 'type_id' => $type, 'customer_name' => 'SQLite member', 'brgy_id' => $barangay, 'address' => 'Makilas', 'previous_reading' => 0]);
        $reading = DB::table('reading')->insertGetId(['customer_id' => $customer, 'customer_account_number' => 'SQLITE-001', 'device_uid' => 'SQLITE-TEST', 'previous_reading' => 0, 'current_reading' => 10, 'usage_m3' => 10, 'reading_at' => now()]);
        return DB::table('bill')->insertGetId(['reference_number' => 'SQLITE-BILL-001', 'customer_id' => $customer, 'customer_account_number' => 'SQLITE-001', 'reading_id' => $reading, 'device_uid' => 'SQLITE-TEST', 'bill_date' => now()->toDateString(), 'due_date' => now()->addMonth()->toDateString(), 'rate_per_m3' => 10, 'charges' => 100, 'penalty' => 0, 'total_due' => 100, 'balance_due' => 100]);
    }

    public function test_real_schema_accepts_partial_and_full_payments_with_foreign_keys_enabled(): void
    {
        $bill = $this->bill();
        $this->postJson('/bills/'.$bill.'/pay', ['amount_paid' => 40, 'cash_received' => 40, 'change_amount' => 0, 'payment_method' => 'cash'])->assertOk()->assertJsonPath('data.status', 'Partially Paid');
        $this->assertDatabaseHas('bill', ['bill_id' => $bill, 'balance_due' => 60]);
        $this->postJson('/bills/'.$bill.'/pay', ['amount_paid' => 60, 'cash_received' => 60, 'change_amount' => 0, 'payment_method' => 'cash'])->assertOk()->assertJsonPath('data.status', 'Paid');
        $this->assertDatabaseCount('bill_transaction', 2);
        $this->assertSame([], DB::select('PRAGMA foreign_key_check'));
    }

    public function test_sqlite_restore_preserves_linked_billing_rows_and_current_audit_history(): void
    {
        $this->bill();
        $snapshot = $this->postJson('/backup/restore-points', ['label' => 'SQLite test', 'source' => 'manual'])->assertCreated()->json('id');
        DB::table('customer')->update(['customer_name' => 'Changed']);
        $this->postJson('/backup/restore-points/'.$snapshot.'/restore')->assertOk();
        $this->assertDatabaseHas('customer', ['customer_name' => 'SQLite member']);
        $this->assertDatabaseCount('bill', 1);
        $this->assertDatabaseHas('audit_logs', ['action' => 'restore_point_created']);
        $this->assertDatabaseHas('audit_logs', ['action' => 'restore_point_restored']);
        $this->assertSame([], DB::select('PRAGMA foreign_key_check'));
    }

    public function test_sqlite_backup_contains_a_readable_consistent_database(): void
    {
        $this->bill();
        $response = $this->get('/backup/export')->assertOk();
        $archive = $response->baseResponse->getFile()->getPathname();
        $zip = new ZipArchive();
        $this->assertTrue($zip->open($archive));
        $copy = tempnam(sys_get_temp_dir(), 'dmbc-test-backup-');
        try {
            $this->assertNotFalse($zip->locateName('watersystem-data.json'));
            file_put_contents($copy, $zip->getFromName('watersystem-database.sqlite'));
            $database = new \PDO('sqlite:'.$copy);
            $this->assertSame('ok', $database->query('PRAGMA integrity_check')->fetchColumn());
            $this->assertSame('SQLITE-BILL-001', $database->query('SELECT reference_number FROM bill')->fetchColumn());
            $database = null;
        } finally {
            $zip->close();
            unlink($copy);
            unlink($archive);
        }
    }
}
