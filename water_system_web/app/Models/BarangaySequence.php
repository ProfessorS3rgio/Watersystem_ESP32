<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Factories\HasFactory;
use Illuminate\Database\Eloquent\Model;

class BarangaySequence extends Model
{
    use HasFactory;

    protected $table = 'barangay_sequence';
    protected $primaryKey = 'brgy_id';
    public $incrementing = true;
    protected $keyType = 'int';

    protected $fillable = [
        'barangay',
        'prefix',
        'next_number',
    ];

    protected $casts = [
        'next_number' => 'integer',
    ];

    public function generateAccountNumber()
    {
        $number = str_pad($this->next_number, 3, '0', STR_PAD_LEFT);
        return $this->prefix . '-' . $number;
    }

    public function incrementSequence()
    {
        $this->increment('next_number');
    }
}