<template>
  <transition name="modal" appear>
    <div v-if="isOpen" class="fixed inset-0 z-50">
      <transition name="backdrop" appear>
        <div class="absolute inset-0 bg-black/50" @click="handleClose"></div>
      </transition>
      <transition name="panel" appear>
        <div class="absolute inset-0 flex items-center justify-center p-4">
          <div class="w-full max-w-lg rounded-lg shadow-lg transform transition-all duration-300 ease-out" :class="isDark ? 'bg-gray-900 text-white' : 'bg-white text-gray-900'">
            <div class="px-6 py-4 border-b" :class="isDark ? 'border-gray-800' : 'border-gray-200'">
              <div class="flex items-center justify-between">
                <h3 class="text-lg font-semibold">{{ title }}</h3>
                <button class="text-sm transition-colors duration-200" :class="isDark ? 'text-gray-300 hover:text-white' : 'text-gray-600 hover:text-gray-900'" @click="handleClose">
                  <svg class="h-5 w-5" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M6 18L18 6M6 6l12 12" />
                  </svg>
                </button>
              </div>
            </div>

            <div class="px-6 py-4">
              <div class="text-center">
                <svg xmlns="http://www.w3.org/2000/svg" class="w-14 shrink-0 fill-green-500 inline animate-bounce-once" viewBox="0 0 512 512">
                  <path d="M383.841 171.838c-7.881-8.31-21.02-8.676-29.343-.775L221.987 296.732l-63.204-64.893c-8.005-8.213-21.13-8.393-29.35-.387-8.213 7.998-8.386 21.137-.388 29.35l77.492 79.561a20.687 20.687 0 0 0 14.869 6.275 20.744 20.744 0 0 0 14.288-5.694l147.373-139.762c8.316-7.888 8.668-21.027.774-29.344z" data-original="#000000" />
                  <path d="M256 0C114.84 0 0 114.84 0 256s114.84 256 256 256 256-114.84 256-256S397.16 0 256 0zm0 470.487c-118.265 0-214.487-96.214-214.487-214.487 0-118.265 96.221-214.487 214.487-214.487 118.272 0 214.487 96.221 214.487 214.487 0 118.272-96.215 214.487-214.487 214.487z" data-original="#000000" />
                </svg>
                <div class="mt-6">
                  <h3 class="text-xl font-semibold" :class="isDark ? 'text-white' : 'text-slate-900'">{{ title }}</h3>
                  <p class="text-sm leading-relaxed mt-3" :class="isDark ? 'text-gray-300' : 'text-slate-500'" v-html="message">
                  </p>
                </div>
              </div>
            </div>

            <div class="px-6 py-4">
              <button type="button" @click="handleClose" class="w-full px-5 py-2.5 rounded-lg text-white text-sm font-medium border-none outline-none bg-gray-800 hover:bg-gray-700 transition-colors duration-200">
                {{ buttonText }}
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
  name: 'SuccessModal',
  props: {
    isOpen: {
      type: Boolean,
      default: false
    },
    title: {
      type: String,
      default: 'Success!'
    },
    message: {
      type: String,
      default: 'Operation completed successfully.'
    },
    buttonText: {
      type: String,
      default: 'Got it'
    },
    isDark: {
      type: Boolean,
      default: false
    }
  },
  emits: ['close'],
  methods: {
    handleClose() {
      this.$emit('close')
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

/* Success icon bounce animation */
@keyframes bounce-once {
  0%, 20%, 50%, 80%, 100% {
    transform: translateY(0);
  }
  40% {
    transform: translateY(-10px);
  }
  60% {
    transform: translateY(-5px);
  }
}

.animate-bounce-once {
  animation: bounce-once 1s ease-in-out;
}
</style>