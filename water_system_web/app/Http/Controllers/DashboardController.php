<?php

namespace App\Http\Controllers;

use App\Models\Customer;
use App\Models\Reading;
use Carbon\Carbon;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\DB;

class DashboardController extends Controller
{
    public function index(Request $request)
    {
        $start = Carbon::now()->startOfMonth()->toDateString();
        $end = Carbon::now()->endOfMonth()->toDateString();

        $totalCustomers = Customer::query()->count();

        // Heuristic: treat each distinct Reading.source as a "device".
        // Today it's typically just "device"; later you can set a unique device id.
        $numberOfDevices = Reading::query()
            ->whereNotNull('source')
            ->distinct()
            ->count('source');

        $totalCollectableThisMonth = (float) DB::table('bill')
            ->where(function ($q) {
                $q->whereNull('status')
                    ->orWhereRaw('LOWER(status) != ?', ['paid']);
            })
            ->where(function ($q) use ($start, $end) {
                $q->whereBetween('due_date', [$start, $end])
                    ->orWhere(function ($q) use ($start, $end) {
                        $q->whereNull('due_date')->whereBetween('bill_date', [$start, $end]);
                    });
            })
            ->sum('total_due');

        $totalSalesThisMonth = (float) DB::table('bill')
            ->whereRaw('LOWER(COALESCE(status, \'\')) = ?', ['paid'])
            ->whereBetween('bill_date', [$start, $end])
            ->sum('total_due');

        return response()->json([
            'total_collectable_this_month' => $totalCollectableThisMonth,
            'number_of_devices' => $numberOfDevices,
            'total_customers' => $totalCustomers,
            'total_sales_this_month' => $totalSalesThisMonth,
        ]);
    }
}
