<?php

namespace App\Http\Controllers;

use App\Models\BillTransaction;
use Illuminate\Http\Request;

class BillTransactionController extends Controller
{
    public function sync(Request $request)
    {
        $validated = $request->validate([
            'bill_transactions' => ['required', 'array', 'max:1000'],
            'bill_transactions.*.bill_transaction_id' => ['nullable', 'integer'],
            'bill_transactions.*.bill_id' => ['required', 'integer'],
            'bill_transactions.*.bill_reference_number' => ['nullable', 'string', 'max:255'],
            'bill_transactions.*.type' => ['nullable', 'string', 'max:255'],
            'bill_transactions.*.source' => ['nullable', 'string', 'max:255'],
            'bill_transactions.*.amount' => ['nullable', 'numeric'],
            'bill_transactions.*.cash_received' => ['nullable', 'numeric'],
            'bill_transactions.*.change' => ['nullable', 'numeric'],
            'bill_transactions.*.transaction_date' => ['nullable', 'string'],
            'bill_transactions.*.payment_method' => ['nullable', 'string', 'max:255'],
            'bill_transactions.*.processed_by_device_uid' => ['nullable', 'string', 'max:255'],
            'bill_transactions.*.notes' => ['nullable', 'string'],
            'bill_transactions.*.created_at' => ['nullable', 'string'],
            'bill_transactions.*.updated_at' => ['nullable', 'string'],
        ]);

        $billTransactions = $validated['bill_transactions'];
        $processed = 0;

        \Log::info('Syncing bill transactions', ['count' => count($billTransactions)]);

        foreach ($billTransactions as $row) {
            BillTransaction::updateOrCreate(
                ['bill_transaction_id' => $row['bill_transaction_id']],
                [
                    'bill_id' => $row['bill_id'],
                    'bill_reference_number' => $row['bill_reference_number'] ?? null,
                    'type' => $row['type'] ?? 'payment',
                    'source' => $row['source'] ?? 'Device',
                    'amount' => $row['amount'] ?? 0,
                    'cash_received' => $row['cash_received'] ?? 0,
                    'change' => $row['change'] ?? 0,
                    'transaction_date' => $row['transaction_date'] ?? now(),
                    'payment_method' => $row['payment_method'] ?? 'cash',
                    'processed_by_device_uid' => $row['processed_by_device_uid'] ?? null,
                    'notes' => $row['notes'] ?? null,
                    'created_at' => $row['created_at'] ?? now(),
                    'updated_at' => $row['updated_at'] ?? now(),
                ]
            );

            $processed++;
        }

        \Log::info('Bill transactions synced', ['processed' => $processed]);

        return response()->json([
            'processed' => $processed,
        ]);
    }
}