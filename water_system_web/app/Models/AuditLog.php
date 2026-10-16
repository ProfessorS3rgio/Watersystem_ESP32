<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class AuditLog extends Model
{
    protected $fillable = [
        'actor_type', 'actor_id', 'action', 'auditable_type', 'auditable_id',
        'before', 'after', 'metadata', 'ip_address', 'device_uid',
    ];

    protected $casts = [
        'before' => 'array',
        'after' => 'array',
        'metadata' => 'array',
    ];
}
