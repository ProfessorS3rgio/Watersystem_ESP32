<?php
use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;
return new class extends Migration {
    public function up(): void {
        Schema::create('sync_reviews', function (Blueprint $table) {
            $table->uuid('id')->primary();
            $table->unsignedBigInteger('user_id')->index();
            $table->string('actor_name');
            $table->string('device_uid')->index();
            $table->string('status')->default('pending');
            $table->longText('payload')->nullable();
            $table->json('preview');
            $table->string('fingerprint', 64);
            $table->string('payload_hash', 64);
            $table->json('receipt')->nullable();
            $table->timestamp('expires_at');
            $table->timestamp('approved_at')->nullable();
            $table->timestamp('acknowledged_at')->nullable();
            $table->timestamps();
        });
        Schema::table('bill_transaction', function (Blueprint $table) {
            $table->unsignedBigInteger('device_transaction_id')->nullable();
            $table->unique(['processed_by_device_uid', 'device_transaction_id'], 'transaction_device_record_unique');
        });
    }
    public function down(): void {
        Schema::table('bill_transaction', function (Blueprint $table) {
            $table->dropUnique('transaction_device_record_unique');
            $table->dropColumn('device_transaction_id');
        });
        Schema::dropIfExists('sync_reviews');
    }
};
