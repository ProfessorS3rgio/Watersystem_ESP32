<?php
namespace App\Models;
use Illuminate\Database\Eloquent\Model;
class SyncReview extends Model {
    public $incrementing = false;
    protected $keyType = 'string';
    protected $guarded = [];
    protected $hidden = ['payload', 'fingerprint'];
    protected $casts = ['payload' => 'encrypted:array', 'preview' => 'array', 'receipt' => 'array', 'expires_at' => 'datetime', 'approved_at' => 'datetime', 'acknowledged_at' => 'datetime'];
}
