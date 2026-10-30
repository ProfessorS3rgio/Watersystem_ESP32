<?php

namespace Tests\Feature;

use App\Models\{Bill, Customer, CustomerProfile, User};
use Illuminate\Http\UploadedFile;
use Illuminate\Support\Facades\DB;
use PhpOffice\PhpSpreadsheet\{IOFactory, Spreadsheet};
use Tests\TestCase;

class CollectionWorkbookImportTest extends TestCase
{
    private Customer $member;
    private Bill $bill;
    private array $temporaryFiles = [];

    protected function setUp(): void
    {
        parent::setUp();
        $this->artisan('migrate', ['--force' => true])->assertExitCode(0);
        $this->seed([\Database\Seeders\BarangaySequenceSeeder::class, \Database\Seeders\CustomerTypeSeeder::class]);
        $this->actingAs(User::factory()->create());
        $brgy = DB::table('barangay_sequence')->where('prefix', 'M')->value('brgy_id');
        $this->member = Customer::create(['account_no' => 'M-001', 'customer_name' => 'Test Member', 'type_id' => 1,
            'brgy_id' => $brgy, 'address' => 'Makilas', 'previous_reading' => 17, 'status' => 'active']);
        DB::table('device')->insert(['brgy_id' => $brgy, 'device_uid' => 'COLLECTION-TEST', 'device_mac' => '00:11:22:33:44:55', 'firmware_version' => 'test']);
        $reading = DB::table('reading')->insertGetId(['customer_id' => $this->member->customer_id, 'customer_account_number' => 'M-001',
            'device_uid' => 'COLLECTION-TEST', 'previous_reading' => 17, 'current_reading' => 27, 'usage_m3' => 10, 'reading_at' => '2026-09-30 12:00:00']);
        $this->bill = Bill::create(['reference_number' => 'COLLECTION-BILL', 'customer_id' => $this->member->customer_id,
            'customer_account_number' => 'M-001', 'reading_id' => $reading, 'device_uid' => 'COLLECTION-TEST',
            'bill_date' => '2026-09-30', 'due_date' => '2026-10-05', 'rate_per_m3' => 15, 'charges' => 150,
            'penalty' => 50, 'total_due' => 200, 'amount_paid' => 0, 'balance_due' => 200, 'status' => 'Pending']);
    }

    protected function tearDown(): void
    {
        foreach ($this->temporaryFiles as $file) @unlink($file);
        parent::tearDown();
    }

    private function workbook(array $rows = [], bool $legacy = false): UploadedFile
    {
        $book = new Spreadsheet;
        $sheet = $book->getActiveSheet();
        if ($legacy) {
            $sheet->setCellValue('A2', 'Barangay: Makilas  •  Billing Month: September 1-30, 2026');
            $sheet->fromArray(['Account Number', 'Customer Name', 'Previous Reading', 'Present Reading', 'Status'], null, 'A4');
            $sheet->fromArray(['M-001', 'Test Member', 17, 27, 'Not Paid'], null, 'A5');
        } else {
            $sheet->setCellValue('L2', 'MAKILAS');
            $sheet->setCellValue('G4', "Billing Month: Sep't. 1-30, 2026");
            $sheet->setCellValue('J4', 'Oct. 5, 2026');
            $sheet->setCellValue('H4', 'Collection Date:');
            $sheet->setCellValue('K4', 'To be collect by the month of:');
            $sheet->fromArray(['cubic un-paid', 'PENALTY', 'Minimum', 'Minimum', 'Cubic Used', 'No.', 'NAME', 'PENALTY LAST MONTH', 'MONTHLY COLLECTION', null, 'METER No.', 'COLLECTABLE FAILURE TO PAY', 'PENALTY'], null, 'A6');
            foreach (($rows ?: [[1, 'Test Member', 150, 0, '‘00123']]) as $i => $row) {
                [$seq, $name, $minimum, $collection, $meter] = $row;
                $sheet->fromArray([0, 999, 1, 0, 0, $seq, $name, 888, $minimum, $collection, $meter, 0, 777], null, 'A'.($i + 7));
            }
        }
        $path = tempnam(sys_get_temp_dir(), 'collection-test-');
        $this->temporaryFiles[] = $path;
        IOFactory::createWriter($book, 'Xlsx')->save($path);
        return new UploadedFile($path, '9. September 2026.xlsx', null, null, true);
    }

    private function preview(array $rows = [])
    {
        $response = $this->post('/customers/readings-import/preview', ['file' => $this->workbook($rows)], ['Accept' => 'application/json'])->assertOk();
        return $response;
    }

