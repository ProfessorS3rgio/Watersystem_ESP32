<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use App\Models\Deduction;

class Customer extends Model
{
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

    public function deductions()
    {
        return $this->belongsToMany(Deduction::class, 'customer_deduction', 'customer_id', 'deduction_id');
    }
}