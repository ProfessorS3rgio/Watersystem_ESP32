export const settingsSections = [
  { key: 'billing', label: 'Billing rules', to: '/settings/billing', description: 'Due dates, service policy & penalties', icon: 'M5 3h14v18l-3-2-4 2-4-2-3 2V3Zm4 5h6m-6 4h6' },
  { key: 'categories', label: 'Customer categories', to: '/settings/categories', description: 'Water rates & minimum charges', icon: 'M16 21v-2a4 4 0 0 0-4-4H6a4 4 0 0 0-4 4v2m20 0v-2a4 4 0 0 0-3-3.87M13 7a4 4 0 1 1-8 0 4 4 0 0 1 8 0' },
  { key: 'backup', label: 'Backup & restore', to: '/settings/backup', description: 'Download archives & recover snapshots', icon: 'M12 16V3m0 0-4 4m4-4 4 4M4 14v5a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2v-5' },
]

export function settingsSectionFromPath(path) {
  return settingsSections.find(section => section.to === path)?.key || 'billing'
}
