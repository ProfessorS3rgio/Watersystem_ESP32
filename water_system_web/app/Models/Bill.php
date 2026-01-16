<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class Bill extends Model
{
    protected $table = 'bill';

    protected $fillable = [
        'customer_id',
        'reading_id',
        'bill_no',
        'bill_date',
        'rate_per_m3',
        'charges',
        'penalty',
        'total_due',
        'due_date',
        'status',
    ];

    protected $casts = [
        'bill_date' => 'date',
        'due_date' => 'date',
        'rate_per_m3' => 'decimal:2',
        'charges' => 'decimal:2',
        'penalty' => 'decimal:2',
        'total_due' => 'decimal:2',
    ];
}
