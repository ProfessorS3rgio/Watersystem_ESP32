<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\DB;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    public function up(): void
    {
        Schema::table('bill', function (Blueprint $table) {
            $table->decimal('amount_paid', 10, 2)->default(0)->after('total_due');
            $table->decimal('balance_due', 10, 2)->default(0)->after('amount_paid');
        });

        Schema::table('bill_transaction', function (Blueprint $table) {
            $table->unsignedBigInteger('bill_id')->nullable()->after('bill_transaction_id');
            $table->index('bill_id', 'bill_transaction_bill_id_index');
            $table->foreign('bill_id', 'bill_transaction_bill_id_foreign')
                ->references('bill_id')
                ->on('bill');
        });

        // Populate only the newly added derived balance column.
        DB::table('bill')->update([
            'balance_due' => DB::raw("CASE WHEN status = 'Paid' THEN 0 ELSE total_due END"),
        ]);

        // Add the new enum value without changing existing bill statuses.
        if (DB::connection()->getDriverName() === 'mysql') {
            DB::statement("ALTER TABLE bill MODIFY status ENUM('Paid', 'Partially Paid', 'Void', 'Pending', 'Due') NOT NULL DEFAULT 'Pending'");
        }
    }

    public function down(): void
    {
        if (Schema::hasTable('bill_transaction')) {
            Schema::table('bill_transaction', function (Blueprint $table) {
                $table->dropForeign('bill_transaction_bill_id_foreign');
                $table->dropIndex('bill_transaction_bill_id_index');
                $table->dropColumn('bill_id');
            });
        }

        Schema::table('bill', function (Blueprint $table) {
            $table->dropColumn(['amount_paid', 'balance_due']);
        });
    }
};
