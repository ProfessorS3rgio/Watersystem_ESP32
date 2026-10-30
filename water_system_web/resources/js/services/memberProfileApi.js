async function request(path, options = {}) {
  const response = await fetch(path, { ...options, credentials: 'same-origin', headers: {
    Accept: 'application/json', ...(options.body ? { 'Content-Type': 'application/json' } : {}),
    'X-CSRF-TOKEN': document.querySelector('meta[name="csrf-token"]')?.getAttribute('content') || '',
  } })
  if (!response.headers.get('content-type')?.includes('application/json')) throw new Error('Your session may have expired. Sign in again and reopen the member profile.')
  const body = await response.json()
  if (!response.ok) {
    const error = new Error(response.status === 401 ? 'Please sign in again to view member profiles.' : body.message || 'Could not save the member profile.')
    error.status = response.status; error.errors = body.errors || {}; throw error
  }
  return body.data
}
export const memberProfileApi = {
  get: (id, signal) => request(`/customers/${id}/profile`, { signal }),
  save: (id, payload) => request(`/customers/${id}/profile`, { method: 'PUT', body: JSON.stringify(payload) }),
}
