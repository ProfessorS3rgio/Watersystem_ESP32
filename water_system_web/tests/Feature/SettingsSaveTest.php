<?php

namespace Tests\Feature;

use App\Models\AuditLog;
use App\Models\Setting;
use App\Models\User;
use App\Services\AuditLogService;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;
use Tests\TestCase;

class SettingsSaveTest extends TestCase
{
    protected function setUp(): void
    {
        parent::setUp();
        Schema::create('settings', function (Blueprint $table) {
            $table->id();
            $table->integer('bill_due_days');
            $table->integer('disconnection_days');
            $table->integer('penalty_after_days');
            $table->decimal('penalty_amount', 10, 2);
            $table->boolean('penalty_enabled');
            $table->boolean('Synced')->default(true);
            $table->timestamp('last_sync')->nullable();
            $table->timestamps();
        });
    }

    public function test_saving_persists_disconnection_days_and_marks_policy_for_device_sync(): void
    {
        $this->mock(AuditLogService::class)->shouldReceive('record')->once()->andReturn(new AuditLog);
        $payload = ['bill_due_days' => 5, 'disconnection_days' => 12, 'penalty_after_days' => 0, 'penalty_amount' => 20, 'penalty_enabled' => false];
        Setting::create($payload);
        $this->actingAs(User::factory()->make())->putJson('/settings', [...$payload, 'disconnection_days' => 15])
            ->assertOk()->assertJsonPath('data.disconnection_days', 15)->assertJsonPath('data.penalty_enabled', false);
        $this->assertDatabaseHas('settings', ['disconnection_days' => 15, 'Synced' => false]);
    }

    public function test_invalid_policy_is_not_saved(): void
    {
        $payload = ['bill_due_days' => 5, 'disconnection_days' => 12, 'penalty_after_days' => 0, 'penalty_amount' => 20, 'penalty_enabled' => true];
        Setting::create($payload);
        $this->actingAs(User::factory()->make())->putJson('/settings', [...$payload, 'bill_due_days' => 366])
            ->assertUnprocessable()->assertJsonValidationErrors('bill_due_days');
        $this->assertDatabaseHas('settings', ['bill_due_days' => 5, 'Synced' => true]);
    }
}
