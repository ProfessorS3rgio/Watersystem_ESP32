<template>
  <div v-if="showModal && receipt" class="dmbc-modal fixed inset-0 z-50" :data-dark="isDark" @keydown.esc="$emit('close')">
    <div class="absolute inset-0 bg-black/50" @click="$emit('close')"></div>
    <div class="absolute inset-0 flex items-center justify-center p-4">
      <div role="dialog" aria-modal="true" aria-labelledby="receipt-dialog-title" class="dmbc-dialog w-full max-w-lg">
        <div class="dmbc-dialog-header flex items-start justify-between gap-4 px-6 py-5 border-b">
          <div>
            <p class="text-xs font-bold uppercase tracking-widest text-teal-600">Payment recorded</p>
            <h3 id="receipt-dialog-title" class="mt-1 text-xl font-bold">Your receipt is ready</h3>
            <p class="mt-1 text-sm opacity-70">Review your payment details before printing.</p>
          </div>
          <button type="button" aria-label="Close receipt" class="rounded-lg p-2 opacity-70 hover:opacity-100" @click="$emit('close')">✕</button>
        </div>
        <div class="px-6 py-5">
          <div class="mb-3 flex items-center justify-between text-xs">
            <span class="font-semibold uppercase tracking-widest opacity-60">Print preview</span>
            <span class="rounded-full bg-teal-100 px-3 py-1 font-semibold text-teal-800">58 mm · Xprinter</span>
          </div>
          <div class="receipt-tray">
            <div class="receipt-preview-paper" v-html="previewHtml"></div>
          </div>
          <p class="mt-4 text-xs leading-relaxed opacity-70">Choose your XP-58IIH with 58 mm paper, 100% scale, and no margins. <strong>Keep browser headers and footers OFF</strong> to avoid extra blank paper.</p>
          <div class="mt-5 flex gap-3">
            <button type="button" @click="$emit('print')" class="flex flex-1 items-center justify-center gap-2 rounded-xl bg-teal-700 px-4 py-3 text-sm font-bold text-white transition-colors hover:bg-teal-800">
              <svg aria-hidden="true" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8"><path d="M6 9V3h12v6M6 18H4a2 2 0 0 1-2-2v-5a2 2 0 0 1 2-2h16a2 2 0 0 1 2 2v5a2 2 0 0 1-2 2h-2"/><path d="M6 14h12v8H6zM18 12h.01"/></svg>
              Print receipt
            </button>
            <button type="button" @click="$emit('close')" class="rounded-xl border border-current/20 px-5 py-3 text-sm font-semibold hover:opacity-70">Done</button>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import { computed } from 'vue'
import { receiptMarkup, receiptStyles } from '../../utils/receipt.js'

export default {
  name: 'PaymentReceipt',
  props: {
    isDark: { type: Boolean, required: true },
    showModal: { type: Boolean, default: false },
    customerName: { type: String, required: true },
    receipt: { type: Object, default: null },
  },
  emits: ['close', 'print'],
  setup(props) {
    return {
      previewHtml: computed(() => props.receipt
        ? `<style>${receiptStyles}</style>${receiptMarkup(props.receipt, props.customerName)}` : ''),
    }
  },
}
</script>

<style scoped>
.receipt-tray { max-height: 48dvh; overflow: auto; padding: 22px 12px 30px; border: 1px solid #dcebe8; border-radius: 14px; background: #eaf1ef; }
[data-dark="true"] .receipt-tray { border-color: #28454c; background: #08171d; }
.receipt-preview-paper { width: 58mm; padding: 0 5mm; margin: 0 auto; background: #fff; box-shadow: 0 5px 18px #183e3920; position: relative; }
.receipt-preview-paper::after { content: ''; position: absolute; bottom: -5px; left: 0; right: 0; height: 6px; background: linear-gradient(135deg, #fff 25%, transparent 25%) -5px 0, linear-gradient(225deg, #fff 25%, transparent 25%) -5px 0; background-size: 10px 10px; }
</style>
