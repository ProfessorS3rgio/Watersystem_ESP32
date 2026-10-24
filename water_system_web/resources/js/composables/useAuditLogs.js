import { ref, reactive, computed, watch, onMounted, onBeforeUnmount } from 'vue'

export function useAuditLogs() {
  const filters = reactive({ q: '', action: '', auditable_type: '', actor_type: '', date_from: '', date_to: '', sync_run_id: '', request_id: '' })
  const logs = ref([]), loading = ref(false), error = ref(''), snapshotId = ref(null), refreshedAt = ref(null)
  const pagination = ref({ current_page: 1, last_page: 1, total: 0, from: 0, to: 0 })
  const options = ref({ actions: [], types: [] })
  let controller, timer, revision = 0
  const activeFilters = computed(() => Object.values(filters).filter(Boolean).length)
  const pageNumbers = computed(() => {
    const first = Math.max(1, Math.min(pagination.value.current_page - 2, pagination.value.last_page - 4))
    return Array.from({ length: Math.min(5, pagination.value.last_page - first + 1) }, (_, i) => first + i)
  })
  async function load(page = 1, fresh = false) {
    clearTimeout(timer)
    controller?.abort()
    const current = ++revision
    controller = new AbortController()
    if (fresh) snapshotId.value = null
    loading.value = true; error.value = ''
    const params = new URLSearchParams({ page: String(page) })
    for (const [key, value] of Object.entries(filters)) if (value) params.set(key, value)
    if (snapshotId.value !== null) params.set('snapshot_id', String(snapshotId.value))
    try {
      const response = await fetch('/audit-logs?' + params, { credentials: 'same-origin', headers: { Accept: 'application/json' }, signal: controller.signal })
      const data = await response.json()
      if (!response.ok) throw new Error(Object.values(data.errors || {}).flat().join(' ') || data.message || 'Unable to load audit history.')
      if (current !== revision) return
      logs.value = data.data || []
      pagination.value = { current_page: data.current_page, last_page: data.last_page, from: data.from || 0, to: data.to || 0, total: data.total }
      options.value = data.filter_options || { actions: [], types: [] }
      snapshotId.value = data.snapshot_id
      if (fresh || !refreshedAt.value) refreshedAt.value = new Date().toISOString()
    } catch (err) { if (current === revision && err.name !== 'AbortError') error.value = err.message } finally { if (current === revision) loading.value = false }
  }
  function clearFilters() { for (const key of Object.keys(filters)) filters[key] = '' }
  function relatedEvents({ key, value }) { clearFilters(); filters[key] = value }
  watch(filters, () => {
    controller?.abort(); revision++; clearTimeout(timer)
    loading.value = true
    timer = setTimeout(() => load(1, true), 300)
  })
  onMounted(() => load(1, true))
  onBeforeUnmount(() => { revision++; clearTimeout(timer); controller?.abort() })
  return { filters, logs, loading, error, options, pagination, pageNumbers, activeFilters, refreshedAt, load, clearFilters, relatedEvents }
}
