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
        Schema::create('bill_transaction', function (Blueprint $table) {
            $table->id('bill_transaction_id');
            $table->unsignedBigInteger('bill_id');
            $table->enum('type', ['payment', 'void']);
            $table->decimal('amount', 10, 2);
            $table->decimal('cash_received', 10, 2)->nullable();
            $table->decimal('change', 10, 2);
            $table->dateTime('transaction_date');
            $table->string('payment_method');
            $table->unsignedBigInteger('processed_by_user_id')->nullable();
            $table->text('notes')->nullable();
            $table->timestamps();

            $table->foreign('bill_id')->references('bill_id')->on('bill');
            $table->foreign('processed_by_user_id')->references('id')->on('users');
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('bill_transaction');
    }
};