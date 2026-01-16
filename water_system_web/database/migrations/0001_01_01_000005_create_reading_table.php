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
        Schema::create('reading', function (Blueprint $table) {
            $table->id();

            $table->foreignId('customer_id')
                ->constrained('customer')
                ->cascadeOnDelete();

            $table->integer('previous_reading');
            $table->integer('current_reading');
            $table->integer('usage_m3');
            $table->dateTime('reading_at');

            $table->foreignId('read_by_user_id')
                ->nullable()
                ->constrained('users')
                ->nullOnDelete();

            $table->string('source')->nullable();

            $table->timestamps();
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('reading');
    }
};
