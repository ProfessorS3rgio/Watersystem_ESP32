// Bulk USB exports may exceed a minute while still making steady progress.
// Bound the idle wait rather than the entire export duration.
export function createExportWatchdog(onTimeout, clock = { setTimeout: globalThis.setTimeout, clearTimeout: globalThis.clearTimeout }) {
  let timer = null, duration = 0, message = '';
  function stop() { if (timer !== null) clock.clearTimeout(timer); timer = null; }
  function arm() {
    stop();
    timer = clock.setTimeout(() => { timer = null; onTimeout(new Error(message)); }, duration);
  }
  return {
    start(ms, text) { duration = ms; message = text; arm(); },
    progress() { if (timer !== null) arm(); },
    stop,
  };
}
