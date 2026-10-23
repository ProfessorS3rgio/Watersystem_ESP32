<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class Reading extends Model
{
    protected $table = 'reading';
    protected $primaryKey = 'reading_id';
    // public $incrementing = false;

    protected $fillable = [
        'customer_id',
        'device_uid',
        'device_reading_id',
        'previous_reading',
        'current_reading',
        'usage_m3',
        'reading_at',
        'read_by_user_id',
        'customer_account_number',
        'Synced',
        'last_sync',
    ];

    protected $casts = [
        'customer_id' => 'integer',
        'device_uid' => 'string',
        'device_reading_id' => 'integer',
        'previous_reading' => 'integer',
        'current_reading' => 'integer',
        'usage_m3' => 'integer',
        'reading_at' => 'datetime',
        'read_by_user_id' => 'integer',
        'Synced' => 'boolean',
        'last_sync' => 'datetime',
    ];
}
