<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class AuditLog extends Model
{
    protected static function booted(): void
    {
        static::updating(fn () => throw new \LogicException('Audit events are append-only.'));
        static::deleting(fn () => throw new \LogicException('Audit events are append-only.'));
    }

    public function actor()
    {
        return $this->belongsTo(User::class, 'actor_id');
    }

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
