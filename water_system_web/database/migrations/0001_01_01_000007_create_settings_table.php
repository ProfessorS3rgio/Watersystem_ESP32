<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    /**
     * Run the migrations.
     */
    public function up(): void
    {
        Schema::create('settings', function (Blueprint $table) {
            $table->id();

            $table->decimal('rate_per_m3', 10, 2)->default(15.00);
            $table->unsignedInteger('bill_due_days')->default(15);

            // Penalty rule:
            // if today > due_date + penalty_after_days, then apply penalty_amount.
            $table->unsignedInteger('penalty_after_days')->default(0);
            $table->decimal('penalty_amount', 10, 2)->default(0.00);

            $table->timestamps();
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('settings');
    }
};
