<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class Setting extends Model
{
    protected $table = 'settings';

    protected $fillable = [
        'rate_per_m3',
        'bill_due_days',
        'penalty_after_days',
        'penalty_amount',
    ];

    protected $casts = [
        'rate_per_m3' => 'decimal:2',
        'bill_due_days' => 'integer',
        'penalty_after_days' => 'integer',
        'penalty_amount' => 'decimal:2',
    ];
}
