<?php

namespace App\Http\Controllers;

use Illuminate\Http\Client\ConnectionException;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\Http;

class WirelessSyncController extends Controller
{
    // Deliberately fixed: requests cannot turn this relay into an arbitrary URL proxy.
    private const DEVICE_URL = 'http://192.168.4.1';

    public function connect(Request $request)
    {
        $request->session()->forget('device_wifi_token');
        $validated = $request->validate(['token' => ['required', 'string', 'regex:/^[A-Fa-f0-9]{8}$/']]);
        $token = strtoupper($validated['token']);
        try {
            $response = $this->client($token, 5)->get(self::DEVICE_URL.'/sync/hello');
        } catch (ConnectionException $error) {
            return response()->json(['message' => 'Cannot reach the master. Enable Wireless Sync mode and connect this laptop to its DMBC-SYNC Wi-Fi network.'], 503);
        }
        if ($response->status() === 401) {
            return response()->json(['message' => 'Pairing code rejected. Enter the code currently shown on the master.'], 422);
        }
        if (!$response->successful() || $response->json('service') !== 'DMBC_SYNC' || $response->json('protocol') !== 1) {
            return response()->json(['message' => 'The device did not return a compatible DMBC sync response.'], 502);
        }
        $request->session()->put('device_wifi_token', $token);
        return response()->json(['device_id' => $response->json('device_id'), 'transport' => 'wifi']);
    }

    public function command(Request $request)
    {
        $validated = $request->validate(['command' => ['required', 'string', 'max:4096']]);
        $command = $validated['command'];
        if (!$this->allowed($command)) {
            return response()->json(['message' => 'Unsupported Wi-Fi command. Use USB for device restart or SD formatting.'], 422);
        }
        $token = $request->session()->get('device_wifi_token');
        if (!$token) return response()->json(['message' => 'Pair the master before sending Wi-Fi commands.'], 409);
        try {
            $response = $this->client($token, 55)->withBody($command, 'text/plain')->post(self::DEVICE_URL.'/sync/command');
        } catch (ConnectionException $error) {
            $request->session()->forget('device_wifi_token');
            return response()->json(['message' => 'Wi-Fi device connection was lost. Reconnect the laptop to the master and pair again.'], 503);
        }
        if (!$response->successful()) {
            if ($response->status() === 401) $request->session()->forget('device_wifi_token');
            return response()->json(['message' => $response->status() === 401 ? 'Wireless session expired. Check the master display and pair again.' : 'The master rejected the wireless command.'], $response->status() === 401 ? 409 : 502);
        }
        return response($response->body(), 200)->header('Content-Type', 'text/plain; charset=utf-8')->header('Cache-Control', 'no-store');
    }

    public function disconnect(Request $request)
    {
        $request->session()->forget('device_wifi_token');
        return response()->json(['disconnected' => true]);
    }

    private function client(string $token, int $timeout)
    {
        return Http::withHeaders(['X-DMBC-Token' => $token])->connectTimeout(3)->timeout($timeout)
            ->withoutRedirecting()->withOptions(['proxy' => '']);
    }

    private function allowed(string $command): bool
    {
        if (str_contains($command, "\n") || str_contains($command, "\r")) return false;
        if (in_array($command, ['EXPORT_DEVICE_INFO', 'EXPORT_READINGS', 'EXPORT_ALL_READINGS', 'EXPORT_BILLS', 'EXPORT_BILL_TRANSACTIONS', 'READINGS_SYNCED', 'BILLS_SYNCED', 'BILL_TRANSACTIONS_SYNCED', 'RELOAD_SD'], true)) return true;
        foreach (['SET_TIME|', 'SET_LAST_SYNC|', 'QUERY_READINGS|', 'UPSERT_NEW_CUSTOMER_JSON_CHUNK|', 'UPSERT_UPDATED_CUSTOMER_JSON_CHUNK|', 'UPSERT_CUSTOMERS_JSON_CHUNK|', 'UPSERT_CUSTOMERS_JSON|', 'UPSERT_BILLS_JSON_CHUNK|', 'UPSERT_CUSTOMER_TYPE|', 'UPSERT_DEDUCTION|', 'UPSERT_BARANGAY|', 'UPSERT_SETTINGS|', 'NORMALIZE_DATES ', 'NORMALIZE_READING_IDS '] as $prefix) {
            if (str_starts_with($command, $prefix)) return true;
        }
        return false;
    }
}