    private function confirm(string $token)
    {
        return $this->postJson('/customers/readings-import/confirm', ['token' => $token, 'selected_accounts' => ['M-001']]);
    }

    public function test_collections_combine_both_columns_preserve_penalties_and_are_idempotent(): void
    {
        $rows = [[1, 'Test Member', 150, 50, '‘00123']];
        $preview = $this->preview($rows)->assertJsonPath('billing_month', '2026-09')->assertJsonPath('rows.0.file_payment_amount', 200);
        $this->assertDatabaseCount('bill_transaction', 0);
        $this->assertDatabaseCount('customer_profiles', 0);
        $this->confirm($preview->json('token'))->assertOk()->assertJsonPath('payments_imported', 1)->assertJsonPath('meters_updated', 1);
        $this->assertDatabaseHas('bill', ['bill_id' => $this->bill->bill_id, 'amount_paid' => 200, 'balance_due' => 0, 'penalty' => 50, 'charges' => 150, 'status' => 'Paid']);
        $this->assertDatabaseHas('bill_transaction', ['amount' => 200, 'type' => 'payment', 'transaction_date' => '2026-10-05 12:00:00']);
        $this->assertDatabaseHas('customer_profiles', ['customer_id' => $this->member->customer_id, 'meter_number' => '00123']);
        $this->preview($rows)->assertJsonPath('summary.unchanged', 1)->assertJsonPath('token', null);
        $this->assertDatabaseCount('reading', 1);
        $this->assertDatabaseCount('bill_transaction', 1);
    }

    public function test_partial_collection_records_only_the_increase_and_zero_does_not_clear_payments(): void
    {
        $this->bill->update(['amount_paid' => 50, 'balance_due' => 150, 'status' => 'Partially Paid']);
        $preview = $this->preview();
        $this->confirm($preview->json('token'))->assertOk();
        $this->assertDatabaseHas('bill_transaction', ['amount' => 100]);
        $this->assertDatabaseHas('bill', ['amount_paid' => 150, 'balance_due' => 50, 'status' => 'Partially Paid']);
        $this->preview([[1, 'Test Member', 0, null, '00123']])->assertJsonPath('summary.unchanged', 1);
        $this->assertDatabaseHas('bill', ['amount_paid' => 150]);
    }

    public function test_invalid_amounts_and_conflicting_existing_meters_still_block_import(): void
    {
        $this->preview([[1, 'Test Member', -150, 0, '123']])->assertJsonPath('summary.errors', 1);
        CustomerProfile::create(['customer_id' => $this->member->customer_id, 'meter_number' => 'OTHER']);
        $this->preview()->assertJsonPath('summary.errors', 1);
    }

    public function test_missing_month_bill_does_not_create_readings_or_bills(): void
    {
        $this->bill->update(['bill_date' => '2026-08-31']);
        $this->preview()->assertJsonPath('summary.errors', 1)->assertJsonPath('token', null);
        $this->assertDatabaseCount('bill', 1);
        $this->assertDatabaseCount('reading', 1);
        $preview = $this->preview([[1, 'Test Member', 0, 0, '123']]);
        $this->confirm($preview->json('token'))->assertOk()->assertJsonPath('meters_updated', 1);
    }

    public function test_changed_bill_or_account_rejects_confirmation_atomically(): void
    {
        $preview = $this->preview();
        $this->bill->update(['amount_paid' => 25]);
        $this->confirm($preview->json('token'))->assertUnprocessable();
        $this->assertDatabaseCount('customer_profiles', 0);
        $this->assertDatabaseCount('bill_transaction', 0);
        $this->assertDatabaseCount('backup_snapshots', 0);
        $preview = $this->preview();
        $this->member->update(['account_no' => 'M-009']);
        $this->confirm($preview->json('token'))->assertUnprocessable();
    }

    public function test_existing_reading_template_still_uses_original_importer(): void
    {
        $this->post('/customers/readings-import/preview', ['file' => $this->workbook(legacy: true)], ['Accept' => 'application/json'])
            ->assertOk()->assertJsonPath('rows.0.file_previous', 17)->assertJsonPath('rows.0.file_present', 27)
            ->assertJsonPath('rows.0.file_payment_status', 'Pending')->assertJsonMissingPath('format');
    }

