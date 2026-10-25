<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class ServiceIncident extends Model
{
    protected $guarded = ['id'];
    protected $hidden = ['client_token', 'request_hash'];
    protected $casts = ['version' => 'integer', 'reopened_count' => 'integer', 'resolved_at' => 'datetime', 'closed_at' => 'datetime'];

    public const OPEN = ['reported', 'in_progress'];
    public const STATUSES = ['reported' => 'Reported', 'in_progress' => 'In progress', 'resolved' => 'Resolved', 'closed' => 'Closed', 'cancelled' => 'Cancelled'];
    public const CATEGORIES = ['leak' => 'Leaking pipe / connection', 'no_water' => 'No water supply', 'low_pressure' => 'Low water pressure', 'meter' => 'Meter concern', 'other' => 'Other service issue'];
    public const PRIORITIES = ['low' => 'Low', 'normal' => 'Normal', 'high' => 'High', 'urgent' => 'Urgent'];
    public const SECTIONS = ['unknown' => 'Needs inspection', 'before_meter' => 'Before the meter', 'meter' => 'At the meter / connection', 'after_meter' => 'After the meter'];
    public const TRANSITIONS = [
        'reported' => ['in_progress', 'resolved', 'cancelled'],
        'in_progress' => ['reported', 'resolved', 'cancelled'],
        'resolved' => ['closed', 'in_progress'],
        'closed' => ['in_progress'],
        'cancelled' => ['reported'],
    ];

    public function customer() { return $this->belongsTo(Customer::class, 'customer_id', 'customer_id'); }
    public function events() { return $this->hasMany(IncidentEvent::class, 'incident_id'); }
}
