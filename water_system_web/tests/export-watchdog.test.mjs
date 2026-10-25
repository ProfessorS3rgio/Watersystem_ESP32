import test from 'node:test';
import assert from 'node:assert/strict';
import { createExportWatchdog } from '../resources/js/utils/exportWatchdog.js';

function fakeClock() {
  let now = 0, nextId = 0;
  const timers = new Map();
  return {
    setTimeout(callback, delay) { const id = ++nextId; timers.set(id, { at: now + delay, callback }); return id; },
    clearTimeout(id) { timers.delete(id); },
    advance(ms) { now += ms; for (const [id, timer] of [...timers]) if (timer.at <= now) { timers.delete(id); timer.callback(); } },
    get pending() { return timers.size; },
  };
}

test('an export can run beyond a minute while chunks keep arriving', () => {
  const clock = fakeClock(), errors = [];
  const watchdog = createExportWatchdog(error => errors.push(error), clock);
  watchdog.start(60000, 'Export stalled');
  for (let chunk = 0; chunk < 150; chunk++) { clock.advance(1000); watchdog.progress(); }
  assert.equal(errors.length, 0);
  assert.equal(clock.pending, 1);
  clock.advance(59999); assert.equal(errors.length, 0);
  clock.advance(1); assert.equal(errors[0].message, 'Export stalled');
  assert.equal(clock.pending, 0);
});

test('completed or failed exports stop their timer and cannot rearm on late lines', () => {
  const clock = fakeClock(), errors = [];
  const watchdog = createExportWatchdog(error => errors.push(error), clock);
  watchdog.start(15000, 'Query stalled'); watchdog.stop(); watchdog.progress(); clock.advance(30000);
  assert.equal(errors.length, 0); assert.equal(clock.pending, 0);
  watchdog.start(15000, 'Next query stalled'); clock.advance(15000);
  assert.equal(errors.length, 1);
  assert.equal(errors[0].message, 'Next query stalled');
});
