<template>
    <BasePage :title="$t('deviceadmin.DeviceManager')" :isLoading="dataLoading || pinMappingLoading || languageLoading">
        <BootstrapAlert
            v-model="alert.show"
            dismissible
            :variant="alert.type"
            :auto-dismiss="alert.type != 'success' ? 0 : 5000"
        >
            {{ alert.message }}
        </BootstrapAlert>

        <form @submit="savePinConfig">
            <nav>
                <div class="nav nav-tabs" id="nav-tab" role="tablist">
                    <button
                        class="nav-link active"
                        id="nav-pin-tab"
                        data-bs-toggle="tab"
                        data-bs-target="#nav-pin"
                        type="button"
                        role="tab"
                        aria-controls="nav-pin"
                        aria-selected="true"
                    >
                        {{ $t('deviceadmin.PinAssignment') }}
                    </button>
                    <button
                        class="nav-link"
                        id="nav-display-tab"
                        data-bs-toggle="tab"
                        data-bs-target="#nav-display"
                        type="button"
                        role="tab"
                        aria-controls="nav-display"
                    >
                        {{ $t('deviceadmin.Display') }}
                    </button>
                    <button
                        class="nav-link"
                        id="nav-leds-tab"
                        data-bs-toggle="tab"
                        data-bs-target="#nav-leds"
                        type="button"
                        role="tab"
                        aria-controls="nav-leds"
                    >
                        {{ $t('deviceadmin.Leds') }}
                    </button>
                </div>
            </nav>
            <div class="tab-content" id="nav-tabContent">
                <div
                    class="tab-pane fade show active"
                    id="nav-pin"
                    role="tabpanel"
                    aria-labelledby="nav-pin-tab"
                    tabindex="0"
                >
                    <div class="card card-tabbed">
                        <div class="card-body">
                            <div class="row mb-3">
                                <label for="inputPinProfile" class="col-sm-2 col-form-label">{{
                                    $t('deviceadmin.SelectedProfile')
                                }}</label>
                                <div class="col-sm-10">
                                    <select
                                        class="form-select"
                                        id="inputPinProfile"
                                        v-model="deviceConfigList.curPin.name"
                                    >
                                        <option
                                            v-for="device in pinMappingList"
                                            :value="device.name"
                                            :key="device.name"
                                        >
                                            {{
                                                device.name === 'Default'
                                                    ? $t('deviceadmin.DefaultProfile')
                                                    : device.name
                                            }}
                                        </option>
                                    </select>
                                </div>
                            </div>

                            <div class="row mb-3" v-if="docLinks.length">
                                <div class="col-sm-2"></div>
                                <div class="col-sm-10 d-flex gap-3">
                                    <a
                                        v-for="(doc, index) in docLinks"
                                        :key="index"
                                        :href="doc.url"
                                        class="btn btn-primary"
                                        target="_blank"
                                        >{{ doc.name }}</a
                                    >
                                </div>
                            </div>

                            <div
                                class="alert alert-danger mt-3"
                                role="alert"
                                v-html="$t('deviceadmin.ProfileHint')"
                            ></div>

                            <PinInfo
                                :selectedPinAssignment="
                                    pinMappingList.find((i) => i.name === deviceConfigList.curPin.name)
                                "
                                :currentPinAssignment="deviceConfigList.curPin"
                            />
                        </div>
                    </div>
                </div>

                <div
                    class="tab-pane fade show"
                    id="nav-display"
                    role="tabpanel"
                    aria-labelledby="nav-display-tab"
                    tabindex="0"
                >
                    <div class="card card-tabbed">
                        <div class="card-body">
                            <InputElement
                                :label="$t('deviceadmin.PowerSafe')"
                                v-model="deviceConfigList.display.power_safe"
                                type="checkbox"
                                :tooltip="$t('deviceadmin.PowerSafeHint')"
                            />

                            <InputElement
                                :label="$t('deviceadmin.Screensaver')"
                                v-model="deviceConfigList.display.screensaver"
                                type="checkbox"
                                :tooltip="$t('deviceadmin.ScreensaverHint')"
                            />

                            <div class="row mb-3">
                                <label class="col-sm-2 col-form-label">
                                    {{ $t('deviceadmin.DiagramMode') }}
                                </label>
                                <div class="col-sm-10">
                                    <select class="form-select" v-model="deviceConfigList.display.diagrammode">
                                        <option v-for="mode in diagramModeList" :key="mode.key" :value="mode.key">
                                            {{ $t(`deviceadmin.` + mode.value) }}
                                        </option>
                                    </select>
                                </div>
                            </div>

                            <InputElement
                                :label="$t('deviceadmin.DiagramDuration')"
                                v-model="deviceConfigList.display.diagramduration"
                                type="number"
                                min="600"
                                max="86400"
                                :tooltip="$t('deviceadmin.DiagramDurationHint')"
                                :postfix="$t('deviceadmin.Seconds')"
                            />

                            <div class="row mb-3">
                                <label class="col-sm-2 col-form-label">
                                    {{ $t('deviceadmin.DisplayLanguage') }}
                                </label>
                                <div class="col-sm-10">
                                    <select class="form-select" v-model="deviceConfigList.display.locale">
                                        <option
                                            v-for="(locale, index) in displayLocaleList"
                                            :key="locale.code"
                                            :value="locale.code"
                                        >
                                            {{ $t((index < 3 ? `deviceadmin.` : ``) + locale.name) }}
                                        </option>
                                    </select>
                                </div>
                            </div>

                            <div class="row mb-3">
                                <label class="col-sm-2 col-form-label">
                                    {{ $t('deviceadmin.Rotation') }}
                                </label>
                                <div class="col-sm-10">
                                    <select class="form-select" v-model="deviceConfigList.display.rotation">
                                        <option
                                            v-for="rotation in displayRotationList"
                                            :key="rotation.key"
                                            :value="rotation.key"
                                        >
                                            {{ $t(`deviceadmin.` + rotation.value) }}
                                        </option>
                                    </select>
                                </div>
                            </div>

                            <div class="row mb-3">
                                <label for="inputDisplayContrast" class="col-sm-2 col-form-label">{{
                                    $t('deviceadmin.Contrast', {
                                        contrast: $n(deviceConfigList.display.contrast / 100, 'percent'),
                                    })
                                }}</label>
                                <div class="col-sm-10">
                                    <input
                                        type="range"
                                        class="form-range"
                                        min="0"
                                        max="100"
                                        id="inputDisplayContrast"
                                        v-model.number="deviceConfigList.display.contrast"
                                    />
                                </div>
                            </div>
                        </div>
                    </div>
                </div>

                <div
                    class="tab-pane fade show"
                    id="nav-leds"
                    role="tabpanel"
                    aria-labelledby="nav-leds-tab"
                    tabindex="0"
                >
                    <div class="card card-tabbed">
                        <div class="card-body">
                            <InputElement
                                :label="$t('deviceadmin.EqualBrightness')"
                                v-model="equalBrightnessCheckVal"
                                type="checkbox"
                            />

                            <div class="row mb-3" v-for="(ledSetting, index) in deviceConfigList.led" :key="index">
                                <label :for="getLedIdFromNumber(index)" class="col-sm-2 col-form-label">{{
                                    $t('deviceadmin.LedBrightness', {
                                        led: index,
                                        brightness: $n(ledSetting.brightness / 100, 'percent'),
                                    })
                                }}</label>
                                <div class="col-sm-10">
                                    <input
                                        type="range"
                                        class="form-range"
                                        min="0"
                                        max="100"
                                        :id="getLedIdFromNumber(index)"
                                        v-model.number="ledSetting.brightness"
                                        @change="syncSliders"
                                    />
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <FormFooter @reload="getDeviceConfig" />
        </form>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import FormFooter from '@/components/FormFooter.vue';
