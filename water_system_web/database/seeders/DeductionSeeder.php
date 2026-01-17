<?php

namespace Database\Seeders;

use Illuminate\Database\Console\Seeds\WithoutModelEvents;
use Illuminate\Database\Seeder;

class DeductionSeeder extends Seeder
{
    /**
     * Run the database seeds.
     */
    public function run(): void
    {
        \DB::table('deduction')->insert([
            [
                'name' => 'Senior Citizen',
                'type' => 'percentage',
                'value' => 20.00, // 20% discount
                'created_at' => now(),
                'updated_at' => now(),
            ],
            [
                'name' => 'SSS',
                'type' => 'fixed',
                'value' => 50.00, // 50 PHP deduction
                'created_at' => now(),
                'updated_at' => now(),
            ],
        ]);
    }
}
