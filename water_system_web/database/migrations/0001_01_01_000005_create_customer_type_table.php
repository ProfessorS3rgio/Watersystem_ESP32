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
        Schema::create('customer_type', function (Blueprint $table) {
            $table->id('type_id');
            $table->string('type_name')->unique();
            $table->decimal('rate_per_m3', 10, 2);
            $table->integer('min_m3')->default(0);
            $table->decimal('min_charge', 10, 2);
            $table->decimal('penalty', 10, 2);
            $table->timestamps();
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('customer_type');
    }
};