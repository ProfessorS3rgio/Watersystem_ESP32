import { ref, computed } from 'vue'
import { memberProfileApi } from '../services/memberProfileApi.js'
import { memberDraft, memberPayload } from '../utils/memberProfile.js'

export function useMemberProfile(api = memberProfileApi) {
  const record = ref(null), draft = ref({}), loading = ref(false), saving = ref(false), error = ref(''), errors = ref({}), saved = ref(false)
  let original = '', generation = 0, memberId, controller
  const dirty = computed(() => Boolean(record.value) && JSON.stringify(memberPayload(draft.value)) !== original)
  function accept(data) { record.value = data; draft.value = memberDraft(data); original = JSON.stringify(memberPayload(draft.value)) }
  async function load(id) {
    controller?.abort(); controller = new AbortController()
    const run = ++generation; memberId = id
    record.value = null; error.value = ''; errors.value = {}; saved.value = false; loading.value = true
    try { const data = await api.get(id, controller.signal); if (run === generation) accept(data) }
    catch (failure) { if (run === generation && failure.name !== 'AbortError') error.value = failure.message }
    finally { if (run === generation) loading.value = false }
  }
  async function save() {
    if (saving.value || !record.value || !dirty.value) return
    const run = generation; saving.value = true; error.value = ''; errors.value = {}; saved.value = false
    try {
      const data = await api.save(memberId, { ...memberPayload(draft.value), revision: record.value.revision })
      if (run === generation) { accept(data); saved.value = true }
    } catch (failure) {
      if (run === generation) { error.value = failure.message; errors.value = failure.errors || {} }
    } finally { saving.value = false }
  }
  function discard() { if (record.value) accept(record.value); saved.value = false; error.value = ''; errors.value = {} }
  function dispose() { ++generation; controller?.abort() }
  return { record, draft, loading, saving, error, errors, saved, dirty, load, save, discard, dispose }
}
