<?php

namespace Database\Seeders;

use Illuminate\Database\Console\Seeds\WithoutModelEvents;
use Illuminate\Database\Seeder;
use Faker\Factory as Faker;

class CustomerSeeder extends Seeder
{
    /**
     * Run the database seeds.
     */
    public function run(): void
    {
        $faker = Faker::create();

        $brgyId = 2; // Makilas
        $prefix = 'M';

        // Find the highest existing account number for this barangay
        $maxAccount = \DB::table('customer')
            ->where('brgy_id', $brgyId)
            ->where('account_no', 'like', $prefix . '-%')
            ->orderByRaw('CAST(SUBSTRING(account_no, 3) AS UNSIGNED) DESC')
            ->value('account_no');

        $startNumber = 1;
        if ($maxAccount) {
            $startNumber = (int)substr($maxAccount, 2) + 1;
        }

        $customers = [];

        for ($i = 0; $i < 1000; $i++) {
            $number = $startNumber + $i;
            $accountNo = $prefix . '-' . str_pad($number, 3, '0', STR_PAD_LEFT);
            $typeId = rand(1, 4);
            $deductionId = rand(1, 5) === 1 ? 1 : null; // 20% chance for deduction
            $address = 'Prk ' . rand(1, 20) . ', ' . $faker->streetName;
            $previousReading = rand(0, 1000);

            $customers[] = [
                'account_no' => $accountNo,
                'customer_name' => $faker->name(),
                'type_id' => $typeId,
                'deduction_id' => $deductionId,
                'brgy_id' => $brgyId,
                'address' => $address,
                'previous_reading' => $previousReading,
                'status' => 'active',
                'created_at' => now(),
                'updated_at' => now(),
            ];
        }

        \DB::table('customer')->insert($customers);

        // Update next_number for Makilas
        $nextNumber = $startNumber + 1000;
        \DB::table('barangay_sequence')->where('brgy_id', $brgyId)->update([
            'next_number' => $nextNumber,
            'updated_at' => now(),
        ]);
    }
}