<?php

namespace App\Http\Controllers;

use App\Models\AuditLog;
use Illuminate\Http\Request;

class AuditLogController extends Controller
{
    public function index(Request $request)
    {
        $query = AuditLog::query()->latest();

        if ($request->filled('action')) $query->where('action', $request->string('action'));
        if ($request->filled('auditable_type')) $query->where('auditable_type', $request->string('auditable_type'));
        if ($request->filled('date_from')) $query->whereDate('created_at', '>=', $request->date('date_from'));
        if ($request->filled('date_to')) $query->whereDate('created_at', '<=', $request->date('date_to'));

        return response()->json($query->paginate(30));
    }
}
