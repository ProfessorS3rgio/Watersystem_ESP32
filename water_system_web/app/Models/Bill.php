<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class Bill extends Model
{
    protected $table = 'bill';
    protected $primaryKey = 'bill_id';

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
        'void_reason',
    ];

    protected $casts = [
        'bill_date' => 'date',
        'due_date' => 'date',
        'rate_per_m3' => 'decimal:2',
        'charges' => 'decimal:2',
        'penalty' => 'decimal:2',
        'total_due' => 'decimal:2',
    ];

    public function customer()
    {
        return $this->belongsTo(Customer::class);
    }

    public function reading()
    {
        return $this->belongsTo(Reading::class);
    }

    public function transactions()
    {
        return $this->hasMany(BillTransaction::class);
    }
}
