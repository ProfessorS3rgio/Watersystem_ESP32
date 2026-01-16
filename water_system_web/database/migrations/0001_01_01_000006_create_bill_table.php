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
        Schema::create('bill', function (Blueprint $table) {
            $table->id();

            $table->foreignId('customer_id')
                ->constrained('customer')
                ->cascadeOnDelete();

            $table->foreignId('reading_id')
                ->constrained('reading')
                ->cascadeOnDelete();

            $table->string('bill_no')->unique();
            $table->date('bill_date');

            $table->decimal('rate_per_m3', 10, 2);
            $table->decimal('charges', 10, 2);
            $table->decimal('penalty', 10, 2);
            $table->decimal('total_due', 10, 2);

            $table->date('due_date')->nullable();
            $table->string('status')->nullable();

            $table->timestamps();
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('bill');
    }
};
