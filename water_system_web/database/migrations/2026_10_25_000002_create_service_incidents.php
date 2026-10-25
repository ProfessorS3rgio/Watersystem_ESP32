<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration {
    public function up(): void
    {
        Schema::create('service_incidents', function (Blueprint $table) {
            $table->id();
            $table->string('reference_number')->unique();
            $table->uuid('client_token')->unique();
            $table->string('request_hash', 64);
            $table->unsignedBigInteger('customer_id')->nullable();
            $table->foreign('customer_id')->references('customer_id')->on('customer')->nullOnDelete();
            $table->string('account_no', 100);
            $table->string('customer_name');
            $table->text('customer_address')->nullable();
            $table->string('barangay')->nullable();
            $table->string('title', 180);
            $table->text('description');
            $table->text('location');
            $table->string('category', 32);
            $table->string('pipe_section', 24)->default('unknown');
            $table->string('priority', 16)->default('normal');
            $table->string('status', 20)->default('reported');
            $table->string('assigned_to', 160)->nullable();
            $table->unsignedBigInteger('reported_by')->nullable();
            $table->foreign('reported_by')->references('id')->on('users')->nullOnDelete();
            $table->string('reported_by_name');
            $table->text('resolution')->nullable();
            $table->timestamp('resolved_at')->nullable();
            $table->timestamp('closed_at')->nullable();
            $table->unsignedInteger('reopened_count')->default(0);
            $table->unsignedInteger('version')->default(1);
            $table->timestamps();
            $table->index(['status', 'priority', 'created_at']);
            $table->index(['customer_id', 'status']);
        });
        Schema::create('incident_events', function (Blueprint $table) {
            $table->id();
            $table->foreignId('incident_id')->constrained('service_incidents')->restrictOnDelete();
            $table->foreignId('actor_id')->nullable()->constrained('users')->nullOnDelete();
            $table->string('actor_name');
            $table->string('kind', 32);
            $table->string('from_status', 20)->nullable();
            $table->string('to_status', 20)->nullable();
            $table->text('message')->nullable();
            $table->json('changes')->nullable();
            $table->timestamp('created_at');
            $table->index(['incident_id', 'id']);
        });
    }

    public function down(): void
    {
        Schema::dropIfExists('incident_events');
        Schema::dropIfExists('service_incidents');
    }
};
