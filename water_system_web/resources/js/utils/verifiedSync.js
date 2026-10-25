// Stable comparisons ignore property order, but preserve every device field.
export function syncSnapshot(value) {
  if (Array.isArray(value)) return `[${value.map(syncSnapshot).sort().join(',')}]`
  if (value && typeof value === 'object') return `{${Object.keys(value).sort().map(key => `${JSON.stringify(key)}:${syncSnapshot(value[key])}`).join(',')}}`
  return JSON.stringify(value)
}

export async function acknowledgeVerifiedGroup(serial, command) {
  let removeLine = () => {}, removeStatus = () => {}, timer
  try {
    await new Promise((resolve, reject) => {
      removeLine = serial.onLine(line => {
        if (line === `ACK|${command}`) resolve()
        else if (line.startsWith('ERR|')) reject(new Error(`Device confirmation failed: ${line}. Saved website data is safe; sync again to retry.`))
      })
      removeStatus = serial.onStatus(state => {
        if (!state.isConnected) reject(new Error('Device disconnected after saving. Sync again to confirm its records.'))
      })
      timer = setTimeout(() => reject(new Error('Device did not confirm the saved records. Sync again to retry.')), 15000)
      Promise.resolve(serial.sendLine(command)).catch(reject)
    })
  } finally { clearTimeout(timer); removeLine(); removeStatus() }
}
