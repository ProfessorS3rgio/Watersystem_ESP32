<?php

namespace Tests\Feature;

use App\Models\User;
use Tests\TestCase;

class DesktopAuthTest extends TestCase
{
    protected function setUp(): void
    {
        parent::setUp();
        (require database_path('migrations/0001_01_01_000000_create_users_table.php'))->up();
        User::factory()->create(['email' => 'admin@dmbc.local']);
    }

    public function test_login_returns_the_current_session_token_and_logout_revokes_authentication(): void
    {
        $response = $this->postJson('/login', ['email' => 'admin@dmbc.local', 'password' => 'password'])->assertOk()->assertJsonPath('user.email', 'admin@dmbc.local');
        $this->assertNotEmpty($response->json('csrf_token'));
        $this->assertSame(session()->token(), $response->json('csrf_token'));
        $this->getJson('/user')->assertOk()->assertJsonPath('user.email', 'admin@dmbc.local');
        $this->postJson('/logout')->assertOk();
        $this->getJson('/user')->assertUnauthorized();
    }

    public function test_wrong_web_credentials_have_a_clear_json_validation_response(): void
    {
        $this->postJson('/login', ['email' => 'admin@localhost', 'password' => 'admin'])->assertUnprocessable()->assertJsonValidationErrors('email');
    }

    public function test_html_guest_requests_redirect_to_the_hash_login_page(): void
    {
        $this->get('/user')->assertRedirect('/login');
        $this->get('/login')->assertRedirect('/#/login');
    }
}
