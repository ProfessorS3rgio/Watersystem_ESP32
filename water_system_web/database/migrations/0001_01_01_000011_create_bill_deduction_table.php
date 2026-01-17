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
        Schema::create('bill_deduction', function (Blueprint $table) {
            $table->id('bill_deduction_id');
            $table->unsignedBigInteger('bill_id');
            $table->unsignedBigInteger('deduction_id');
            $table->decimal('amount', 10, 2);
            $table->timestamps();

            $table->foreign('bill_id')->references('bill_id')->on('bill');
            $table->foreign('deduction_id')->references('deduction_id')->on('deduction');
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('bill_deduction');
    }
};