<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class Reading extends Model
{
    protected $table = 'reading';
    protected $primaryKey = 'reading_id';
    public $incrementing = false;

    protected $fillable = [
        'customer_id',
        'device_uid',
        'previous_reading',
        'current_reading',
        'usage_m3',
        'reading_at',
        'read_by_user_id',
        'customer_account_number',
    ];

    protected $casts = [
        'customer_id' => 'integer',
        'device_uid' => 'string',
        'previous_reading' => 'integer',
        'current_reading' => 'integer',
        'usage_m3' => 'integer',
        'reading_at' => 'datetime',
        'read_by_user_id' => 'integer',
    ];
}
