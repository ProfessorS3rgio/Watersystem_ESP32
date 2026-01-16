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
        Schema::create('bill_transactions', function (Blueprint $table) {
            $table->id();
            $table->foreignId('bill_id')->constrained('bill')->onDelete('cascade');
            $table->enum('type', ['payment', 'void']);
            $table->decimal('amount', 10, 2);
            $table->decimal('cash_received', 10, 2)->nullable();
            $table->decimal('change', 10, 2)->default(0);
            $table->dateTime('transaction_date');
            $table->string('payment_method')->default('cash');
            $table->foreignId('processed_by_user_id')->nullable()->constrained('users')->onDelete('set null');
            $table->text('notes')->nullable();
            $table->timestamps();
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('bill_transactions');
    }
};