    public function test_multiple_collection_sheets_require_selection_and_skip_summary_rows(): void
    {
        $upload = $this->workbook();
        $book = IOFactory::load($upload->getRealPath());
        $sheet = $book->getActiveSheet();
        $sheet->setTitle('First');
        $sheet->setCellValue('G4', 'Billing Month: September 1-30, 2026');
        $sheet->setCellValue('F10', 813);
        $sheet->setCellValue('G10', 6840);
        $copy = clone $sheet;
        $copy->setTitle('Second');
        $book->addSheet($copy);
        IOFactory::createWriter($book, 'Xlsx')->save($upload->getRealPath());
        $this->post('/customers/readings-import/preview', ['file' => $upload], ['Accept' => 'application/json'])
            ->assertOk()->assertJsonPath('requires_sheet_selection', true)->assertJsonCount(2, 'sheets')->assertJsonPath('sheets.0.rows', 1);
        $this->post('/customers/readings-import/preview', ['file' => $upload, 'sheet_name' => 'Second'], ['Accept' => 'application/json'])
            ->assertOk()->assertJsonPath('sheet_name', 'Second')->assertJsonPath('billing_month', '2026-09')->assertJsonPath('summary.changes', 1);
    }

    public function test_names_ignore_case_spacing_and_missing_middle_initials(): void
    {
        foreach ([
            ['Marilyn B. Cataytay', 'MARILYN CATAYTAY'],
            ['Marilyn Cataytay', '  marilyn   B.  cataytay  '],
            ['Michael Flores', 'MICHAEL FLORES'],
            ['María B. Dela Cruz', 'MARIA DELA CRUZ'],
            ['Mary Ann B. Flores', 'MARY ANN FLORES'],
            ['Marilou V. Magdadaro', 'MARILOU V.MAGDADARO'],
            ['Marilou Magdadaro', 'MARILOU V.MAGDADARO'],
            ['Anaclita Layson', 'ANACLITA LAYSON CLOSE'],
            ['Anaclita Layson', 'ANACLITA LAYSON CLOSED'],
            ['Marilyn B. C. Cataytay', 'MARILYN CATAYTAY'],
        ] as [$database, $file]) {
            $this->member->update(['customer_name' => $database]);
            $this->preview([[1, $file, 150, 0, '02313']])
                ->assertJsonPath('summary.errors', 0)->assertJsonPath('rows.0.status', 'change');
        }
        $preview = $this->preview([[1, 'MARILYN CATAYTAY', 150, 0, '02313']]);
        $this->confirm($preview->json('token'))->assertOk();
        $this->assertDatabaseHas('customer', ['customer_id' => $this->member->customer_id, 'customer_name' => 'Marilyn B. C. Cataytay']);
        $this->assertDatabaseHas('customer_profiles', ['customer_id' => $this->member->customer_id, 'meter_number' => '02313']);
    }

    public function test_different_names_and_conflicting_initials_warn_without_changing_database_names(): void
    {
        foreach ([
            ['Michael Flores', 'JOHN FLORES'],
            ['Michael Flores', 'MICHAEL CRUZ'],
            ['Marilyn B. Cataytay', 'MARILYN C. CATAYTAY'],
            ['Mary Ann Flores', 'MARY JANE FLORES'],
            ['Mary Ann Flores', 'MARY FLORES'],
            ['Marilyn Cataytay Jr.', 'MARILYN CATAYTAY SR.'],
            ['Michael Flores', 'M FLORES'],
            ['Michael Flores', 'JOHN FLORES CLOSE'],
            ['Rolly T. Pogoy', 'JOHN T. PUGOY'],
            ['Rolly T. Pogoy', 'ROLLY C. PUGOY'],
            ['Rolly T. Pogoy', 'ROLLY T. GARCIA'],
        ] as [$database, $file]) {
            $this->member->update(['customer_name' => $database]);
            $response = $this->preview([[1, $file, 150, 0, '02313']])
                ->assertJsonPath('summary.errors', 0)->assertJsonPath('summary.warnings', 1)->assertJsonPath('rows.0.status', 'warning');
            $this->assertStringStartsWith('Name does not match account', $response->json('rows.0.message'));
        }
        $this->assertDatabaseCount('bill_transaction', 0);
        $this->assertDatabaseCount('customer_profiles', 0);
        $this->confirm($response->json('token'))->assertOk()->assertJsonPath('payments_imported', 1);
        $this->assertDatabaseHas('customer', ['customer_id' => $this->member->customer_id, 'customer_name' => 'Rolly T. Pogoy']);
    }

