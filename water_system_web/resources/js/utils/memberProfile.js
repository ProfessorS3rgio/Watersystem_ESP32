export const profileFields = ['first_name', 'middle_name', 'last_name', 'name_suffix', 'birthdate', 'phone', 'email', 'meter_number', 'meter_model', 'meter_location', 'meter_installed_on', 'service_started_on', 'notes']
export function fullMemberName(draft) {
  return [draft.first_name, draft.middle_name, draft.last_name, draft.name_suffix].map(value => String(value || '').trim()).filter(Boolean).join(' ')
}
export function memberDraft(data) {
  return { full_name: data.customer.full_name, address: data.customer.address, ...Object.fromEntries(profileFields.map(field => [field, data.profile?.[field] || ''])) }
}
export function memberPayload(draft) {
  return { full_name: fullMemberName(draft) || String(draft.full_name || '').trim(), address: String(draft.address || '').trim(),
    ...Object.fromEntries(profileFields.map(field => [field, String(draft[field] || '').trim() || null])) }
}
export function manilaToday() {
  const parts = Object.fromEntries(new Intl.DateTimeFormat('en', { timeZone:'Asia/Manila', year:'numeric', month:'2-digit', day:'2-digit' }).formatToParts(new Date()).map(part => [part.type, part.value]))
  return `${parts.year}-${parts.month}-${parts.day}`
}
export function memberAge(birthdate, today = manilaToday()) {
  if (!/^\d{4}-\d{2}-\d{2}$/.test(birthdate || '') || birthdate > today) return null
  return Number(today.slice(0, 4)) - Number(birthdate.slice(0, 4)) - (today.slice(5) < birthdate.slice(5) ? 1 : 0)
}
