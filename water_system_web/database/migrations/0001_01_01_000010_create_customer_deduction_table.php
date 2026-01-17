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
        Schema::create('customer_deduction', function (Blueprint $table) {
            $table->unsignedBigInteger('customer_id');
            $table->unsignedBigInteger('deduction_id');
            $table->timestamps();

            $table->unique(['customer_id', 'deduction_id']);

            $table->foreign('customer_id')->references('customer_id')->on('customer');
            $table->foreign('deduction_id')->references('deduction_id')->on('deduction');
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('customer_deduction');
    }
};