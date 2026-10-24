import assert from 'node:assert/strict'
import { createRenderer, h, nextTick } from 'vue'
import { createServer } from 'vite'
import { settingsSectionFromPath } from '../resources/js/config/settingsSections.js'

const renderer = createRenderer({
  createElement: () => ({ children: [] }), createText: text => ({ text }), createComment: () => ({}),
  setText() {}, setElementText() {}, patchProp() {},
  insert: (child, parent) => parent.children.push(child), remove() {}, parentNode: () => null, nextSibling: () => null,
})
const server = await createServer({ configFile: false, server: { middlewareMode: true }, appType: 'custom' })
let app
try {
  const { useEditableSettings } = await server.ssrLoadModule('/resources/js/composables/useEditableSettings.js')
  let editor, calls = 0, fail = false
  const states = []
  app = renderer.createApp({ setup() {
    editor = useEditableSettings({ section: 'billing', initial: { days: 0 }, load: async () => ({ days: 5 }),
      persist: async draft => { calls++; if (fail) throw new Error('Save failed'); return draft },
      emit: (_, state) => states.push(state),
    })
    return () => h('div')
  } })
  app.mount({ children: [] })
  await new Promise(resolve => setImmediate(resolve))
  assert.equal(editor.ready.value, true)
  editor.form.value.days = 12
  await nextTick()
  assert.equal(editor.dirty.value, true)
  assert.equal(states.at(-1).dirty, true)
  editor.discard()
  assert.equal(editor.form.value.days, 5)
  await editor.save()
  assert.equal(calls, 0, 'unchanged drafts must not issue saves')
  editor.form.value.days = 15
  fail = true
  await editor.save()
  assert.equal(editor.error.value, 'Save failed')
  assert.equal(editor.dirty.value, true, 'failed saves must retain the draft')
  assert.equal(editor.baseline.value.days, 5)
  fail = false
  await editor.save()
  assert.equal(editor.dirty.value, false)
  assert.equal(editor.baseline.value.days, 15)
  assert.match(editor.message.value, /saved successfully/)
  assert.equal(settingsSectionFromPath('/settings/categories'), 'categories')
  assert.equal(settingsSectionFromPath('/settings/backup'), 'backup')
  assert.equal(settingsSectionFromPath('/settings'), 'billing')
  console.log('Settings draft checks passed: discard, failed-save recovery, saved feedback, route selection.')
} finally { app?.unmount(); await server.close() }
