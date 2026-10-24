import { ref, computed, onMounted, watch } from 'vue'

const clone = value => JSON.parse(JSON.stringify(value))

// Each settings panel owns its draft, confirmed baseline, and request state.
export function useEditableSettings({ section, initial, load, persist, emit }) {
  const form = ref(clone(initial))
  const baseline = ref(clone(initial))
  const loading = ref(false)
  const saving = ref(false)
  const ready = ref(false)
  const error = ref('')
  const message = ref('')
  const dirty = computed(() => ready.value && JSON.stringify(form.value) !== JSON.stringify(baseline.value))
  watch([dirty, saving], () => emit('state-change', { section, dirty: dirty.value, busy: saving.value }), { immediate: true })

  async function reload() {
    if (saving.value || loading.value) return
    loading.value = true
    error.value = ''
    try {
      const data = await load()
      form.value = clone(data)
      baseline.value = clone(data)
      ready.value = true
    } catch (err) { error.value = err.message } finally { loading.value = false }
  }

  function discard() {
    if (saving.value) return
    form.value = clone(baseline.value)
    error.value = ''
    message.value = ''
  }

  async function save() {
    if (!ready.value || saving.value || !dirty.value) return
    saving.value = true
    error.value = ''
    message.value = ''
    try {
      const result = await persist(clone(form.value))
      form.value = clone(result)
      baseline.value = clone(result)
      message.value = 'Changes saved successfully.'
    } catch (err) { error.value = err.message } finally { saving.value = false }
  }

  onMounted(reload)
  return { form, baseline, loading, saving, ready, dirty, error, message, reload, discard, save }
}
