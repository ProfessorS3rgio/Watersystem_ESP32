import assert from 'node:assert/strict'
import { createServer } from 'vite'
const server = await createServer({ configFile:false, optimizeDeps:{ noDiscovery:true, entries:[] }, server:{ middlewareMode:true, hmr:false }, appType:'custom' })
try {
  const { useSyncVerification } = await server.ssrLoadModule('/resources/js/composables/Sync/useSyncVerification.js')
  const { databaseService: db } = await server.ssrLoadModule('/resources/js/services/databaseService.js')
  const events=[]
  let fail = true, id = 0
  db.previewSyncBatch = async () => ({ id:++id, preview:{ can_approve:true } })
  db.cancelSyncReview = async reviewId => { events.push('cancel:'+reviewId) }
  db.approveSyncBatch = async (reviewId, uid, accept) => {
    events.push(['approve',reviewId,uid,accept])
    if (fail) throw { response:{data:{message:'Website changed. Refresh checks.'}} }
    return { review_id:reviewId }
  }
  const flow = useSyncVerification()
  const pending = flow.reviewIncomingBatch({device:{device_uid:'MASTER'}}, async () => { events.push('recheck') })
  await new Promise(resolve => setImmediate(resolve))
  assert.equal(flow.syncReview.value.id,1)
  await flow.approveSyncReview(true)
  assert.match(flow.reviewError.value,/Website changed/)
  assert.equal(flow.syncReview.value.id,1,'failed saves keep the review open')
  assert.equal(flow.reviewBusy.value,false)
  await flow.refreshSyncReview()
  assert.equal(flow.syncReview.value.id,2)
  fail = false
  await flow.approveSyncReview(true)
  assert.deepEqual(await pending,{review_id:2})
  assert.equal(flow.syncReview.value,null)
  assert.deepEqual(events.at(-1),['approve',2,'MASTER',true])
  const cancelled = flow.reviewIncomingBatch({device:{device_uid:'MASTER'}},async () => {})
  await new Promise(resolve => setImmediate(resolve))
  const rejection = assert.rejects(cancelled,error=>error.code==='SYNC_CANCELLED')
  await flow.cancelSyncReview()
  await rejection
  assert.equal(flow.syncReview.value,null)
  console.log('Review interaction checks passed: pause, stale save recovery, refreshed approval, and cancellation.')
} finally { await server.close() }
