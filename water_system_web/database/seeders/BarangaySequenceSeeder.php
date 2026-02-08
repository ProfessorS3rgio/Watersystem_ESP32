<?php

namespace Database\Seeders;

use Illuminate\Database\Console\Seeds\WithoutModelEvents;
use Illuminate\Database\Seeder;

class BarangaySequenceSeeder extends Seeder
{
    /**
     * Run the database seeds.
     */
    public function run(): void
    {
        \DB::table('barangay_sequence')->insert([
            ['barangay' => 'Dona Josefa', 'prefix' => 'D', 'next_number' => 1, 'Synced' => false, 'last_sync' => null, 'updated_at' => now()],
            ['barangay' => 'Makilas', 'prefix' => 'M', 'next_number' => 1, 'Synced' => false, 'last_sync' => null, 'updated_at' => now()],
            ['barangay' => 'Buluan', 'prefix' => 'B', 'next_number' => 1, 'Synced' => false, 'last_sync' => null, 'updated_at' => now()],
            ['barangay' => 'Caparan', 'prefix' => 'C', 'next_number' => 1, 'Synced' => false, 'last_sync' => null, 'updated_at' => now()],
        ]);
    }
}
