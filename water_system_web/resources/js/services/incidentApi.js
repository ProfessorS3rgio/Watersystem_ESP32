import { settingsJson } from './settingsApi.js';

export const incidentApi = {
  list: (filters, signal) => settingsJson('/incidents?' + new URLSearchParams(Object.entries(filters).filter(([, value]) => value !== '' && value != null)), { signal }),
  customers: (filters, signal) => settingsJson('/incidents/customers?' + new URLSearchParams(filters), { signal }),
  detail: (id, signal) => settingsJson(`/incidents/${id}`, { signal }),
  history: (id, before, signal) => settingsJson(`/incidents/${id}/history?before_id=${before}`, { signal }),
  create: payload => settingsJson('/incidents', { method: 'POST', body: JSON.stringify(payload) }),
  update: (id, payload) => settingsJson(`/incidents/${id}`, { method: 'PATCH', body: JSON.stringify(payload) }),
};

export function incidentToken() {
  if (globalThis.crypto.randomUUID) return globalThis.crypto.randomUUID();
  const bytes = globalThis.crypto.getRandomValues(new Uint8Array(16));
  bytes[6] = (bytes[6] & 15) | 64; bytes[8] = (bytes[8] & 63) | 128;
  const hex = [...bytes].map(byte => byte.toString(16).padStart(2, '0')).join('');
  return `${hex.slice(0, 8)}-${hex.slice(8, 12)}-${hex.slice(12, 16)}-${hex.slice(16, 20)}-${hex.slice(20)}`;
}
