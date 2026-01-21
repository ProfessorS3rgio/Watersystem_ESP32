<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Factories\HasFactory;
use Illuminate\Database\Eloquent\Model;

class CustomerType extends Model
{
    use HasFactory;

    protected $table = 'customer_type';
    protected $primaryKey = 'type_id';

    protected $fillable = [
        'type_name',
        'rate_per_m3',
        'min_m3',
        'min_charge',
        'penalty',
    ];

    protected $casts = [
        'rate_per_m3' => 'decimal:2',
        'min_m3' => 'integer',
        'min_charge' => 'decimal:2',
        'penalty' => 'decimal:2',
    ];
}