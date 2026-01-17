<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Factories\HasFactory;
use Illuminate\Database\Eloquent\Model;

class Deduction extends Model
{
    use HasFactory;

    protected $table = 'deduction';

    protected $primaryKey = 'deduction_id';

    protected $fillable = [
        'name',
        'type',
        'value',
    ];

    protected $casts = [
        'value' => 'decimal:2',
    ];
}