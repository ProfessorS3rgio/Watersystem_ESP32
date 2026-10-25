import { ref } from 'vue'
import { databaseService } from '../../services/databaseService'

export function useSyncVerification() {
  const syncReview = ref(null)
  const reviewBusy = ref(false)
  const reviewError = ref('')
  const reviewReceipt = ref(null)
  let settle, checkDevice, stagedPayload, approvalAttempted = false
  const message = error => error?.response?.data?.message || error?.message || String(error)

  async function reviewIncomingBatch(payload, recheck) {
    reviewReceipt.value = null
    stagedPayload = payload
    approvalAttempted = false
    checkDevice = recheck
    reviewError.value = ''
    syncReview.value = await databaseService.previewSyncBatch(payload)
    return new Promise((resolve, reject) => { settle = { resolve, reject } })
  }
  async function approveSyncReview(acceptWarnings) {
    if (!settle || reviewBusy.value || !syncReview.value?.preview.can_approve) return
    reviewBusy.value = true
    reviewError.value = ''
    try {
      await checkDevice()
      approvalAttempted = true
      const receipt = await databaseService.approveSyncBatch(syncReview.value.id, stagedPayload.device.device_uid, acceptWarnings)
      reviewReceipt.value = receipt
      syncReview.value = null
      const done = settle; settle = null
      done.resolve(receipt)
    } catch (error) { reviewError.value = message(error) }
    finally { reviewBusy.value = false }
  }
  async function refreshSyncReview() {
    if (!settle || reviewBusy.value) return
    reviewBusy.value = true; reviewError.value = ''
    try {
      await checkDevice()
      await databaseService.cancelSyncReview(syncReview.value.id)
      syncReview.value = await databaseService.previewSyncBatch(stagedPayload)
    } catch (error) { reviewError.value = message(error) }
    finally { reviewBusy.value = false }
  }
  async function cancelSyncReview() {
    if (!settle || reviewBusy.value) return
    reviewBusy.value = true
    try { await databaseService.cancelSyncReview(syncReview.value.id) }
    catch (error) {
      if (approvalAttempted || error?.response?.status === 409) {
        reviewError.value = 'Could not confirm cancellation. The batch may already be saved. Retry approval to retrieve its receipt, or try cancelling again.'
        reviewBusy.value = false
        return
      }
    }
    syncReview.value = null; reviewBusy.value = false
    const done = settle; settle = null
    done.reject(Object.assign(new Error('Sync cancelled. Device records remain pending.'), { code: 'SYNC_CANCELLED' }))
  }
  return { syncReview, reviewBusy, reviewError, reviewReceipt, reviewIncomingBatch, approveSyncReview, refreshSyncReview, cancelSyncReview }
}
