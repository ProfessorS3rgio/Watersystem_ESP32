<?php
namespace App\Http\Controllers;

use App\Models\SyncReview;
use App\Services\{AuditLogService, SyncVerificationService};
use Illuminate\Http\Request;
use Illuminate\Support\Facades\DB;
use Illuminate\Support\Str;

class SyncReviewController extends Controller
{
    private function owned(Request $request, string $id, bool $lock = false): SyncReview {
        $query = SyncReview::where('id', $id)->where('user_id', $request->user()->id);
        return ($lock ? $query->lockForUpdate() : $query)->firstOrFail();
    }

    public function preview(Request $request, SyncVerificationService $service, AuditLogService $audit) {
        $payload = $service->validate($request->all());
        $plan = $service->preview($payload);
        $fingerprint = $plan['fingerprint']; unset($plan['fingerprint']);
        // Expired staged data is unnecessary; keep its review summary as history.
        SyncReview::where('user_id', $request->user()->id)->where('status', 'pending')->where('expires_at', '<', now())->update(['payload' => null, 'status' => 'expired']);
        $review = SyncReview::create(['id' => (string) Str::uuid(), 'user_id' => $request->user()->id, 'actor_name' => $request->user()->name,
            'device_uid' => $payload['device']['device_uid'], 'payload' => $payload, 'payload_hash' => hash('sha256', json_encode($payload, JSON_THROW_ON_ERROR)),
            'preview' => $plan, 'fingerprint' => $fingerprint, 'expires_at' => now()->addMinutes(30)]);
        $audit->record('sync_review_created', 'SyncReview', null, ['summary' => $plan['summary']], ['device_uid' => $review->device_uid, 'review_id' => $review->id]);
        return response()->json(['review' => $review], 201);
    }

    public function approve(Request $request, string $id, SyncVerificationService $service, AuditLogService $audit) {
        $request->validate(['device_uid' => 'required|string', 'accept_warnings' => 'required|boolean']);
        return DB::transaction(function () use ($request, $id, $service, $audit) {
            $review = $this->owned($request, $id, true);
            abort_unless($request->device_uid === $review->device_uid, 409, 'Connected device changed. Start a new review.');
            if ($review->status === 'approved') return response()->json(['receipt' => $review->receipt]);
            abort_unless($review->status === 'pending' && $review->expires_at->isFuture(), 409, 'This review expired or was cancelled. Read the device again.');
            $plan = $service->preview($review->payload, true);
            abort_unless(hash_equals($review->fingerprint, $plan['fingerprint']), 409, 'Website records changed during review. Refresh the review before approving.');
            abort_unless($plan['can_approve'], 422, 'Resolve all blocking issues before approving this batch.');
            abort_if($plan['summary']['warnings'] > 0 && !$request->boolean('accept_warnings'), 422, 'Review and accept the warnings before saving.');
            // Device rows are needed for foreign keys; registration is in the same transaction.
            $device = $review->payload['device'];
            $existing = DB::table('device')->where('device_uid', $review->device_uid)->first();
            if (!$existing) DB::table('device')->insert(['device_uid' => $review->device_uid, 'device_mac' => $review->device_uid, 'brgy_id' => $device['brgy_id'], 'firmware_version' => $device['firmware_version'] ?? 'unknown', 'created_at' => now(), 'updated_at' => now()]);
            $service->apply($review->payload, $plan);
            $receipt = ['review_id' => $review->id, 'device_uid' => $review->device_uid, 'approved_by' => $request->user()->name,
                'approved_at' => now()->toIso8601String(), 'summary' => $plan['summary'], 'accepted_warnings' => $plan['summary']['warnings'] > 0,
                'payload_hash' => $review->payload_hash];
            $review->update(['status' => 'approved', 'payload' => null, 'actor_name' => $request->user()->name, 'approved_at' => now(), 'receipt' => $receipt]);
            $audit->record('sync_batch_approved', 'SyncReview', null, $receipt, ['device_uid' => $review->device_uid, 'review_id' => $review->id]);
            return response()->json(['receipt' => $receipt]);
        }, 3);
    }

    public function cancel(Request $request, string $id, AuditLogService $audit) {
        return DB::transaction(function () use ($request, $id, $audit) {
            $review = $this->owned($request, $id, true);
            abort_unless(in_array($review->status, ['pending', 'cancelled', 'expired']), 409, 'This batch was already saved.');
            if ($review->status === 'pending') {
                $review->update(['status' => 'cancelled', 'payload' => null]);
                $audit->record('sync_review_cancelled', 'SyncReview', null, ['summary' => $review->preview['summary']], ['device_uid' => $review->device_uid, 'review_id' => $review->id]);
            }
            return response()->json(['status' => $review->status]);
        });
    }

    public function acknowledged(Request $request, string $id, AuditLogService $audit) {
        return DB::transaction(function () use ($request, $id, $audit) {
            $review = $this->owned($request, $id, true);
            abort_unless($review->status === 'approved', 409, 'This batch has not been saved.');
            if (!$review->acknowledged_at) {
                $review->update(['acknowledged_at' => now()]);
                $audit->record('sync_batch_acknowledged', 'SyncReview', null, ['review_id' => $id], ['device_uid' => $review->device_uid, 'review_id' => $review->id]);
            }
            return response()->json(['acknowledged_at' => $review->acknowledged_at]);
        });
    }

    public function history(Request $request) {
        return response()->json(['reviews' => SyncReview::where('user_id', $request->user()->id)->latest()->limit(20)->get(['id', 'device_uid', 'actor_name', 'status', 'receipt', 'acknowledged_at', 'created_at'])]);
    }
}
