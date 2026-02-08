<?php

namespace Database\Seeders;

use Illuminate\Database\Console\Seeds\WithoutModelEvents;
use Illuminate\Database\Seeder;

class SettingsSeeder extends Seeder
{
    /**
     * Run the database seeds.
     */
    public function run(): void
    {
        \DB::table('settings')->insert([
            [
                'bill_due_days' => 5,
                'disconnection_days' => 8,
                'Synced' => false,
                'last_sync' => null,
                'created_at' => now(),
                'updated_at' => now(),
            ],
        ]);
    }
}