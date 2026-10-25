<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class IncidentEvent extends Model
{
    public $timestamps = false;
    protected $guarded = ['id'];
    protected $casts = ['changes' => 'array', 'created_at' => 'datetime'];

    protected static function booted(): void
    {
        static::updating(fn () => throw new \LogicException('Incident history is append-only.'));
        static::deleting(fn () => throw new \LogicException('Incident history is append-only.'));
    }
}
