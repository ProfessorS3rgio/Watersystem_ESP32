<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    public function up(): void
    {
        Schema::create('backup_snapshots', function (Blueprint $table) {
            $table->id();
            $table->string('label');
            $table->string('source')->default('sync');
            $table->foreignId('created_by')->nullable()->constrained('users')->nullOnDelete();
            $table->unsignedInteger('table_count')->default(0);
            $table->unsignedBigInteger('row_count')->default(0);
            $table->longText('payload');
            $table->timestamps();
        });
    }

    public function down(): void
    {
        Schema::dropIfExists('backup_snapshots');
    }
};
