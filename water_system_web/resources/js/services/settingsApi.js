export async function settingsRequest(path, options = {}) {
  const response = await fetch(path, {
    ...options,
    credentials: 'same-origin',
    headers: {
      Accept: 'application/json',
      ...(options.body ? { 'Content-Type': 'application/json' } : {}),
      'X-CSRF-TOKEN': document.querySelector('meta[name="csrf-token"]')?.getAttribute('content') || '',
      ...options.headers,
    },
  })
  if (!response.ok) {
    const data = await response.json().catch(() => ({}))
    const error = new Error(response.status === 401 ? 'Your session has expired. Please sign in again.' : Object.values(data.errors || {}).flat().join(' ') || data.message || 'Unable to complete this request. Please try again.')
    error.status = response.status
    throw error
  }
  return response
}

export async function settingsJson(path, options) {
  return (await settingsRequest(path, options)).json()
}
