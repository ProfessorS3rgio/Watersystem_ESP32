<template>
  <transition name="modal" appear>
    <div v-if="isOpen" class="fixed inset-0 z-50">
      <transition name="backdrop" appear>
        <div class="absolute inset-0 bg-black/50" @click="handleCancel"></div>
      </transition>
      <transition name="panel" appear>
        <div class="absolute inset-0 flex items-center justify-center p-4">
          <div class="w-full max-w-md rounded-lg shadow-lg transform transition-all duration-300 ease-out" :class="isDark ? 'bg-gray-900 text-white' : 'bg-white text-gray-900'">
            <div class="px-6 py-4 border-b" :class="isDark ? 'border-gray-800' : 'border-gray-200'">
              <div class="flex items-center justify-between">
                <h3 class="text-lg font-semibold">{{ title }}</h3>
                <button class="text-sm transition-colors duration-200" :class="isDark ? 'text-gray-300 hover:text-white' : 'text-gray-600 hover:text-gray-900'" @click="handleCancel">
                  <svg class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M6 18L18 6M6 6l12 12" />
                  </svg>
                </button>
              </div>
            </div>

            <div class="px-6 py-4">
              <div class="flex items-start">
                <div class="mx-auto flex size-12 shrink-0 items-center justify-center rounded-full bg-red-500/10 animate-pulse-once">
                  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" class="size-6 text-red-400">
                    <path d="M12 9v3.75m-9.303 3.376c-.866 1.5.217 3.374 1.948 3.374h14.71c1.73 0 2.813-1.874 1.948-3.374L13.949 3.378c-.866-1.5-3.032-1.5-3.898 0L2.697 16.126ZM12 15.75h.007v.008H12v-.008Z" stroke-linecap="round" stroke-linejoin="round" />
                  </svg>
                </div>
                <div class="mt-3 ml-4 text-left">
                  <div class="text-sm" :class="isDark ? 'text-gray-300' : 'text-gray-600'" v-html="message">
                  </div>
                </div>
              </div>
            </div>

            <div class="px-6 py-4 flex items-center justify-end gap-3" :class="isDark ? 'bg-gray-800' : 'bg-gray-50'">
              <button type="button" @click="handleCancel" class="px-4 py-2 rounded-lg transition-colors duration-200" :class="isDark ? 'bg-gray-700 hover:bg-gray-600 text-white' : 'bg-gray-200 hover:bg-gray-300 text-gray-900'">
                {{ cancelText }}
              </button>
              <button type="button" @click="handleConfirm" :disabled="isLoading" class="bg-red-600 hover:bg-red-700 text-white px-4 py-2 rounded-lg font-medium disabled:opacity-50 transition-colors duration-200">
                {{ isLoading ? 'Processing...' : confirmText }}
              </button>
            </div>
          </div>
        </div>
      </transition>
    </div>
  </transition>
</template>

<script>
export default {
  name: 'ConfirmDialog',
  props: {
    isOpen: {
      type: Boolean,
      default: false
    },
    title: {
      type: String,
      default: 'Confirm Action'
    },
    message: {
      type: String,
      default: 'Are you sure you want to proceed?'
    },
    confirmText: {
      type: String,
      default: 'Confirm'
    },
    cancelText: {
      type: String,
      default: 'Cancel'
    },
    isLoading: {
      type: Boolean,
      default: false
    },
    isDark: {
      type: Boolean,
      default: false
    }
  },
  emits: ['confirm', 'cancel'],
  methods: {
    handleConfirm() {
      this.$emit('confirm')
    },
    handleCancel() {
      this.$emit('cancel')
    }
  }
}
</script>

<style scoped>
/* Modal container animation */
.modal-enter-active,
.modal-leave-active {
  transition: opacity 0.3s ease;
}

.modal-enter-from,
.modal-leave-to {
  opacity: 0;
}

/* Backdrop animation */
.backdrop-enter-active,
.backdrop-leave-active {
  transition: opacity 0.3s ease;
}

.backdrop-enter-from,
.backdrop-leave-to {
  opacity: 0;
}

/* Panel animation */
.panel-enter-active,
.panel-leave-active {
  transition: all 0.3s ease;
}

.panel-enter-from {
  opacity: 0;
  transform: scale(0.95) translateY(-10px);
}

.panel-leave-to {
  opacity: 0;
  transform: scale(0.95) translateY(-10px);
}

/* Warning icon pulse animation */
@keyframes pulse-once {
  0%, 100% {
    opacity: 1;
  }
  50% {
    opacity: 0.7;
  }
}

.animate-pulse-once {
  animation: pulse-once 1s ease-in-out;
}
</style>