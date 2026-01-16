<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class Customer extends Model
{
    protected $table = 'customer';

    protected $fillable = [
        'account_no',
        'customer_name',
        'address',
        'previous_reading',
        'is_active',
    ];

    protected $casts = [
        'previous_reading' => 'integer',
        'is_active' => 'boolean',
    ];
}
