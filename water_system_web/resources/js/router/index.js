import { createRouter, createWebHashHistory } from 'vue-router';
import Home from '../pages/Home.vue';
import Login from '../pages/Login.vue';
import Clients from '../pages/Clients.vue';
import Billing from '../pages/Billing.vue';
import SyncDevice from '../pages/SyncDevice.vue';
import Settings from '../pages/Settings.vue';

const checkAuth = async () => {
  try {
    const response = await fetch('/user', {
      headers: {
        'X-CSRF-TOKEN': document.querySelector('meta[name="csrf-token"]').getAttribute('content')
      }
    });
    return response.ok;
  } catch (error) {
    return false;
  }
};

export const router = createRouter({
  history: createWebHashHistory(),
  routes: [
    {
      path: '/',
      name: 'home',
      component: Home,
      meta: { requiresAuth: true }
    },
    {
      path: '/login',
      name: 'login',
      component: Login,
    },
    {
      path: '/clients',
      name: 'clients',
      component: Clients,
      meta: { requiresAuth: true }
    },
    {
      path: '/billing',
      name: 'billing',
      component: Billing,
      meta: { requiresAuth: true }
    },
    {
      path: '/sync',
      name: 'sync',
      component: SyncDevice,
      meta: { requiresAuth: true }
    },
    {
      path: '/settings',
      name: 'settings',
      component: Settings,
      meta: { requiresAuth: true }
    },
  ],
});

// Navigation guard
router.beforeEach(async (to, from, next) => {
  const isAuthenticated = await checkAuth();

  if (to.meta.requiresAuth && !isAuthenticated) {
    next('/login');
  } else if (to.path === '/login' && isAuthenticated) {
    next('/');
  } else {
    next();
  }
});
