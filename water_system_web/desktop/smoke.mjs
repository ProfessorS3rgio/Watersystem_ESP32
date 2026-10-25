import assert from 'node:assert/strict';
import { spawn, spawnSync } from 'node:child_process';
import { mkdtemp, readFile, rm } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { join, resolve } from 'node:path';
import { createServer } from 'node:net';
import { randomBytes } from 'node:crypto';
import { createServer as createViteServer } from 'vite';

const project = resolve(import.meta.dirname, '..');
const backend = join(project, 'src-tauri/resources/backend');
const php = join(project, 'src-tauri/resources/php/php.exe');
const ini = join(project, 'src-tauri/resources/php/php.ini');
const data = await mkdtemp(join(tmpdir(), 'dmbc-desktop-smoke-'));
const reservation = createServer();
await new Promise(resolve => reservation.listen(0, '127.0.0.1', resolve));
const port = reservation.address().port;
const origin = `http://127.0.0.1:${port}`;
const token = randomBytes(32).toString('hex');
const env = { ...process.env, DMBC_DESKTOP_DATA: data, DMBC_DESKTOP_TOKEN: token,
  DB_CONNECTION: 'sqlite', DB_DATABASE: `${data}/dmbc.sqlite`, APP_ENV: 'production', APP_DEBUG: 'false',
  APP_URL: origin, SESSION_DRIVER: 'file', SESSION_COOKIE: 'dmbc_desktop_session', CACHE_STORE: 'file', QUEUE_CONNECTION: 'sync', LOG_CHANNEL: 'single' };
