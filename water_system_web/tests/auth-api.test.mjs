import test from 'node:test';
import assert from 'node:assert/strict';
import { signIn, checkAuthentication } from '../resources/js/services/authApi.js';

function environment(t, handler) {
  const originalFetch = globalThis.fetch, originalDocument = globalThis.document;
  let token = 'initial-token';
  globalThis.document = { querySelector: () => ({ getAttribute: () => token, setAttribute: (_name, value) => { token = value; } }) };
  globalThis.fetch = handler;
  t.after(() => { globalThis.fetch = originalFetch; globalThis.document = originalDocument; });
  return () => token;
}

test('invalid credentials are requested as JSON and show the server validation message', async t => {
  environment(t, async (path, options) => {
    assert.equal(path, '/login');
    assert.equal(options.credentials, 'same-origin');
    assert.equal(options.headers.Accept, 'application/json');
    assert.equal(JSON.parse(options.body).email, 'admin@dmbc.local');
    return Response.json({ message: 'Invalid input', errors: { email: ['The provided credentials are incorrect.'] } }, { status: 422 });
  });
  await assert.rejects(signIn({ email: ' admin@dmbc.local ', password: 'test' }), /credentials are incorrect/);
});

test('successful login updates the page token for subsequent settings and payment requests', async t => {
  const token = environment(t, async () => Response.json({ user: { id: 1 }, csrf_token: 'new-session-token' }));
  assert.deepEqual(await signIn({ email: 'admin@dmbc.local', password: 'test' }), { id: 1 });
  assert.equal(token(), 'new-session-token');
});

test('a stale token after logout is refreshed once before retrying sign-in', async t => {
  let attempts = 0;
  const token = environment(t, async (path, options) => {
    if (path === '/csrf-token') return Response.json({ csrf_token: 'refreshed-token' });
    attempts++;
    if (attempts === 1) return Response.json({ message: 'CSRF token mismatch.' }, { status: 419 });
    assert.equal(options.headers['X-CSRF-TOKEN'], 'refreshed-token');
    return Response.json({ user: { id: 1 }, csrf_token: 'signed-in-token' });
  });
  await signIn({ email: 'admin@dmbc.local', password: 'test' });
  assert.equal(attempts, 2);
  assert.equal(token(), 'signed-in-token');
});

test('HTML redirects are not accepted as a logged-in user', async t => {
  environment(t, async () => new Response('<html>Login</html>'));
  assert.equal(await checkAuthentication(), false);
  await assert.rejects(signIn({ email: 'admin@dmbc.local', password: 'test' }), /unexpected sign-in response/);
});
