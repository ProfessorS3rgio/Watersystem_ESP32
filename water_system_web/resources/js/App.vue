<template>
  <RouterView />
  <DesktopPortPicker />
</template>

<script setup>
import { RouterView } from 'vue-router';
import DesktopPortPicker from './components/sync/DesktopPortPicker.vue';
</script>
