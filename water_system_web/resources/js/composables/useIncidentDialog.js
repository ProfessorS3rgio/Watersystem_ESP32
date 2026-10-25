import { onMounted, onBeforeUnmount, nextTick } from 'vue';

export function useIncidentDialog(root) {
  let previous;
  function trap(event) {
    if (event.key !== 'Tab') return;
    const elements = [...root.value.querySelectorAll('button:not(:disabled),input:not(:disabled),select:not(:disabled),textarea:not(:disabled),a[href],[tabindex="0"]')].filter(item => item.getClientRects().length);
    if (!elements.length) { event.preventDefault(); return; }
    const first = elements[0], last = elements.at(-1);
    if (event.shiftKey && (document.activeElement === first || document.activeElement === root.value)) { event.preventDefault(); last.focus(); }
    else if (!event.shiftKey && document.activeElement === last) { event.preventDefault(); first.focus(); }
  }
  onMounted(async () => { previous = document.activeElement; await nextTick(); root.value?.focus(); root.value?.addEventListener('keydown', trap); });
  onBeforeUnmount(() => { root.value?.removeEventListener('keydown', trap); if (previous?.isConnected) previous.focus(); });
}
