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
            $table->id('bill_id');
            $table->unsignedBigInteger('customer_id');
            $table->unsignedBigInteger('reading_id');
            $table->string('bill_no')->unique();
            $table->date('bill_date');
            $table->decimal('rate_per_m3', 10, 2);
            $table->decimal('charges', 10, 2);
            $table->decimal('penalty', 10, 2);
            $table->decimal('total_due', 10, 2);
            $table->date('due_date')->nullable();
            $table->enum('status', ['Paid', 'Void', 'Pending', 'Due'])->default('Pending');
            $table->timestamps();

            $table->foreign('customer_id')->references('customer_id')->on('customer');
            $table->foreign('reading_id')->references('reading_id')->on('reading');
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
