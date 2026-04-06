import AboutView from '@/views/AboutView.vue';
import BatteryAdminView from '@/views/BatteryAdminView.vue';
import GridChargerAdminView from '@/views/GridChargerAdminView.vue';
import ConfigAdminView from '@/views/ConfigAdminView.vue';
import ConsoleInfoView from '@/views/ConsoleInfoView.vue';
import DeviceAdminView from '@/views/DeviceAdminView.vue';
import DtuAdminView from '@/views/DtuAdminView.vue';
import ErrorView from '@/views/ErrorView.vue';
import FeatureDisabledView from '@/views/FeatureDisabledView.vue';
import FirmwareUpgradeView from '@/views/FirmwareUpgradeView.vue';
import HomeView from '@/views/HomeView.vue';
import SolarChargerAdminView from '@/views/SolarChargerAdminView.vue';
import PowerMeterAdminView from '@/views/PowerMeterAdminView.vue';
import PowerLimiterAdminView from '@/views/PowerLimiterAdminView.vue';
import InverterAdminView from '@/views/InverterAdminView.vue';
import LoginView from '@/views/LoginView.vue';
import MaintenanceRebootView from '@/views/MaintenanceRebootView.vue';
import LoggingAdminView from '@/views/LoggingAdminView.vue';
import MqttAdminView from '@/views/MqttAdminView.vue';
import MqttInfoView from '@/views/MqttInfoView.vue';
import NetworkAdminView from '@/views/NetworkAdminView.vue';
import NetworkInfoView from '@/views/NetworkInfoView.vue';
import NtpAdminView from '@/views/NtpAdminView.vue';
import NtpInfoView from '@/views/NtpInfoView.vue';
import SecurityAdminView from '@/views/SecurityAdminView.vue';
import SystemInfoView from '@/views/SystemInfoView.vue';
import WaitRestartView from '@/views/WaitRestartView.vue';
import { createRouter, createWebHistory } from 'vue-router';
import { features, isFeatureEnabled } from '@/utils/features';

type WebApiFeatureKey = 'webapi_device' | 'webapi_mqtt' | 'webapi_network' | 'webapi_ntp';

const router = createRouter({
    history: createWebHistory(import.meta.env.BASE_URL),
    linkActiveClass: 'active',
    scrollBehavior() {
        return new Promise((resolve) => {
            setTimeout(() => {
                resolve({ top: 0 });
            }, 100);
        });
    },
    routes: [
        {
            path: '/',
            name: 'Home',
            component: HomeView,
        },
        {
            path: '/login',
            name: 'Login',
            component: LoginView,
        },
        {
            path: '/error?status=:status&message=:message',
            name: 'Error',
            component: ErrorView,
        },
        {
            path: '/feature-disabled',
            name: 'Feature Disabled',
            component: FeatureDisabledView,
        },
        {
            path: '/about',
            name: 'About',
            component: AboutView,
        },
        {
            path: '/info/network',
            name: 'Network',
            component: NetworkInfoView,
            meta: { requiredWebApiFeature: 'webapi_network' },
        },
        {
            path: '/info/system',
            name: 'System',
            component: SystemInfoView,
        },
        {
            path: '/info/ntp',
            name: 'NTP',
            component: NtpInfoView,
            meta: { requiredWebApiFeature: 'webapi_ntp' },
        },
        {
            path: '/info/mqtt',
            name: 'MqTT',
            component: MqttInfoView,
            meta: { requiredWebApiFeature: 'webapi_mqtt' },
        },
        {
            path: '/info/console',
            name: 'Web Console',
            component: ConsoleInfoView,
        },
        {
            path: '/settings/network',
            name: 'Network Settings',
            component: NetworkAdminView,
            meta: { requiredWebApiFeature: 'webapi_network' },
        },
        {
            path: '/settings/ntp',
            name: 'NTP Settings',
            component: NtpAdminView,
            meta: { requiredWebApiFeature: 'webapi_ntp' },
        },
        {
            path: '/settings/solarcharger',
            name: 'Solar Charger Settings',
            component: SolarChargerAdminView,
            meta: { feature: 'solarcharger' },
        },
        {
            path: '/settings/powermeter',
            name: 'Power meter Settings',
            component: PowerMeterAdminView,
            meta: { feature: 'powermeter' },
        },
        {
            path: '/settings/powerlimiter',
            name: 'Power limiter Settings',
            component: PowerLimiterAdminView,
            meta: { feature: 'powerlimiter' },
        },
        {
            path: '/settings/battery',
            name: 'Battery Settings',
            component: BatteryAdminView,
            meta: { feature: 'battery' },
        },
        {
            path: '/settings/chargerac',
            name: 'Charger Settings',
            component: GridChargerAdminView,
            meta: { feature: 'gridcharger' },
        },
        {
            path: '/settings/mqtt',
            name: 'MqTT Settings',
            component: MqttAdminView,
            meta: { requiredWebApiFeature: 'webapi_mqtt' },
        },
        {
            path: '/settings/inverter',
            name: 'Inverter Settings',
            component: InverterAdminView,
        },
        {
            path: '/settings/dtu',
            name: 'DTU Settings',
            component: DtuAdminView,
        },
        {
            path: '/settings/device',
            name: 'Device Manager',
            component: DeviceAdminView,
            meta: { requiredWebApiFeature: 'webapi_device' },
        },
        {
            path: '/firmware/upgrade',
            name: 'Firmware Upgrade',
            component: FirmwareUpgradeView,
        },
        {
            path: '/settings/config',
            name: 'Config Management',
            component: ConfigAdminView,
        },
        {
            path: '/settings/security',
            name: 'Security',
            component: SecurityAdminView,
        },
        {
            path: '/settings/logging',
            name: 'Logging',
            component: LoggingAdminView,
        },
        {
            path: '/maintenance/reboot',
            name: 'Device Reboot',
            component: MaintenanceRebootView,
        },
        {
            path: '/wait',
            name: 'Wait Restart',
            component: WaitRestartView,
        },
    ],
});

router.beforeEach((to) => {
    const feature = to.meta?.feature as
        | 'battery'
        | 'powermeter'
        | 'solarcharger'
        | 'gridcharger'
        | 'powerlimiter'
        | undefined;
    if (feature && !isFeatureEnabled(feature)) {
        return '/';
    }

    const requiredWebApiFeature = to.meta?.requiredWebApiFeature as WebApiFeatureKey | undefined;
    if (requiredWebApiFeature && !features.components[requiredWebApiFeature]) {
        return {
            path: '/feature-disabled',
            query: {
                feature: requiredWebApiFeature,
                path: to.path,
            },
        };
    }

    return true;
});

export default router;
