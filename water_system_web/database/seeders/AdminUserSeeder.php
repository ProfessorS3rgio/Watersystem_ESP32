<?php

namespace Database\Seeders;

use App\Models\User;
use Illuminate\Database\Console\Seeds\WithoutModelEvents;
use Illuminate\Database\Seeder;
use Illuminate\Support\Facades\Hash;

class AdminUserSeeder extends Seeder
{
    use WithoutModelEvents;

    /**
     * Seed an admin user for initial authentication.
     */
    public function run(): void
    {
        $name = env('ADMIN_NAME', 'Admin');
        $email = env('ADMIN_EMAIL', 'admin@localhost');
        $password = env('ADMIN_PASSWORD', 'admin');

        User::updateOrCreate(
            ['email' => $email],
            [
                'name' => $name,
                'password' => Hash::make($password),
                'email_verified_at' => now(),
            ]
        );
    }
}