import InputElement from '@/components/InputElement.vue';
import PinInfo from '@/components/PinInfo.vue';
import type { AlertResponse } from '@/types/AlertResponse';
import type { DeviceConfig, Led } from '@/types/DeviceConfig';
import type { PinMapping, Device } from '@/types/PinMapping';
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';

interface DisplayLocaleOption {
    code: string;
    name: string;
}

function createDefaultDevice(name = 'Default'): Device {
    return {
        name,
        links: [],
        nrf24: { miso: -1, mosi: -1, clk: -1, irq: -1, en: -1, cs: -1 },
        cmt: { clk: -1, cs: -1, fcs: -1, sdio: -1, gpio2: -1, gpio3: -1 },
        eth: { enabled: false, phy_addr: -1, power: -1, mdc: -1, mdio: -1, type: -1, clk_mode: -1 },
        display: { type: -1, data: -1, clk: -1, cs: -1, reset: -1 },
        victron: { rx: -1, tx: -1 },
        battery: { rx: -1, tx: -1 },
        huawei: { miso: -1, mosi: -1, clk: -1, cs: -1, irq: -1, rx: -1, tx: -1, power: -1 },
        powermeter: { rx: -1, tx: -1, dere: -1, rxen: -1, txen: -1 },
    };
}

function createDefaultDeviceConfig(): DeviceConfig {
    return {
        curPin: createDefaultDevice('Default'),
        display: {
            rotation: 0,
            power_safe: false,
            screensaver: false,
            contrast: 100,
            locale: 'en',
            diagramduration: 600,
            diagrammode: 0,
        },
        led: [],
    };
}

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        FormFooter,
        InputElement,
        PinInfo,
    },
    data() {
        return {
            dataLoading: true,
            pinMappingLoading: true,
            languageLoading: true,
            deviceConfigList: createDefaultDeviceConfig(),
            pinMappingList: [] as PinMapping,
            alert: {} as AlertResponse,
            equalBrightnessCheckVal: false,
            displayRotationList: [
                { key: 0, value: 'rot0' },
                { key: 1, value: 'rot90' },
                { key: 2, value: 'rot180' },
                { key: 3, value: 'rot270' },
            ],
            displayLocaleList: [
                { code: 'en', name: 'en' },
                { code: 'de', name: 'de' },
                { code: 'fr', name: 'fr' },
            ] as Array<DisplayLocaleOption>,
            diagramModeList: [
                { key: 0, value: 'off' },
                { key: 1, value: 'small' },
                { key: 2, value: 'fullscreen' },
            ],
        };
    },
    created() {
        this.getDeviceConfig();
        this.getPinMappingList();
        this.getLanguageList();
    },
    watch: {
        equalBrightnessCheckVal: function (val) {
            if (!val) {
                return;
            }
            this.deviceConfigList.led.every((v) => (v.brightness = this.deviceConfigList.led[0]?.brightness ?? 0));
        },
    },
    computed: {
        docLinks() {
            const mapping = this.pinMappingList.find((i) => i.name === this.deviceConfigList.curPin.name);
            return mapping?.links || [];
        },
    },
    methods: {
        getLanguageList() {
            this.languageLoading = true;
            fetch('/api/i18n/languages')
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data: Array<DisplayLocaleOption>) => {
                    this.displayLocaleList.push(...data);
                })
                .catch(() => {})
                .finally(() => {
                    this.languageLoading = false;
                });
        },
        getPinMappingList() {
            this.pinMappingLoading = true;
            fetch('/api/file/get?file=pin_mapping.json', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router, true))
                .then((data) => {
                    this.pinMappingList = data;
                })
                .catch((error) => {
                    if (error.status != 404) {
                        this.alert.message = this.$t('deviceadmin.ParseError', {
                            error: error.message,
                        });
                        this.alert.type = 'danger';
                        this.alert.show = true;
                    }
                    this.pinMappingList = [] as PinMapping;
                })
                .finally(() => {
                    this.pinMappingList.sort((a, b) => (a.name < b.name ? -1 : 1));
                    this.pinMappingList.splice(0, 0, createDefaultDevice('Default'));
                    this.pinMappingLoading = false;
                });
        },
        getDeviceConfig() {
            this.dataLoading = true;
            fetch('/api/device/config', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    const defaults = createDefaultDeviceConfig();
                    this.deviceConfigList = {
                        ...defaults,
                        ...data,
                        curPin: {
                            ...defaults.curPin,
                            ...(data?.curPin ?? {}),
                        },
                        display: {
                            ...defaults.display,
                            ...(data?.display ?? {}),
                        },
                        led: Array.isArray(data?.led) ? data.led : defaults.led,
                    };
                    if (this.deviceConfigList.curPin.name === '') {
                        this.deviceConfigList.curPin.name = 'Default';
                    }
                    this.equalBrightnessCheckVal = this.isEqualBrightness();
                })
                .catch(() => {
                    this.deviceConfigList = createDefaultDeviceConfig();
                })
                .finally(() => {
                    this.dataLoading = false;
                });
        },
        savePinConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append('data', JSON.stringify(this.deviceConfigList));

            fetch('/api/device/config', {
                method: 'POST',
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((response) => {
                    this.alert.message = this.$t('apiresponse.' + response.code, response.param);
                    this.alert.type = response.type;
                    this.alert.show = true;
                });
        },
        getLedIdFromNumber(ledNo: number): string {
            return 'inputLED' + ledNo + 'Brightness';
        },
        getNumberFromLedId(id: string): number {
            return parseInt(id.replace('inputLED', '').replace('Brightness', ''));
        },
        isEqualBrightness(): boolean {
            const allEqual = (arr: Led[]) => arr.every((v) => v.brightness === arr[0]?.brightness);
            return allEqual(this.deviceConfigList.led);
        },
        syncSliders(event: Event) {
            if (!this.equalBrightnessCheckVal) {
                return;
            }
            const srcId = this.getNumberFromLedId((event.target as Element).id);
            this.deviceConfigList.led.map((v) => (v.brightness = this.deviceConfigList.led[srcId]?.brightness ?? 0));
        },
    },
});
</script>
