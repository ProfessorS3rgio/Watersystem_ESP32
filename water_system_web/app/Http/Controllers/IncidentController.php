<?php

namespace App\Http\Controllers;

use App\Models\Customer;
use App\Models\ServiceIncident;
use App\Services\IncidentService;
use Illuminate\Http\Request;
use Illuminate\Validation\Rule;

class IncidentController extends Controller
{
    public function index(Request $request)
    {
        $filters = $request->validate(['q' => 'nullable|string|max:120', 'status' => ['nullable', Rule::in(['open', 'all', 'completed', ...array_keys(ServiceIncident::STATUSES)])],
            'priority' => ['nullable', Rule::in(array_keys(ServiceIncident::PRIORITIES))], 'category' => ['nullable', Rule::in(array_keys(ServiceIncident::CATEGORIES))],
            'customer_id' => 'nullable|integer|min:1', 'page' => 'nullable|integer|min:1']);
        $query = ServiceIncident::query();
        if (!empty($filters['customer_id'])) $query->where('customer_id', $filters['customer_id']);
        if ($term = trim($filters['q'] ?? '')) $query->where(function ($search) use ($term) {
            foreach (['reference_number', 'account_no', 'customer_name', 'title', 'location', 'assigned_to', 'barangay'] as $field) $search->orWhere($field, 'like', '%'.$term.'%');
        });
        foreach (['priority', 'category'] as $field) if (!empty($filters[$field])) $query->where($field, $filters[$field]);
        $summary = [
            'open' => (clone $query)->whereIn('status', ServiceIncident::OPEN)->count(),
            'urgent' => (clone $query)->whereIn('status', ServiceIncident::OPEN)->where('priority', 'urgent')->count(),
            'in_progress' => (clone $query)->where('status', 'in_progress')->count(),
            'resolved' => (clone $query)->whereIn('status', ['resolved', 'closed'])->count(),
        ];
        $status = $filters['status'] ?? 'open';
        if ($status === 'open') $query->whereIn('status', ServiceIncident::OPEN);
        elseif ($status === 'completed') $query->whereIn('status', ['resolved', 'closed']);
        elseif ($status !== 'all') $query->where('status', $status);
        $page = $query->select(['id', 'reference_number', 'customer_id', 'account_no', 'customer_name', 'barangay', 'title', 'location', 'category', 'pipe_section', 'priority', 'status', 'assigned_to', 'created_at', 'updated_at', 'resolved_at', 'version'])
            ->orderByRaw("CASE priority WHEN 'urgent' THEN 0 WHEN 'high' THEN 1 WHEN 'normal' THEN 2 ELSE 3 END")
            ->orderByDesc('id')->paginate(15);
        return response()->json([...$page->toArray(), 'summary' => $summary, 'options' => self::options()])->header('Cache-Control', 'no-store');
    }

    public static function options(): array
    {
        return ['statuses' => ServiceIncident::STATUSES, 'categories' => ServiceIncident::CATEGORIES, 'priorities' => ServiceIncident::PRIORITIES, 'sections' => ServiceIncident::SECTIONS];
    }

    public function customers(Request $request)
    {
        $filters = $request->validate(['q' => 'nullable|string|max:120', 'id' => 'nullable|integer|min:1']);
        $query = Customer::query()->select('customer_id', 'account_no', 'customer_name', 'address', 'status');
        if (!empty($filters['id'])) $query->where('customer_id', $filters['id']);
        elseif ($term = trim($filters['q'] ?? '')) $query->where(fn ($q) => $q->where('account_no', 'like', '%'.$term.'%')->orWhere('customer_name', 'like', '%'.$term.'%'));
        else return response()->json(['data' => []]);
        return response()->json(['data' => $query->withCount(['incidents as open_incidents_count' => fn ($q) => $q->whereIn('status', ServiceIncident::OPEN)])->orderBy('customer_name')->limit(15)->get()]);
    }

    public function show(ServiceIncident $incident)
    {
        $incident->load('customer:customer_id,account_no,customer_name,address')->loadCount('events');
        $history = $this->historyPage($incident);
        return response()->json(['data' => [...$incident->toArray(), 'events' => $history['data'], 'more_history_before' => $history['before_id'], 'allowed_statuses' => ServiceIncident::TRANSITIONS[$incident->status],
            'other_open_incidents' => ServiceIncident::where('customer_id', $incident->customer_id)->whereNotNull('customer_id')->whereKeyNot($incident->id)->whereIn('status', ServiceIncident::OPEN)->select('id', 'reference_number', 'title')->limit(5)->get()], 'options' => self::options()])->header('Cache-Control', 'no-store');
    }

    public function history(Request $request, ServiceIncident $incident)
    {
        $data = $request->validate(['before_id' => 'nullable|integer|min:1']);
        return response()->json($this->historyPage($incident, $data['before_id'] ?? null))->header('Cache-Control', 'no-store');
    }

    private function historyPage(ServiceIncident $incident, ?int $before = null): array
    {
        $query = $incident->events()->orderByDesc('id');
        if ($before) $query->where('id', '<', $before);
        $events = $query->limit(31)->get();
        return ['data' => $events->take(30)->values(), 'before_id' => $events->count() > 30 ? $events[29]->id : null];
    }

    public function store(Request $request, IncidentService $service)
    {
        $data = $request->validate(['client_token' => 'required|uuid', 'customer_id' => 'required|integer|exists:customer,customer_id', ...$this->fields(true)]);
        $data['pipe_section'] ??= 'unknown'; $data['assigned_to'] ??= null;
        [$incident, $created] = $service->create($data);
        return response()->json(['data' => $incident], $created ? 201 : 200);
    }

    public function update(Request $request, ServiceIncident $incident, IncidentService $service)
    {
        $data = $request->validate(['version' => 'required|integer|min:1', ...$this->fields(false), 'status' => ['sometimes', 'required', Rule::in(array_keys(ServiceIncident::STATUSES))],
            'note' => 'nullable|string|min:3|max:5000', 'resolution' => 'nullable|string|max:5000']);
        $service->update($incident, $data);
        return $this->show($incident->fresh());
    }

    private function fields(bool $required): array
    {
        $rule = $required ? 'required' : 'sometimes|required';
        return ['title' => $rule.'|string|max:180', 'description' => $rule.'|string|min:5|max:5000', 'location' => $rule.'|string|max:1000',
            'category' => [$required ? 'required' : 'sometimes', 'required', Rule::in(array_keys(ServiceIncident::CATEGORIES))],
            'priority' => [$required ? 'required' : 'sometimes', 'required', Rule::in(array_keys(ServiceIncident::PRIORITIES))],
            'pipe_section' => ['sometimes', 'required', Rule::in(array_keys(ServiceIncident::SECTIONS))], 'assigned_to' => 'sometimes|nullable|string|max:160'];
    }
}
