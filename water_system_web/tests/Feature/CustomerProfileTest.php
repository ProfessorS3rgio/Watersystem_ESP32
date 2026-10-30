<?php
namespace Tests\Feature;
use App\Models\{Customer, CustomerProfile, User};
use Illuminate\Support\Facades\DB;
use Tests\TestCase;

class CustomerProfileTest extends TestCase {
    private Customer $member;
    protected function setUp(): void {
        parent::setUp();
        $this->artisan('migrate', ['--force' => true])->assertExitCode(0);
        $this->seed([\Database\Seeders\BarangaySequenceSeeder::class, \Database\Seeders\CustomerTypeSeeder::class, \Database\Seeders\SettingsSeeder::class]);
        $this->actingAs(User::factory()->create());
        $this->member = Customer::create(['account_no'=>'M-001','customer_name'=>'Original full name as recorded','type_id'=>1,'brgy_id'=>1,'address'=>'Makilas','previous_reading'=>17,'status'=>'active','Synced'=>true,'last_sync'=>'2026-01-01 12:00:00']);
    }
    private function url(?Customer $customer = null): string { return '/customers/'.($customer ?? $this->member)->customer_id.'/profile'; }
    private function payload(array $extra = [], ?Customer $customer = null): array {
        $data = $this->getJson($this->url($customer))->assertOk()->json('data');
        return [...['revision'=>$data['revision'],'full_name'=>$data['customer']['full_name'],'address'=>$data['customer']['address']],...$extra];
    }
    public function test_existing_names_are_preserved_and_viewing_a_profile_does_not_create_data(): void {
        $this->getJson($this->url())->assertOk()->assertJsonPath('data.customer.full_name','Original full name as recorded')->assertJsonPath('data.profile',null);
        $this->assertDatabaseCount('customer_profiles',0);
        $this->assertDatabaseHas('customer',['customer_name'=>'Original full name as recorded','Synced'=>true]);
    }
    public function test_name_parts_build_the_billing_name_and_mark_only_changed_account_details_unsynced(): void {
        $result = $this->putJson($this->url(),$this->payload(['first_name'=>'  María  ','middle_name'=>'de la Paz','last_name'=>"D'Angelo",'name_suffix'=>'Jr.','birthdate'=>'1990-08-12','meter_number'=>'wm-001','phone'=>'+63 917 555 0101','email'=>'member@example.test','meter_installed_on'=>'2020-01-01','service_started_on'=>'2013-07-14']))->assertOk();
        $result->assertJsonPath('data.customer.full_name',"María de la Paz D'Angelo Jr.")->assertJsonPath('data.profile.meter_number','WM-001')->assertJsonPath('data.profile.birthdate','1990-08-12');
        $this->assertDatabaseHas('customer',['account_no'=>'M-001','customer_name'=>"María de la Paz D'Angelo Jr.",'Synced'=>false,'last_sync'=>'2026-01-01 12:00:00']);
        $this->assertDatabaseHas('audit_logs',['action'=>'customer_profile_updated']);
        // The returned revision must immediately work for a second edit.
        $this->putJson($this->url(),['revision'=>$result->json('data.revision'),'full_name'=>$result->json('data.customer.full_name'),'address'=>'Makilas','phone'=>'09175550102'])->assertOk();
    }
    public function test_personal_details_do_not_change_readings_balances_or_device_sync_flags(): void {
        $this->putJson($this->url(),$this->payload(['birthdate'=>'1995-05-06','meter_number'=>'AB-123','notes'=>'Meter beside the gate.']))->assertOk();
        $this->assertDatabaseHas('customer',['Synced'=>true,'previous_reading'=>17,'customer_name'=>'Original full name as recorded']);
        foreach (['reading','bill','bill_transaction'] as $table) $this->assertDatabaseCount($table,0);
    }
    public function test_incomplete_names_future_dates_and_invalid_contact_details_are_rejected(): void {
        $this->putJson($this->url(),$this->payload(['first_name'=>'Jose','birthdate'=>now('Asia/Manila')->addDay()->toDateString(),'phone'=>'not a phone','email'=>'invalid']))->assertUnprocessable()->assertJsonValidationErrors(['last_name','birthdate','phone','email']);
        $this->assertDatabaseCount('customer_profiles',0);
        $this->putJson($this->url(),$this->payload(['birthdate'=>'2025-02-30']))->assertUnprocessable()->assertJsonValidationErrors('birthdate');
    }
    public function test_meter_numbers_are_unique_after_normalisation_and_blank_numbers_can_be_shared(): void {
        $other = Customer::create(['account_no'=>'M-002','customer_name'=>'Second member','type_id'=>1,'brgy_id'=>1,'address'=>'Makilas','previous_reading'=>0,'status'=>'active']);
        $this->putJson($this->url(),$this->payload(['meter_number'=>' wm-123 ']))->assertOk();
        $this->putJson($this->url($other),$this->payload(['meter_number'=>'wm-123'],$other))->assertUnprocessable()->assertJsonValidationErrors('meter_number');
        $this->putJson($this->url(),$this->payload(['meter_number'=>'']))->assertOk();
        $this->putJson($this->url($other),$this->payload(['meter_number'=>''],$other))->assertOk();
        $this->assertDatabaseCount('customer_profiles',2);
    }
    public function test_stale_profiles_cannot_overwrite_another_edit(): void {
        $payload = $this->payload(['phone'=>'09170000001']);
        $this->putJson($this->url(),$payload)->assertOk();
        $this->putJson($this->url(),[...$payload,'phone'=>'09170000002'])->assertConflict();
        $this->assertDatabaseHas('customer_profiles',['phone'=>'09170000001']);
    }
    public function test_account_changes_also_invalidate_an_open_profile(): void {
        $payload = $this->payload(['notes'=>'Stale note']);
        $this->member->update(['address'=>'New service address']);
        $this->putJson($this->url(),$payload)->assertConflict();
        $this->assertDatabaseCount('customer_profiles',0);
    }
    public function test_search_finds_meter_numbers_without_exposing_personal_details_in_bulk_customer_data(): void {
        $this->putJson($this->url(),$this->payload(['birthdate'=>'1990-01-01','phone'=>'09170000000','meter_number'=>'WM-876']))->assertOk();
        $response = $this->getJson('/customers?search=WM-876')->assertOk()->assertJsonCount(1,'data');
        $response->assertJsonPath('data.0.meter_number','WM-876');
        foreach (['birthdate','phone','email','notes','first_name'] as $key) $this->assertArrayNotHasKey($key,$response->json('data.0'));
    }
    public function test_account_number_cannot_be_changed_through_the_profile(): void {
        $this->putJson($this->url(),$this->payload(['account_no'=>'REASSIGNED','customer_id'=>999,'meter_number'=>'WM-456']))->assertOk()->assertJsonPath('data.customer.account_no','M-001');
        $this->assertDatabaseHas('customer_profiles',['customer_id'=>$this->member->customer_id]);
    }
    public function test_generic_account_edits_cannot_desynchronise_structured_names(): void {
        $this->putJson($this->url(),$this->payload(['first_name'=>'Jose','last_name'=>'Dela Cruz']))->assertOk();
        $this->putJson('/customers/'.$this->member->customer_id,['account_no'=>'M-001','customer_name'=>'Unrelated name','type_id'=>1,'brgy_id'=>1,'address'=>'Makilas','status'=>'active'])->assertUnprocessable()->assertJsonValidationErrors('customer_name');
        $this->assertDatabaseHas('customer',['customer_name'=>'Jose Dela Cruz']);
    }
    public function test_profile_stays_with_the_member_when_account_numbers_are_swapped(): void {
        $other = Customer::create(['account_no'=>'M-002','customer_name'=>'Second member','type_id'=>1,'brgy_id'=>1,'address'=>'Makilas','previous_reading'=>0,'status'=>'active']);
        $this->putJson($this->url(),$this->payload(['meter_number'=>'WM-789','birthdate'=>'1980-01-02']))->assertOk();
        $this->postJson('/customers/swap-account-no',['source_customer_id'=>$this->member->customer_id,'target_customer_id'=>$other->customer_id])->assertOk();
        $this->getJson($this->url())->assertJsonPath('data.customer.account_no','M-002')->assertJsonPath('data.profile.meter_number','WM-789');
        $this->getJson($this->url($other))->assertJsonPath('data.profile',null);
    }
    public function test_backups_restore_profile_data_and_legacy_snapshots_preserve_matching_profiles(): void {
        $legacyId = $this->postJson('/backup/restore-points',['label'=>'Before profiles','source'=>'manual'])->assertCreated()->json('id');
        $legacy = DB::table('backup_snapshots')->where('id',$legacyId)->first();
        $oldPayload=json_decode($legacy->payload,true); unset($oldPayload['tables']['customer_profiles']);
        DB::table('backup_snapshots')->where('id',$legacyId)->update(['payload'=>json_encode($oldPayload)]);
        $this->putJson($this->url(),$this->payload(['first_name'=>'Jose','last_name'=>'Dela Cruz','meter_number'=>'WM-123']))->assertOk();
        $currentId = $this->postJson('/backup/restore-points',['label'=>'With profiles','source'=>'manual'])->assertCreated()->json('id');
        $this->putJson($this->url(),$this->payload(['meter_number'=>'CHANGED']))->assertOk();
        $this->postJson('/backup/restore-points/'.$currentId.'/restore')->assertOk();
        $this->assertDatabaseHas('customer_profiles',['meter_number'=>'WM-123']);
        $this->postJson('/backup/restore-points/'.$legacyId.'/restore')->assertOk();
        $this->assertDatabaseHas('customer_profiles',['meter_number'=>'WM-123']);
        $this->assertDatabaseHas('customer',['customer_name'=>'Jose Dela Cruz']);
        $this->assertSame([],DB::select('PRAGMA foreign_key_check'));
    }
    public function test_deleting_an_unbilled_member_does_not_leave_an_orphan_profile(): void {
        $this->putJson($this->url(),$this->payload(['meter_number'=>'WM-123']))->assertOk();
        $this->deleteJson('/customers/'.$this->member->customer_id)->assertOk();
        $this->assertDatabaseCount('customer_profiles',0);
    }
    public function test_profiles_require_sign_in_and_unknown_members_return_not_found(): void {
        $this->getJson('/customers/999999/profile')->assertNotFound();
        $this->app['auth']->forgetGuards();
        $this->getJson($this->url())->assertUnauthorized();
    }
}
