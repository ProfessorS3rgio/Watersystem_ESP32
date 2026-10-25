function csrfToken() {
  return document.querySelector('meta[name="csrf-token"]')?.getAttribute('content') || '';
}

export async function signIn(form) {
  const request = () => fetch('/login', {
    method: 'POST', credentials: 'same-origin',
    headers: { Accept: 'application/json', 'Content-Type': 'application/json', 'X-CSRF-TOKEN': csrfToken() },
    body: JSON.stringify({ email: form.email.trim(), password: form.password }),
  });
  let response = await request();
  if (response.status === 419) {
    const refresh = await fetch('/csrf-token', { credentials: 'same-origin', headers: { Accept: 'application/json' } });
    if (refresh.ok) {
      const token = (await refresh.json()).csrf_token;
      document.querySelector('meta[name="csrf-token"]')?.setAttribute('content', token);
      response = await request();
    }
  }
  const data = await response.json().catch(() => null);
  if (!response.ok) {
    const message = response.status === 422 ? Object.values(data?.errors || {}).flat()[0] || data?.message || 'Check your email and password.'
      : response.status === 419 ? 'Your sign-in session expired. Reload DMBC and try again.'
      : response.status === 403 ? 'Your desktop session is unavailable. Close and reopen DMBC, then try again.'
      : response.status >= 500 ? 'DMBC could not complete sign-in. Please check the application log.'
      : data?.message || 'Unable to sign in. Please try again.';
    throw new Error(message);
  }
  if (!data?.user) throw new Error('DMBC returned an unexpected sign-in response. Reload the application and try again.');
  if (data.csrf_token) document.querySelector('meta[name="csrf-token"]')?.setAttribute('content', data.csrf_token);
  return data.user;
}

export async function checkAuthentication() {
  try {
    const response = await fetch('/user', { credentials: 'same-origin', headers: { Accept: 'application/json' } });
    if (!response.ok) return false;
    return Boolean((await response.json()).user);
  } catch { return false; }
}
