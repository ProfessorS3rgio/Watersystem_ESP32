<?php

namespace App\Http\Controllers;

use App\Models\SyncRun;
use App\Services\AuditLogService;
use Illuminate\Http\Request;
use Illuminate\Support\Str;

class SyncRunController extends Controller
{
    public function start(Request $request, AuditLogService $audit)
    {
        $run = SyncRun::create([
            'run_uuid' => (string) Str::uuid(),
            'status' => 'started',
            'device_uid' => $request->input('device_uid'),
            'started_at' => now(),
        ]);

        $audit->record('sync_started', 'SyncRun', null, ['run_uuid' => $run->run_uuid], [
            'auditable_id' => $run->id,
            'sync_run_id' => $run->run_uuid,
            'device_uid' => $run->device_uid,
        ]);

        return response()->json(['run_id' => $run->run_uuid], 201);
    }

    public function finish(Request $request, string $run, AuditLogService $audit)
    {
        $syncRun = SyncRun::where('run_uuid', $run)->firstOrFail();
        $status = $request->input('status', 'completed');
        if (!in_array($status, ['completed', 'failed'], true)) {
            return response()->json(['message' => 'Invalid sync status.'], 422);
        }

        $syncRun->update([
            'status' => $status,
            'summary' => $request->input('summary'),
            'error' => $request->input('error'),
            'finished_at' => now(),
        ]);

        $audit->record($status === 'completed' ? 'sync_completed' : 'sync_failed', 'SyncRun', null, [
            'run_uuid' => $syncRun->run_uuid,
            'status' => $status,
            'summary' => $syncRun->summary,
            'error' => $syncRun->error,
        ], [
            'auditable_id' => $syncRun->id,
            'sync_run_id' => $syncRun->run_uuid,
            'device_uid' => $syncRun->device_uid,
        ]);

        return response()->json(['status' => $status]);
    }
}
