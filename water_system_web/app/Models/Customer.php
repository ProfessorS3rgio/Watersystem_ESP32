<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use App\Models\Deduction;

class Customer extends Model
{
    protected static function booted(): void {
        static::saving(function (Customer $customer) {
            if ($customer->exists && $customer->isDirty('customer_name')) {
                $profile = $customer->profile()->first();
                if ($profile?->hasStructuredName() && $customer->customer_name !== $profile->fullName()) {
                    throw \Illuminate\Validation\ValidationException::withMessages(['customer_name' => 'Update this member’s name through Member profile so the name fields stay consistent.']);
                }
            }
        });
    }
    public function profile() { return $this->hasOne(CustomerProfile::class, 'customer_id', 'customer_id'); }
    protected $table = 'customer';
    protected $primaryKey = 'customer_id';

    protected $fillable = [
        'account_no',
        'customer_name',
        'type_id',
        'deduction_id',
        'brgy_id',
        'address',
        'previous_reading',
        'status',
        'Synced',
        'last_sync',
    ];

    protected $casts = [
        'previous_reading' => 'integer',
        'type_id' => 'integer',
        'deduction_id' => 'integer',
        'brgy_id' => 'integer',
        'Synced' => 'boolean',
        'last_sync' => 'datetime',
    ];

    public function getRouteKeyName()
    {
        return 'customer_id';
    }

    public function deductions()
    {
        return $this->belongsToMany(Deduction::class, 'customer_deduction', 'customer_id', 'deduction_id');
    }

    public function incidents()
    {
        return $this->hasMany(ServiceIncident::class, 'customer_id', 'customer_id');
    }
}
