<?php

namespace Database\Seeders;

use Illuminate\Database\Console\Seeds\WithoutModelEvents;
use Illuminate\Database\Seeder;

class CustomerTypeSeeder extends Seeder
{
    /**
     * Run the database seeds.
     */
    public function run(): void
    {
        \DB::table('customer_type')->insert([
            [
                'type_name' => 'Communal faucet',
                'rate_per_m3' => 10.00,
                'min_m3' => 10,
                'min_charge' => 100.00,
                'penalty' => 20.00,
                'Synced' => false,
                'last_sync' => null,
                'created_at' => now(),
                'updated_at' => now(),
            ],
            [
                'type_name' => 'Residential',
                'rate_per_m3' => 15.00,
                'min_m3' => 10,
                'min_charge' => 150.00,
                'penalty' => 50.00,
                'Synced' => false,
                'last_sync' => null,
                'created_at' => now(),
                'updated_at' => now(),
            ],
            [
                'type_name' => 'Commercial',
                'rate_per_m3' => 20.00,
                'min_m3' => 10,
                'min_charge' => 200.00,
                'penalty' => 50.00,
                'Synced' => false,
                'last_sync' => null,
                'created_at' => now(),
                'updated_at' => now(),
            ],

            [
                'type_name' => 'Officers',
                'rate_per_m3' => 5.00,
                'min_m3' => 10,
                'min_charge' => 100.00,
                'penalty' => 20.00,
                'Synced' => false,
                'last_sync' => null,
                'created_at' => now(),
                'updated_at' => now(),
            ],
        ]);
    }
}
