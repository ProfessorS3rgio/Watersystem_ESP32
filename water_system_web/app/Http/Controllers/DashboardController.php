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
        try {
            $now = Carbon::now();
            $startOfMonth = $now->copy()->startOfMonth()->toDateString();
            $endOfMonth = $now->copy()->endOfMonth()->toDateString();
            $startOfYear = $now->copy()->startOfYear()->toDateString();
            $endOfYear = $now->copy()->endOfYear()->toDateString();
            $lastMonthStart = $now->copy()->subMonthNoOverflow()->startOfMonth()->toDateString();
            $lastMonthEnd = $now->copy()->subMonthNoOverflow()->endOfMonth()->toDateString();

            $totalCustomers = Customer::query()->count();
            $newCustomersThisMonth = Customer::query()
                ->whereBetween('created_at', [$startOfMonth, $endOfMonth])
                ->count();

// Heuristic: treat each distinct Reading.device_uid as a "device".
        // Today it's typically just "device"; later you can set a unique device id.
        $numberOfDevices = Reading::query()
            ->whereNotNull('device_uid')
            ->distinct()
            ->count('device_uid');

            $totalCollectableLastMonth = (float) DB::table('bill')
                ->where(function ($q) {
                    $q->whereNull('status')
                        ->orWhereRaw('LOWER(status) != ?', ['paid']);
                })
                ->where(function ($q) use ($lastMonthStart, $lastMonthEnd) {
                    $q->whereBetween('due_date', [$lastMonthStart, $lastMonthEnd])
                        ->orWhere(function ($q) use ($lastMonthStart, $lastMonthEnd) {
                            $q->whereNull('due_date')->whereBetween('bill_date', [$lastMonthStart, $lastMonthEnd]);
                        });
                })
                ->sum('total_due');

            $totalSalesThisYear = (float) DB::table('bill')
                ->whereRaw('LOWER(COALESCE(status, \'\')) = ?', ['paid'])
                ->whereBetween('bill_date', [$startOfYear, $endOfYear])
                ->sum('total_due');

            $customerAllBalance = (float) DB::table('bill')
                ->where(function ($q) {
                    $q->whereNull('status')
                        ->orWhereRaw('LOWER(status) != ?', ['paid']);
                })
                ->sum('total_due');

            $totalCollectableThisMonth = (float) DB::table('bill')
                ->where(function ($q) {
                    $q->whereNull('status')
                        ->orWhereRaw('LOWER(status) != ?', ['paid']);
                })
                ->where(function ($q) use ($startOfMonth, $endOfMonth) {
                    $q->whereBetween('due_date', [$startOfMonth, $endOfMonth])
                        ->orWhere(function ($q) use ($startOfMonth, $endOfMonth) {
                            $q->whereNull('due_date')->whereBetween('bill_date', [$startOfMonth, $endOfMonth]);
                        });
                })
                ->sum('total_due');

            $totalSalesThisMonth = (float) DB::table('bill')
                ->whereRaw('LOWER(COALESCE(status, \'\')) = ?', ['paid'])
                ->whereBetween('bill_date', [$startOfMonth, $endOfMonth])
                ->sum('total_due');

            return response()->json([
                'total_collectable_this_month' => $totalCollectableThisMonth,
                'total_collectable_last_month' => $totalCollectableLastMonth,
                'number_of_devices' => $numberOfDevices,
                'total_customers' => $totalCustomers,
                'total_sales_this_month' => $totalSalesThisMonth,
                'new_customers_this_month' => $newCustomersThisMonth,
                'total_sales_this_year' => $totalSalesThisYear,
                'customer_all_balance' => $customerAllBalance,
            ]);
        } catch (\Exception $e) {
            \Log::error('Dashboard error: ' . $e->getMessage());
            return response()->json(['error' => 'Internal server error'], 500);
        }
    }
}
