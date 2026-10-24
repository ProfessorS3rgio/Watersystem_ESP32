// The USB and HTTP transports both deliver complete newline-delimited protocol lines.
export function parseSyncChunks(lines, beginMarker, chunkPrefix) {
  let expected = null
  const chunks = new Map()
  for (const raw of lines) {
    const line = String(raw).trim()
    if (line.startsWith('ERR|')) throw new Error('Device error: ' + line)
    if (line.startsWith(beginMarker + '|')) {
      expected = Number(line.slice(beginMarker.length + 1))
      if (!Number.isInteger(expected) || expected < 0) throw new Error('Invalid export chunk count')
    }
    if (!line.startsWith(chunkPrefix)) continue
    const separator = line.indexOf('|', chunkPrefix.length)
    if (separator < 0) throw new Error('Incomplete device export chunk')
    const index = Number(line.slice(chunkPrefix.length, separator))
    if (!Number.isInteger(index) || index < 0 || chunks.has(index)) throw new Error('Invalid or repeated export chunk')
    const rows = JSON.parse(line.slice(separator + 1))
    if (!Array.isArray(rows)) throw new Error('Invalid device export rows')
    chunks.set(index, rows)
  }
  if (expected === null || chunks.size !== expected) throw new Error('Incomplete device export — no records were acknowledged')
  const rows = []
  for (let i = 0; i < expected; i++) {
    if (!chunks.has(i)) throw new Error('Missing device export chunk')
    rows.push(...chunks.get(i))
  }
  return rows
}
