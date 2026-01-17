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
                'type_name' => 'Residential',
                'rate_per_m3' => 15.00,
                'min_m3' => 10,
                'min_charge' => 150.00,
                'created_at' => now(),
                'updated_at' => now(),
            ],
            [
                'type_name' => 'Commercial',
                'rate_per_m3' => 20.00,
                'min_m3' => 20,
                'min_charge' => 400.00,
                'created_at' => now(),
                'updated_at' => now(),
            ],
            [
                'type_name' => 'Industrial',
                'rate_per_m3' => 25.00,
                'min_m3' => 50,
                'min_charge' => 1250.00,
                'created_at' => now(),
                'updated_at' => now(),
            ],
        ]);
    }
}
