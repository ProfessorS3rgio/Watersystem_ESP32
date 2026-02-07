import axios from 'axios';
window.axios = axios;

window.axios.defaults.headers.common['X-Requested-With'] = 'XMLHttpRequest';

// Ensure cookies are sent (important when using a different dev port/origin).
window.axios.defaults.withCredentials = true;

// Prefer Laravel's XSRF cookie flow (stays in sync with the session).
// This avoids failures when the session is regenerated and the meta token becomes stale.
window.axios.defaults.xsrfCookieName = 'XSRF-TOKEN';
window.axios.defaults.xsrfHeaderName = 'X-XSRF-TOKEN';

// Best-effort fallback: if the XSRF cookie isn't present (very first load), use the meta tag.
const getMetaCsrf = () => document.head.querySelector('meta[name="csrf-token"]')?.getAttribute('content') || null;
const hasXsrfCookie = () => typeof document !== 'undefined' && document.cookie.includes('XSRF-TOKEN=');

window.axios.interceptors.request.use((config) => {
    // Don't pin a potentially stale token globally.
    if (!hasXsrfCookie()) {
        const metaToken = getMetaCsrf();
        if (metaToken) {
            config.headers = config.headers || {};
            config.headers['X-CSRF-TOKEN'] = metaToken;
        }
    } else if (config?.headers && config.headers['X-CSRF-TOKEN']) {
        // If a stale meta token was set elsewhere, drop it so Laravel uses X-XSRF-TOKEN.
        delete config.headers['X-CSRF-TOKEN'];
    }
    return config;
});

// Auto-recover from CSRF/session rotation (Laravel returns 419).
window.axios.interceptors.response.use(
    (response) => response,
    async (error) => {
        const status = error?.response?.status;
        const original = error?.config;

        if (status === 419 && original && !original.__retried419) {
            original.__retried419 = true;
            try {
                // Touch a lightweight endpoint to refresh XSRF-TOKEN + session cookies.
                await window.axios.get('/csrf-token', { headers: { 'Accept': 'application/json' } });
            } catch (_) {
                // Ignore; retry will still happen and may succeed if cookies refreshed anyway.
            }
            return window.axios(original);
        }

        return Promise.reject(error);
    }
);
