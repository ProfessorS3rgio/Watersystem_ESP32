<template>
  <div class="settings-savebar flex flex-wrap items-center justify-between gap-4 border-t p-5 sm:px-6">
    <p role="status" class="flex items-center gap-2 text-xs font-semibold settings-muted"><span class="h-2 w-2 rounded-full" :class="dirty ? 'bg-amber-500' : 'bg-teal-500'"></span>{{ saving ? 'Saving your changes…' : dirty ? 'You have unsaved changes' : 'All changes saved' }}</p>
    <div class="flex gap-3"><button type="button" @click="$emit('discard')" :disabled="!dirty || saving" class="settings-secondary">Discard changes</button><button type="submit" :disabled="!dirty || saving" class="settings-primary">{{ saving ? 'Saving…' : label }}</button></div>
  </div>
</template>

<script setup>
defineProps({ dirty: Boolean, saving: Boolean, label: { type: String, default: 'Save changes' } })
defineEmits(['discard'])
</script>
