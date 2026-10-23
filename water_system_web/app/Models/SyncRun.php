<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class SyncRun extends Model
{
    protected $fillable = [
        'run_uuid', 'status', 'device_uid', 'summary', 'error', 'started_at', 'finished_at',
    ];

    protected $casts = [
        'summary' => 'array',
        'started_at' => 'datetime',
        'finished_at' => 'datetime',
    ];
}
