<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class Device extends Model
{
    protected $table = 'device';
    // No primary key, using device_mac as unique identifier

    protected $fillable = [
        'brgy_id',
        'device_mac',
        'device_uid',
        'firmware_version',
        'device_name',
        'print_count',
        'customer_count',
        'last_sync',
    ];

    protected $casts = [
        'brgy_id' => 'integer',
        'print_count' => 'integer',
        'customer_count' => 'integer',
        'last_sync' => 'datetime',
    ];
}