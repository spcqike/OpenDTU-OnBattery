<template>
    <BasePage :title="$t('featuredisabled.Title')">
        <CardElement :text="$t('featuredisabled.Title')" textVariant="text-bg-warning">
            <p class="mb-2">
                {{ $t('featuredisabled.Message', { feature: featureLabel }) }}
            </p>
            <p class="mb-3 text-body-secondary" v-if="targetPath">
                {{ $t('featuredisabled.Target', { path: targetPath }) }}
            </p>
            <router-link to="/" class="btn btn-primary">
                {{ $t('featuredisabled.BackHome') }}
            </router-link>
        </CardElement>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import CardElement from '@/components/CardElement.vue';
import { defineComponent } from 'vue';

const featureToLabelKey: Record<string, string> = {
    webapi_device: 'menu.DeviceManager',
    webapi_mqtt: 'menu.MQTT',
    webapi_network: 'menu.Network',
    webapi_ntp: 'menu.NTP',
};

export default defineComponent({
    components: {
        BasePage,
        CardElement,
    },
    computed: {
        featureKey(): string {
            const value = this.$route.query.feature;
            if (Array.isArray(value)) {
                return value[0] ?? '';
            }
            return value ?? '';
        },
        targetPath(): string {
            const value = this.$route.query.path;
            if (Array.isArray(value)) {
                return value[0] ?? '';
            }
            return value ?? '';
        },
        featureLabel(): string {
            const labelKey = featureToLabelKey[this.featureKey];
            return labelKey ? String(this.$t(labelKey)) : this.featureKey;
        },
    },
});
</script>
