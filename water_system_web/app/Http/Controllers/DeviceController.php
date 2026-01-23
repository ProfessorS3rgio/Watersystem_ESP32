<?php

namespace App\Http\Controllers;

use App\Models\Device;
use Illuminate\Http\Request;

class DeviceController extends Controller
{
    public function sync(Request $request)
    {
        \Log::info('Device sync request received', $request->all());

        $validated = $request->validate([
            'device_info' => ['required', 'array'],
            'device_info.brgy_id' => ['required', 'integer'],
            'device_info.device_mac' => ['required', 'string'],
            'device_info.device_uid' => ['nullable', 'string'],
            'device_info.firmware_version' => ['nullable', 'string'],
            'device_info.device_name' => ['nullable', 'string'],
            'device_info.print_count' => ['nullable', 'integer'],
            'device_info.customer_count' => ['nullable', 'integer'],
        ]);

        $info = $validated['device_info'];

        \Log::info('Validated device info', $info);

        $device = Device::updateOrCreate(
            ['device_mac' => $info['device_mac']],
            [
                'brgy_id' => $info['brgy_id'],
                'device_uid' => $info['device_uid'] ?? null,
                'firmware_version' => $info['firmware_version'] ?? null,
                'device_name' => $info['device_name'] ?? null,
                'print_count' => $info['print_count'] ?? 0,
                'customer_count' => $info['customer_count'] ?? 0,
                'last_sync' => now(),
            ]
        );

        \Log::info('Device synced', ['device_id' => $device->device_id]);

        return response()->json([
            'message' => 'Device info synced successfully',
            'device_mac' => $device->device_mac,
        ]);
    }
}