    public function test_duplicate_meters_in_file_warn_and_skip_meter_but_process_collection(): void
    {
        $preview = $this->preview([[1, 'Wrong person', 150, 0, '123'], [2, 'Other Member', 0, 0, '123']])
            ->assertJsonPath('summary.warnings', 1)->assertJsonPath('summary.errors', 1)
            ->assertJsonPath('rows.0.meter_update_skipped', true);
        $this->assertStringContainsString('Name does not match', $preview->json('rows.0.message'));
        $this->assertStringContainsString('Meter update will be skipped', $preview->json('rows.0.message'));
        $this->confirm($preview->json('token'))->assertOk()->assertJsonPath('payments_imported', 1)->assertJsonPath('meters_updated', 0);
        $this->assertDatabaseHas('customer', ['customer_name' => 'Test Member']);
        $this->assertDatabaseCount('customer_profiles', 0);
        $this->assertDatabaseHas('bill', ['bill_id' => $this->bill->bill_id, 'amount_paid' => 150]);
        $this->preview([[1, 'Wrong person', 150, 0, '123'], [2, 'Other Member', 0, 0, '123']])
            ->assertJsonPath('token', null)->assertJsonPath('rows.0.status', 'unchanged');
    }

    public function test_meter_owned_by_another_account_is_preserved_while_collection_is_processed(): void
    {
        $other = Customer::create(['account_no' => 'M-002', 'customer_name' => 'Other Member', 'type_id' => 1,
            'brgy_id' => $this->member->brgy_id, 'address' => 'Makilas', 'previous_reading' => 0, 'status' => 'active']);
        CustomerProfile::create(['customer_id' => $other->customer_id, 'meter_number' => '123']);
        CustomerProfile::create(['customer_id' => $this->member->customer_id, 'meter_number' => 'ORIGINAL']);
        $preview = $this->preview([[1, 'Test Member', 150, 0, '123']])->assertJsonPath('summary.warnings', 1)
            ->assertJsonPath('rows.0.meter_update_skipped', true);
        $this->confirm($preview->json('token'))->assertOk()->assertJsonPath('payments_imported', 1)->assertJsonPath('meters_updated', 0);
        $this->assertDatabaseHas('customer_profiles', ['customer_id' => $other->customer_id, 'meter_number' => '123']);
        $this->assertDatabaseHas('customer_profiles', ['customer_id' => $this->member->customer_id, 'meter_number' => 'ORIGINAL']);
    }

    public function test_both_accounts_with_same_workbook_meter_can_receive_their_collections(): void
    {
        $other = Customer::create(['account_no' => 'M-002', 'customer_name' => 'Other Member', 'type_id' => 1,
            'brgy_id' => $this->member->brgy_id, 'address' => 'Makilas', 'previous_reading' => 0, 'status' => 'active']);
        $reading = \App\Models\Reading::findOrFail($this->bill->reading_id)->replicate();
        $reading->customer_id = $other->customer_id;
        $reading->customer_account_number = 'M-002';
        $reading->save();
        $bill = $this->bill->replicate();
        $bill->reference_number = 'COLLECTION-BILL-2';
        $bill->customer_id = $other->customer_id;
        $bill->customer_account_number = 'M-002';
        $bill->reading_id = $reading->reading_id;
        $bill->save();
        $preview = $this->preview([[1, 'Test Member', 150, 0, '123'], [2, 'Other Member', 100, 0, '123']])
            ->assertJsonPath('summary.errors', 0)->assertJsonPath('summary.warnings', 2);
        $this->postJson('/customers/readings-import/confirm', ['token' => $preview->json('token'), 'selected_accounts' => ['M-001', 'M-002']])
            ->assertOk()->assertJsonPath('payments_imported', 2)->assertJsonPath('meters_updated', 0);
        $this->assertDatabaseCount('customer_profiles', 0);
        $this->assertDatabaseCount('bill_transaction', 2);
        $this->assertDatabaseHas('bill', ['bill_id' => $this->bill->bill_id, 'amount_paid' => 150]);
        $this->assertDatabaseHas('bill', ['bill_id' => $bill->bill_id, 'amount_paid' => 100]);
    }

    public function test_minor_surname_spelling_difference_is_an_importable_review_warning(): void
    {
        $this->member->update(['customer_name' => 'Rolly T. Pogoy']);
        $rows = [[1, 'ROLLY T. PUGOY', 150, 0, '2614']];
        $preview = $this->preview($rows)->assertJsonPath('summary.errors', 0)
            ->assertJsonPath('summary.warnings', 1)->assertJsonPath('rows.0.status', 'warning');
        $this->assertStringContainsString('Check surname spelling', $preview->json('rows.0.message'));
        $this->confirm($preview->json('token'))->assertOk()->assertJsonPath('payments_imported', 1);
        $this->assertDatabaseHas('customer', ['customer_name' => 'Rolly T. Pogoy']);
        $this->assertDatabaseHas('customer_profiles', ['customer_id' => $this->member->customer_id, 'meter_number' => '2614']);
        $this->preview($rows)->assertJsonPath('summary.unchanged', 1)->assertJsonPath('token', null);
    }

