<?php

namespace Tests\Feature;

use App\Models\User;
use Illuminate\Support\Facades\Http;
use Tests\TestCase;

class WirelessSyncTest extends TestCase
{
    public function test_pairing_validates_the_device_before_storing_the_code(): void
    {
        Http::preventStrayRequests();
        Http::fake(['http://192.168.4.1/sync/hello' => Http::response(['service' => 'DMBC_SYNC', 'protocol' => 1, 'device_id' => 2])]);
        $this->actingAs(User::factory()->make())->postJson('/device-link/wifi/connect', ['token' => 'abcd1234'])
            ->assertOk()->assertJsonPath('device_id', 2)->assertSessionHas('device_wifi_token', 'ABCD1234');
        Http::assertSent(fn ($request) => $request->hasHeader('X-DMBC-Token', 'ABCD1234'));
    }

    public function test_invalid_pairing_cannot_open_a_session(): void
    {
        Http::fake(['*' => Http::response('Rejected', 401)]);
        $this->actingAs(User::factory()->make())->postJson('/device-link/wifi/connect', ['token' => 'ABCD1234'])
            ->assertStatus(422)->assertSessionMissing('device_wifi_token');
    }

    public function test_only_allowlisted_commands_are_relayed(): void
    {
        Http::preventStrayRequests();
        Http::fake(['http://192.168.4.1/sync/command' => Http::response("ACK|SET_TIME|123\n", 200, ['Content-Type' => 'text/plain'])]);
        $this->actingAs(User::factory()->make())->withSession(['device_wifi_token' => 'ABCD1234'])
            ->postJson('/device-link/wifi/command', ['command' => 'SET_TIME|123'])->assertOk()->assertContent("ACK|SET_TIME|123\n");
        $this->postJson('/device-link/wifi/command', ['command' => "EXPORT_BILLS\nDROP_DB"])->assertStatus(422);
        $this->postJson('/device-link/wifi/command', ['command' => 'FORMAT_SD'])->assertStatus(422);
        Http::assertSentCount(1);
    }

    public function test_missing_session_prevents_contacting_the_device(): void
    {
        Http::fake();
        $this->actingAs(User::factory()->make())->postJson('/device-link/wifi/command', ['command' => 'EXPORT_BILLS'])->assertStatus(409);
        Http::assertNothingSent();
    }

    public function test_lost_connection_expires_the_session(): void
    {
        Http::fake(['*' => Http::failedConnection()]);
        $this->actingAs(User::factory()->make())->withSession(['device_wifi_token' => 'ABCD1234'])
            ->postJson('/device-link/wifi/command', ['command' => 'EXPORT_BILLS'])->assertStatus(503)->assertSessionMissing('device_wifi_token');
    }

    public function test_relay_requires_login(): void
    {
        Http::fake();
        $this->postJson('/device-link/wifi/connect', ['token' => 'ABCD1234'])->assertUnauthorized();
        Http::assertNothingSent();
    }
}
