<?php
namespace App\Models;
use Illuminate\Database\Eloquent\Model;
class CustomerProfile extends Model {
    protected $guarded = ['id'];
    public function customer() { return $this->belongsTo(Customer::class, 'customer_id', 'customer_id'); }
    public function hasStructuredName(): bool { return (bool) ($this->first_name && $this->last_name); }
    public function fullName(): string {
        return implode(' ', array_filter([$this->first_name, $this->middle_name, $this->last_name, $this->name_suffix], fn ($value) => $value !== null && $value !== ''));
    }
}
