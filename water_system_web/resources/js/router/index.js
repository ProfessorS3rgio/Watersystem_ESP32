import { createRouter, createWebHashHistory } from 'vue-router';
import Home from '../pages/Home.vue';
import Login from '../pages/Login.vue';
import Clients from '../pages/Clients.vue';
import Billing from '../pages/Billing.vue';
import BillingSummary from '../components/billing/BillingSummary.vue';
import CustomerBilling from '../components/billing/CustomerBilling.vue';
import AllBills from '../components/billing/AllBills.vue';
import SyncDevice from '../pages/SyncDevice.vue';
import Settings from '../pages/Settings.vue';
import AuditLogs from '../pages/AuditLogs.vue';
import Incidents from '../pages/Incidents.vue';
import { checkAuthentication } from '../services/authApi.js';

const checkAuth = checkAuthentication;

export const router = createRouter({
  history: createWebHashHistory(),
  routes: [
    { path: '/incidents', name: 'incidents', component: Incidents, meta: { requiresAuth: true } },
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
    { path: '/clients/:id(\\d+)/profile', name: 'member-profile', component: () => import('../pages/MemberProfile.vue'), meta: { requiresAuth: true } },
    {
      path: '/billing',
      name: 'billing',
      component: Billing,
      meta: { requiresAuth: true },
      children: [
        {
          path: 'summary',
          name: 'billing-summary',
          component: BillingSummary
        },
        {
          path: 'customer',
          name: 'billing-customer',
          component: CustomerBilling
        },
        {
          path: 'all-bills',
          name: 'billing-all-bills',
          component: AllBills
        }
      ]
    },
    {
      path: '/sync',
      name: 'sync',
      component: SyncDevice,
      meta: { requiresAuth: true }
    },
    {
      path: '/sync/normalize-dates',
      name: 'sync-normalize-dates',
      component: SyncDevice,
      meta: { requiresAuth: true }
    },
    {
      path: '/settings',
      name: 'settings',
      redirect: '/settings/billing',
      meta: { requiresAuth: true }
    },
    {
      path: '/settings/billing',
      name: 'settings-billing',
      component: Settings,
      meta: { requiresAuth: true }
    },
    {
      path: '/settings/backup',
      name: 'settings-backup',
      component: Settings,
      meta: { requiresAuth: true }
    },
    {
      path: '/settings/categories',
      name: 'settings-categories',
      component: Settings,
      meta: { requiresAuth: true }
    },
    {
      path: '/audit-logs',
      name: 'audit-logs',
      component: AuditLogs,
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
