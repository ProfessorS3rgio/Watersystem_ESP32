<?php
use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;
return new class extends Migration {
    public function up(): void {
        Schema::create('customer_profiles', function (Blueprint $table) {
            $table->id();
            $table->foreignId('customer_id')->unique()->constrained('customer', 'customer_id')->cascadeOnDelete();
            foreach (['first_name', 'middle_name', 'last_name'] as $field) $table->string($field, 80)->nullable();
            $table->string('name_suffix', 20)->nullable();
            $table->date('birthdate')->nullable();
            $table->string('phone', 40)->nullable();
            $table->string('email', 254)->nullable();
            $table->string('meter_number', 80)->nullable()->unique();
            $table->string('meter_model', 80)->nullable();
            $table->string('meter_location')->nullable();
            $table->date('meter_installed_on')->nullable();
            $table->date('service_started_on')->nullable();
            $table->text('notes')->nullable();
            $table->timestamps();
        });
    }
    public function down(): void { Schema::dropIfExists('customer_profiles'); }
};
