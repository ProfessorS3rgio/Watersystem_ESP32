<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class Customer extends Model
{
    protected $table = 'customer';

    protected $fillable = [
        'account_no',
        'customer_name',
        'type_id',
        'brgy_id',
        'address',
        'previous_reading',
        'status',
    ];

    protected $casts = [
        'previous_reading' => 'integer',
        'type_id' => 'integer',
        'brgy_id' => 'integer',
    ];
}
