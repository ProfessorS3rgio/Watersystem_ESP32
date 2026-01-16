<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;

class BillTransaction extends Model
{
    protected $fillable = [
        'bill_id',
        'type',
        'amount',
        'cash_received',
        'change',
        'transaction_date',
        'payment_method',
        'processed_by_user_id',
        'notes'
    ];

    protected $casts = [
        'transaction_date' => 'datetime',
        'amount' => 'decimal:2',
        'cash_received' => 'decimal:2',
        'change' => 'decimal:2'
    ];

    public function bill(): BelongsTo
    {
        return $this->belongsTo(Bill::class);
    }

    public function processedBy(): BelongsTo
    {
        return $this->belongsTo(User::class, 'processed_by_user_id');
    }
}
