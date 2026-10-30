<?php
namespace App\Http\Controllers;

use App\Models\{Customer, CustomerProfile};
use App\Services\AuditLogService;
use Illuminate\Database\QueryException;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\DB;
use Illuminate\Validation\{Rule, ValidationException};

class CustomerProfileController extends Controller
{
    private function revision(Customer $customer, ?CustomerProfile $profile): string {
        return hash('sha256', json_encode([$customer->getAttributes(), $profile?->getAttributes()], JSON_THROW_ON_ERROR));
    }
    private function present(Customer $customer): array {
        $customer->refresh();
        $profile = $customer->profile()->first();
        return [
            'customer' => ['id' => $customer->customer_id, 'account_no' => $customer->account_no, 'full_name' => $customer->customer_name,
                'address' => $customer->address, 'status' => $customer->status,
                'barangay' => DB::table('barangay_sequence')->where('brgy_id', $customer->brgy_id)->value('barangay'),
                'customer_type' => DB::table('customer_type')->where('type_id', $customer->type_id)->value('type_name'),
                'created_at' => $customer->created_at?->toIso8601String()],
            'profile' => $profile ? $profile->only(['first_name', 'middle_name', 'last_name', 'name_suffix', 'birthdate', 'phone', 'email', 'meter_number', 'meter_model', 'meter_location', 'meter_installed_on', 'service_started_on', 'notes', 'updated_at']) : null,
            'revision' => $this->revision($customer, $profile),
        ];
    }
    public function show(Customer $customer) { return response()->json(['data' => $this->present($customer)]); }
    public function update(Request $request, Customer $customer, AuditLogService $audit) {
        $input = $request->all();
        foreach (['full_name', 'address', 'first_name', 'middle_name', 'last_name', 'name_suffix', 'phone', 'email', 'meter_number', 'meter_model', 'meter_location', 'notes'] as $field) {
            if (isset($input[$field]) && is_string($input[$field])) $input[$field] = trim($input[$field]);
            if (($input[$field] ?? null) === '') $input[$field] = null;
        }
        if (isset($input['meter_number'])) $input['meter_number'] = mb_strtoupper($input['meter_number']);
        $today = now('Asia/Manila')->toDateString();
        $data = validator($input, [
            'revision' => ['required', 'string', 'size:64'], 'full_name' => ['required', 'string', 'max:255'], 'address' => ['required', 'string', 'max:255'],
            'first_name' => ['nullable', 'required_with:middle_name,last_name,name_suffix', 'string', 'max:80'],
            'middle_name' => ['nullable', 'string', 'max:80'],
            'last_name' => ['nullable', 'required_with:first_name,middle_name,name_suffix', 'string', 'max:80'],
            'name_suffix' => ['nullable', 'string', 'max:20'],
            'birthdate' => ['nullable', 'date_format:Y-m-d', 'after_or_equal:1900-01-01', 'before_or_equal:'.$today],
            'phone' => ['nullable', 'string', 'max:40', 'regex:/^[0-9+().\s-]+$/'], 'email' => ['nullable', 'email:rfc', 'max:254'],
            'meter_number' => ['nullable', 'string', 'max:80', 'not_regex:/[\x00-\x1F\x7F]/', Rule::unique('customer_profiles', 'meter_number')->ignore($customer->customer_id, 'customer_id')],
            'meter_model' => ['nullable', 'string', 'max:80'], 'meter_location' => ['nullable', 'string', 'max:255'],
            'meter_installed_on' => ['nullable', 'date_format:Y-m-d', 'before_or_equal:'.$today],
            'service_started_on' => ['nullable', 'date_format:Y-m-d', 'before_or_equal:'.$today], 'notes' => ['nullable', 'string', 'max:2000'],
        ])->validate();
        try {
            return DB::transaction(function () use ($customer, $data, $audit) {
                $customer = Customer::whereKey($customer->customer_id)->lockForUpdate()->firstOrFail();
                $profile = $customer->profile()->lockForUpdate()->first();
                abort_unless(hash_equals($this->revision($customer, $profile), $data['revision']), 409, 'This member was changed in another window. Reload the profile before saving.');
                $before = ['full_name' => $customer->customer_name, 'address' => $customer->address, 'profile' => $profile?->getAttributes()];
                $profile ??= new CustomerProfile(['customer_id' => $customer->customer_id]);
                $profile->fill(collect($data)->except(['revision', 'full_name', 'address'])->all());
                $name = $profile->hasStructuredName() ? $profile->fullName() : $data['full_name'];
                if (mb_strlen($name) > 255) throw ValidationException::withMessages(['full_name' => 'The combined full name must be 255 characters or fewer.']);
                $changed = !$profile->exists || $profile->isDirty() || $customer->customer_name !== $name || $customer->address !== $data['address'];
                if ($changed) {
                    $profile->save();
                    if ($customer->customer_name !== $name || $customer->address !== $data['address']) {
                        $customer->customer_name = $name; $customer->address = $data['address']; $customer->Synced = false; $customer->save();
                    }
                    $audit->record('customer_profile_updated', $customer, $before, ['full_name' => $customer->customer_name, 'address' => $customer->address, 'profile' => $profile->getAttributes()], ['account_no' => $customer->account_no]);
                }
                return response()->json(['data' => $this->present($customer)]);
            });
        } catch (QueryException $error) {
            if (in_array($error->getCode(), ['23000', '23505'], true) && str_contains(strtolower($error->getMessage()), 'meter_number')) {
                throw ValidationException::withMessages(['meter_number' => 'This meter number is already assigned to another member.']);
            }
            throw $error;
        }
    }
}