for (const [key, file] of Object.entries({ APP_CONFIG_CACHE: 'config', APP_SERVICES_CACHE: 'services', APP_PACKAGES_CACHE: 'packages', APP_ROUTES_CACHE: 'routes', APP_EVENTS_CACHE: 'events' })) env[key] = join(data, `cache/${file}.php`);
let server;
let vite;
const originalFetch = globalThis.fetch;
const originalDocument = globalThis.document;
let output = '';
try {
  const initialize = () => {
    const result = spawnSync(php, ['-c', ini, 'desktop/initialize.php'], { cwd: backend, env, encoding: 'utf8', windowsHide: true, timeout: 60000 });
    assert.equal(result.status, 0, result.stdout + result.stderr);
    assert.match(result.stdout, /SQLite profile ready/);
  };
  initialize();
  const credentials = await readFile(join(data, 'initial-login.txt'), 'utf8');
  const key = await readFile(join(data, 'application.key'), 'utf8');
  initialize();
  assert.equal(await readFile(join(data, 'initial-login.txt'), 'utf8'), credentials);
  assert.equal(await readFile(join(data, 'application.key'), 'utf8'), key);
  await new Promise(resolve => reservation.close(resolve));
  server = spawn(php, ['-c', ini, '-S', `127.0.0.1:${port}`, '-t', 'public', 'desktop/router.php'], { cwd: backend, env, windowsHide: true });
  server.stdout.on('data', chunk => output += chunk);
  server.stderr.on('data', chunk => output += chunk);
  let ready = false;
  for (let attempt = 0; attempt < 100; attempt++) {
    try { assert.equal((await fetch(origin)).status, 403); ready = true; break; } catch { await new Promise(resolve => setTimeout(resolve, 100)); }
  }
  assert.ok(ready, output);
  const launch = await fetch(`${origin}/desktop-launch/${token}`, { redirect: 'manual' });
  assert.equal(launch.status, 302, await launch.text());
  let cookie = launch.headers.getSetCookie().map(value => value.split(';')[0]).join('; ');
  const headers = () => ({ Cookie: cookie, Accept: 'application/json', Origin: origin });
  const home = await fetch(origin, { headers: { Cookie: cookie } });
  const html = await home.text();
  assert.equal(home.status, 200, html);
  assert.match(html, /build\/assets/);
  assert.doesNotMatch(html, /@vite\/client/);
  const denied = await fetch(`${origin}/user`, { headers: { ...headers(), Origin: 'https://example.com' } });
  assert.equal(denied.status, 403);
  const csrf = await fetch(`${origin}/csrf-token`, { headers: headers() });
  assert.equal(csrf.status, 200);
  const csrfBody = await csrf.json();
  cookie += '; ' + csrf.headers.getSetCookie().map(value => value.split(';')[0]).join('; ');
  const password = credentials.match(/Password: (\w+)/)[1];
  // Exercise the actual Vue form method and its request headers, rather than a
  // hand-written login request that can accidentally conceal frontend failures.
  let csrfToken = csrfBody.csrf_token;
  globalThis.document = { createElement: () => ({}), querySelector: () => ({ getAttribute: () => csrfToken, setAttribute: (_name, value) => { csrfToken = value; } }) };
  globalThis.fetch = async (path, options = {}) => {
    const response = await originalFetch(new URL(path, origin), { ...options, headers: { Cookie: cookie, Origin: origin, ...options.headers } });
    const jar = new Map(cookie.split('; ').filter(Boolean).map(value => { const i = value.indexOf('='); return [value.slice(0, i), value.slice(i + 1)]; }));
    for (const value of response.headers.getSetCookie()) { const pair = value.split(';')[0], i = pair.indexOf('='); jar.set(pair.slice(0, i), pair.slice(i + 1)); }
    cookie = [...jar].map(([name, value]) => `${name}=${value}`).join('; ');
    return response;
  };
  vite = await createViteServer({ configFile: false, plugins: [(await import('@vitejs/plugin-vue')).default()], server: { middlewareMode: true, hmr: false }, appType: 'custom' });
  const { default: Login } = await vite.ssrLoadModule('/resources/js/pages/Login.vue');
  const { checkAuthentication } = await import('../resources/js/services/authApi.js');
  assert.equal(await checkAuthentication(), false);
  let destination = null;
  const form = { form: { email: 'admin@localhost', password: 'admin' }, loading: false, error: null, $router: { push: async path => { destination = path; } } };
  await Login.methods.handleLogin.call(form);
  assert.match(form.error, /credentials are incorrect/);
  assert.equal(destination, null);
  form.form = { email: 'admin@dmbc.local', password };
  await Login.methods.handleLogin.call(form);
  assert.equal(form.error, null);
  assert.equal(destination, '/');
  assert.equal(form.loading, false);
  assert.equal(await checkAuthentication(), true);
  for (const endpoint of ['/user', '/settings', '/customer-types', '/deductions', '/barangays', '/customers', '/dashboard', '/audit-logs', '/bills', '/bills/stats', '/bills/transactions', '/bills/today-payments', '/bills/monthly-payments', '/bills/monthly-collectable', '/backup/restore-points']) {
    const response = await fetch(origin + endpoint, { headers: headers() });
    assert.equal(response.status, 200, `${endpoint}: ${await response.text()}`);
  }
  const backup = await fetch(`${origin}/backup/export`, { headers: headers() });
  assert.equal(backup.status, 200, await backup.clone().text());
  const archive = Buffer.from(await backup.arrayBuffer());
  assert.equal(archive.subarray(0, 2).toString(), 'PK');

  const policy = (await (await fetch('/settings')).json()).data;
  const changes = { bill_due_days: 12, disconnection_days: 20, penalty_after_days: 3, penalty_amount: 25, penalty_enabled: false };
  const save = await fetch('/settings', { method: 'PUT', headers: { Accept: 'application/json', 'Content-Type': 'application/json', 'X-CSRF-TOKEN': csrfToken }, body: JSON.stringify(changes) });
  assert.equal(save.status, 200, await save.text());
  assert.equal((await (await fetch('/settings')).json()).data.disconnection_days, 20);
  const categories = (await (await fetch('/customer-types')).json()).data;
  assert.ok(categories.length > 0);
  const customerResponse = await fetch('/customers', { method: 'POST', headers: { Accept: 'application/json', 'Content-Type': 'application/json', 'X-CSRF-TOKEN': csrfToken }, body: JSON.stringify({ customer_name: 'Desktop test member', type_id: categories[0].type_id, brgy_id: 1, address: 'Makilas', previous_reading: 0, status: 'active' }) });
  assert.equal(customerResponse.status, 201, await customerResponse.text());
  const members = (await (await fetch('/customers')).json()).data;
  assert.equal(members[0].customer_name, 'Desktop test member');
  const jsonHeaders = { Accept:'application/json', 'Content-Type':'application/json', 'X-CSRF-TOKEN':csrfToken };
  const localDate = new Date().toLocaleString('sv-SE', { timeZone:'Asia/Manila' });
  const batch = {
    device: {device_uid:'DESKTOP-VERIFY',brgy_id:1,firmware_version:'v1.1.0',verified_sync_ack:1},
    readings:[{reading_id:1,device_uid:'DESKTOP-VERIFY',customer_account_number:members[0].account_no,previous_reading:0,current_reading:10,usage_m3:10,reading_at:Math.floor(Date.now()/1000)}],
    bills:[{bill_id:1,reading_id:1,device_uid:'DESKTOP-VERIFY',customer_account_number:members[0].account_no,reference_number:'DESKTOP-VERIFY-BILL',bill_date:localDate.slice(0,10),rate_per_m3:10,charges:100,penalty:0,total_due:100,status:'Paid'}],
    transactions:[{bill_transaction_id:1,bill_reference_number:'DESKTOP-VERIFY-BILL',processed_by_device_uid:'DESKTOP-VERIFY',type:'payment',source:'Device',amount:100,cash_received:100,change:0,transaction_date:localDate,payment_method:'cash'}],
  };
  const staged = await fetch('/sync-reviews',{method:'POST',headers:jsonHeaders,body:JSON.stringify(batch)});
  assert.equal(staged.status,201,await staged.clone().text());
  const review = (await staged.json()).review;
  assert.equal(review.preview.can_approve,true);
  for (let attempt=0;attempt<2;attempt++) {
    const approval = await fetch(`/sync-reviews/${review.id}/approve`,{method:'POST',headers:jsonHeaders,body:JSON.stringify({device_uid:'DESKTOP-VERIFY',accept_warnings:false})});
    assert.equal(approval.status,200,await approval.clone().text());
    assert.equal((await approval.json()).receipt.review_id,review.id);
  }
  const confirmation = await fetch(`/sync-reviews/${review.id}/acknowledged`,{method:'POST',headers:jsonHeaders});
  assert.equal(confirmation.status,200);
  // Generate real workbooks through the same export flow used by the UI.
  // The native save bridge is captured here; Rust tests cover writing files.
  const { useMonthlyReportExport } = await vite.ssrLoadModule('/resources/js/composables/useMonthlyReportExport.js');
  const reportExport = useMonthlyReportExport({ desktop: true, save: async ({ filename, bytes }) => {
    assert.ok(filename.startsWith('monthly-billing-report-') && filename.endsWith('.xlsx'));
    const workbook = Buffer.from(bytes);
    assert.equal(workbook.subarray(0,4).toString('hex'), '504b0304');
    assert.ok(workbook.includes(Buffer.from('xl/workbook.xml')));
    return [join(data, filename), true];
  } });
  for (const barangayId of ['', '1']) {
    await reportExport.downloadMonthlyReport({ month: '2026-10', barangayId });
    assert.equal(reportExport.reportExportError.value, '');
    assert.match(reportExport.reportExportResult.value, /File Explorer opened/);
    assert.equal(reportExport.exportingReport.value, false);
  }
  const { incidentApi, incidentToken } = await import('../resources/js/services/incidentApi.js');
  const incidentPayload = { client_token: incidentToken(), customer_id: members[0].id || members[0].customer_id, title: 'Test leak at the meter', description: 'Water leaking at the test household connection.', location: 'Beside the test meter', category: 'leak', priority: 'urgent', pipe_section: 'meter', assigned_to: 'Test repair crew' };
  const incident = (await incidentApi.create(incidentPayload)).data;
  assert.equal((await incidentApi.create(incidentPayload)).data.id, incident.id, 'retrying a saved submission must not duplicate the report');
  assert.equal((await incidentApi.customers({ id: incidentPayload.customer_id })).data[0].open_incidents_count, 1);
  let repaired = (await incidentApi.update(incident.id, { version: 1, status: 'in_progress', note: 'Inspection started.' })).data;
  await assert.rejects(incidentApi.update(incident.id, { version: 1, priority: 'normal' }), error => error.status === 409);
  repaired = (await incidentApi.update(incident.id, { version: repaired.version, status: 'resolved', resolution: 'Replaced the fitting and checked for leaks.' })).data;
  repaired = (await incidentApi.update(incident.id, { version: repaired.version, status: 'closed' })).data;
  assert.equal((await incidentApi.list({ status: 'completed' })).total, 1);
  repaired = (await incidentApi.update(incident.id, { version: repaired.version, status: 'in_progress', note: 'Leak returned during the next inspection.' })).data;
  assert.equal(repaired.resolution, null);
  assert.equal(repaired.reopened_count, 1);
  const snapshot = await fetch('/backup/restore-points', { method: 'POST', headers: { Accept: 'application/json', 'Content-Type': 'application/json', 'X-CSRF-TOKEN': csrfToken }, body: JSON.stringify({ label: 'Desktop smoke snapshot', source: 'manual' }) });
  assert.equal(snapshot.status, 201);
  const snapshotId = (await snapshot.json()).id;
  repaired = (await incidentApi.update(incident.id, { version: repaired.version, note: 'This progress note was written after the snapshot.' })).data;
  const restore = await fetch(`/backup/restore-points/${snapshotId}/restore`, { method: 'POST', headers: { Accept: 'application/json', 'X-CSRF-TOKEN': csrfToken } });
  assert.equal(restore.status, 200, await restore.text());
  assert.equal((await incidentApi.detail(incident.id)).data.version, repaired.version, 'snapshot restores must preserve later incident progress');
  assert.equal((await (await fetch('/sync-reviews')).json()).reviews[0].id,review.id,'restore preserves verification receipts');

  await new Promise(resolve => { server.once('exit', resolve); server.kill(); });
  initialize();
  server = spawn(php, ['-c', ini, '-S', `127.0.0.1:${port}`, '-t', 'public', 'desktop/router.php'], { cwd: backend, env, windowsHide: true });
  server.stdout.on('data', chunk => output += chunk); server.stderr.on('data', chunk => output += chunk);
  for (let attempt = 0; attempt < 100; attempt++) { try { await fetch(origin); break; } catch { await new Promise(resolve => setTimeout(resolve, 100)); } }
  assert.equal(await checkAuthentication(), true, 'login session survives backend restart');
  assert.equal((await (await fetch('/settings')).json()).data.bill_due_days, 12);
  assert.equal((await (await fetch('/customers')).json()).data[0].customer_name, 'Desktop test member');
  assert.equal((await incidentApi.detail(incident.id)).data.events[0].message, 'This progress note was written after the snapshot.');
  const logout = await fetch('/logout', { method: 'POST', headers: { Accept: 'application/json', 'X-CSRF-TOKEN': csrfToken } });
  assert.equal(logout.status, 200);
  assert.equal(await checkAuthentication(), false);
  // A stale page token after logout should be refreshed once during sign-in.
  await Login.methods.handleLogin.call(form);
  assert.equal(form.error, null);
  assert.equal(await checkAuthentication(), true);
  console.log('Desktop smoke checks passed: login, logout/re-login, restart persistence, customer creation, settings save, snapshot restore, ZIP export, all-barangay and selected-barangay Excel export, batch review/approval/retry/receipt preservation, incident report retries, repair/close/reopen workflow, stale edit conflicts, and incident progress retained across restore/restart. All writes used a temporary test profile.');
} finally {
  globalThis.fetch = originalFetch;
  globalThis.document = originalDocument;
  if (vite) await vite.close();
  if (server && server.exitCode === null) { server.kill(); await new Promise(resolve => server.once('exit', resolve)); }
  if (reservation.listening) await new Promise(resolve => reservation.close(resolve));
  // Only the unique temporary test profile created above is removed.
  await rm(data, { recursive: true, force: true });
}
