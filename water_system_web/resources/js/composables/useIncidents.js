import { ref, reactive, watch, onBeforeUnmount } from 'vue';
import { incidentApi } from '../services/incidentApi.js';

export function useIncidents() {
  const filters = reactive({ q: '', status: 'open', priority: '', category: '', customer_id: '' });
  const items = ref([]), summary = ref({ open: 0, urgent: 0, in_progress: 0, resolved: 0 });
  const options = ref({ categories: {}, statuses: {}, priorities: {}, sections: {} });
  const pagination = ref({ current_page: 1, last_page: 1, total: 0 });
  const loading = ref(false), error = ref('');
  let controller, revision = 0, timer;
  async function load(page = 1) {
    clearTimeout(timer); controller?.abort(); controller = new AbortController();
    const current = ++revision; loading.value = true; error.value = '';
    try {
      const data = await incidentApi.list({ ...filters, page }, controller.signal);
      if (current !== revision) return;
      if (data.total && data.current_page > data.last_page) return await load(data.last_page);
      items.value = data.data; options.value = data.options; summary.value = data.summary;
      pagination.value = { current_page: data.current_page, last_page: data.last_page, total: data.total };
    } catch (err) { if (current === revision && err.name !== 'AbortError') error.value = err.message; }
    finally { if (current === revision) loading.value = false; }
  }
  watch(filters, () => { clearTimeout(timer); controller?.abort(); ++revision; loading.value = true; timer = setTimeout(() => load(1), 300); }, { deep: true });
  onBeforeUnmount(() => { clearTimeout(timer); controller?.abort(); ++revision; });
  return { filters, items, summary, options, pagination, loading, error, load };
}