    public function test_close_note_can_import_a_meter_without_creating_a_payment(): void
    {
        $this->member->update(['customer_name' => 'Anaclita Layson']);
        $preview = $this->preview([[1, 'ANACLITA LAYSON CLOSE', 0, 0, '220620880']])
            ->assertJsonPath('rows.0.status', 'change');
        $this->confirm($preview->json('token'))->assertOk()->assertJsonPath('payments_imported', 0);
        $this->assertDatabaseCount('bill_transaction', 0);
        $this->assertDatabaseHas('customer', ['customer_name' => 'Anaclita Layson', 'status' => 'active']);
    }

    public function test_collection_import_saves_a_restorable_point_before_any_changes(): void
    {
        $preview = $this->preview();
        $this->assertDatabaseCount('backup_snapshots', 0);
        $response = $this->confirm($preview->json('token'))->assertOk();
        $id = $response->json('restore_point_id');
        $this->assertNotNull($id);
        $snapshot = DB::table('backup_snapshots')->where('id', $id)->first();
        $this->assertSame('excel_import', $snapshot->source);
        $this->assertStringContainsString('September 2026.xlsx', $snapshot->label);
        $data = json_decode($snapshot->payload, true);
        $this->assertEquals(0, $data['tables']['bill'][0]['amount_paid']);
        $this->assertSame([], $data['tables']['customer_profiles']);
        $this->assertSame([], $data['tables']['bill_transaction']);
        $this->assertDatabaseCount('backup_snapshots', 1);
        $this->getJson('/backup/restore-points')->assertOk()->assertJsonPath('data.0.id', $id);
        $this->postJson('/backup/restore-points/'.$id.'/restore')->assertOk();
        $this->assertDatabaseHas('bill', ['bill_id' => $this->bill->bill_id, 'amount_paid' => 0, 'balance_due' => 200]);
        $this->assertDatabaseCount('customer_profiles', 0);
        $this->assertDatabaseCount('bill_transaction', 0);
    }

    public function test_legacy_reading_import_also_saves_pre_import_restore_point(): void
    {
        $upload = $this->workbook(legacy: true);
        $book = IOFactory::load($upload->getRealPath());
        $book->getActiveSheet()->setCellValue('D5', 28);
        IOFactory::createWriter($book, 'Xlsx')->save($upload->getRealPath());
        DB::table('bill')->where('bill_id', $this->bill->bill_id)->update(['bill_date' => '2026-09-30']);
        $preview = $this->post('/customers/readings-import/preview', ['file' => $upload], ['Accept' => 'application/json'])->assertOk();
        $response = $this->confirm($preview->json('token'))->assertOk();
        $id = $response->json('restore_point_id');
        $this->assertNotNull($id);
        $snapshot = json_decode(DB::table('backup_snapshots')->where('id', $id)->value('payload'), true);
        $this->assertEquals(27, $snapshot['tables']['reading'][0]['current_reading']);
        $this->assertDatabaseHas('reading', ['reading_id' => $this->bill->reading_id, 'current_reading' => 28]);
        $this->postJson('/backup/restore-points/'.$id.'/restore')->assertOk();
        $this->assertDatabaseHas('reading', ['reading_id' => $this->bill->reading_id, 'current_reading' => 27]);
    }

    public function test_restore_point_failure_stops_import_before_any_mutation(): void
    {
        $preview = $this->preview();
        $this->mock(\App\Services\RestorePointService::class, function ($mock) {
            $mock->shouldReceive('create')->once()->andThrow(new \RuntimeException('Backup storage unavailable'));
        });
        $this->withoutExceptionHandling();
        try {
            $this->confirm($preview->json('token'));
            $this->fail('Import must stop if its restore point cannot be saved.');
        } catch (\RuntimeException $error) {
            $this->assertSame('Backup storage unavailable', $error->getMessage());
        }
        $this->assertDatabaseCount('backup_snapshots', 0);
        $this->assertDatabaseCount('customer_profiles', 0);
        $this->assertDatabaseCount('bill_transaction', 0);
        $this->assertDatabaseHas('bill', ['bill_id' => $this->bill->bill_id, 'amount_paid' => 0]);
    }
}
