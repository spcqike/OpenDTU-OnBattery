import { reactive } from 'vue';

export interface FeatureSet {
    components: {
        webapp: boolean;
        display: boolean;
        battery: boolean;
        powermeter: boolean;
        solarcharger: boolean;
        gridcharger: boolean;
        powerlimiter: boolean;
        webapi_device: boolean;
        webapi_mqtt: boolean;
        webapi_network: boolean;
        webapi_ntp: boolean;
        webapi_prometheus: boolean;
    };
    providers: {
        powermeter: {
            mqtt: boolean;
            sdm: boolean;
            http_json: boolean;
            serial_sml: boolean;
            smahm2: boolean;
            http_sml: boolean;
            modbus_udp_victron: boolean;
        };
        battery: {
            pylontech: boolean;
            jkbms: boolean;
            mqtt: boolean;
            victron_smartshunt: boolean;
            pytes: boolean;
            sbs: boolean;
            jbdbms: boolean;
            zendure: boolean;
        };
        solarcharger: {
            vedirect: boolean;
            mqtt: boolean;
        };
        gridcharger: {
            huawei: boolean;
            trucki: boolean;
        };
    };
}

const defaults: FeatureSet = {
    components: {
        webapp: true,
        display: true,
        battery: true,
        powermeter: true,
        solarcharger: true,
        gridcharger: true,
        powerlimiter: true,
        webapi_device: true,
        webapi_mqtt: true,
        webapi_network: true,
        webapi_ntp: true,
        webapi_prometheus: true,
    },
    providers: {
        powermeter: {
            mqtt: true,
            sdm: true,
            http_json: true,
            serial_sml: true,
            smahm2: true,
            http_sml: true,
            modbus_udp_victron: true,
        },
        battery: {
            pylontech: true,
            jkbms: true,
            mqtt: true,
            victron_smartshunt: true,
            pytes: true,
            sbs: true,
            jbdbms: true,
            zendure: true,
        },
        solarcharger: {
            vedirect: true,
            mqtt: true,
        },
        gridcharger: {
            huawei: true,
            trucki: true,
        },
    },
};

export const features = reactive<FeatureSet>(JSON.parse(JSON.stringify(defaults)));

function updateBooleans(target: Record<string, unknown>, source: Record<string, unknown>) {
    Object.keys(target).forEach((key) => {
        if (typeof source[key] === 'boolean') {
            target[key] = source[key];
        } else if (
            typeof target[key] === 'object' &&
            target[key] !== null &&
            typeof source[key] === 'object' &&
            source[key] !== null
        ) {
            updateBooleans(target[key] as Record<string, unknown>, source[key] as Record<string, unknown>);
        }
    });
}

export async function loadFeatures() {
    try {
        const response = await fetch('/api/features');
        if (!response.ok) {
            return;
        }

        const responseData = (await response.json()) as Record<string, unknown>;
        updateBooleans(features as unknown as Record<string, unknown>, responseData);
    } catch {
        // Keep defaults when the endpoint is unavailable (older firmware / auth constraints).
    }
}

export function isFeatureEnabled(component: keyof FeatureSet['components']): boolean {
    return features.components[component];
}
