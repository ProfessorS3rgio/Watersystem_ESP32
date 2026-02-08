<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;

class BillTransaction extends Model
{
    protected $table = 'bill_transaction';
    protected $primaryKey = 'bill_transaction_id';

    protected $fillable = [
        'bill_reference_number',
        'device_uid',
        'type',
        'source',
        'amount',
        'cash_received',
        'change',
        'transaction_date',
        'payment_method',
        'processed_by_user_id',
        'processed_by_device_uid',
        'notes',
        'Synced',
        'last_sync'
    ];

    protected $casts = [
        'transaction_date' => 'datetime',
        'amount' => 'decimal:2',
        'cash_received' => 'decimal:2',
        'change' => 'decimal:2'
    ];

    public function bill(): BelongsTo
    {
        return $this->belongsTo(Bill::class, 'bill_reference_number', 'reference_number');
    }

    public function processedBy(): BelongsTo
    {
        return $this->belongsTo(User::class, 'processed_by_user_id');
    }
}
