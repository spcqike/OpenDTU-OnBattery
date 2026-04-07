// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2026 Thomas Basler and others
 */
#include "WebApi_features.h"
#include "FeatureFlags.h"
#include "WebApi.h"
#include <AsyncJson.h>

void WebApiFeaturesClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    (void)scheduler;
    using std::placeholders::_1;
    server.on("/api/features", HTTP_GET, static_cast<ArRequestHandlerFunction>(std::bind(&WebApiFeaturesClass::onFeatures, this, _1)));
}

void WebApiFeaturesClass::onFeatures(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    auto asBool = [](int value) -> bool { return value != 0; };

    auto components = root["components"].to<JsonObject>();
    components["webapp"] = asBool(OPENDTU_FEATURE_WEBAPP);
    components["display"] = asBool(OPENDTU_FEATURE_DISPLAY);
    components["battery"] = asBool(OPENDTU_FEATURE_BATTERY);
    components["powermeter"] = asBool(OPENDTU_FEATURE_POWERMETER);
    components["solarcharger"] = asBool(OPENDTU_FEATURE_SOLARCHARGER);
    components["gridcharger"] = asBool(OPENDTU_FEATURE_GRIDCHARGER);
    components["powerlimiter"] = asBool(OPENDTU_FEATURE_POWERLIMITER);
    components["webapi_prometheus"] = asBool(OPENDTU_FEATURE_WEBAPI_PROMETHEUS);
    components["webapi_device"] = asBool(OPENDTU_FEATURE_WEBAPI_DEVICE);
    components["webapi_mqtt"] = asBool(OPENDTU_FEATURE_WEBAPI_MQTT);
    components["webapi_network"] = asBool(OPENDTU_FEATURE_WEBAPI_NETWORK);
    components["webapi_ntp"] = asBool(OPENDTU_FEATURE_WEBAPI_NTP);

    auto integrations = root["integrations"].to<JsonObject>();
    integrations["mqtt_hass"] = asBool(OPENDTU_FEATURE_MQTT_HASS);
    integrations["mqtt_tls_certinfo"] = asBool(OPENDTU_FEATURE_MQTT_TLS_CERTINFO);

    auto providers = root["providers"].to<JsonObject>();

    auto providersPowerMeter = providers["powermeter"].to<JsonObject>();
    providersPowerMeter["mqtt"] = asBool(OPENDTU_FEATURE_POWERMETER_PROVIDER_MQTT);
    providersPowerMeter["sdm"] = asBool(OPENDTU_FEATURE_POWERMETER_PROVIDER_SDM);
    providersPowerMeter["http_json"] = asBool(OPENDTU_FEATURE_POWERMETER_PROVIDER_HTTP_JSON);
    providersPowerMeter["serial_sml"] = asBool(OPENDTU_FEATURE_POWERMETER_PROVIDER_SERIAL_SML);
    providersPowerMeter["smahm2"] = asBool(OPENDTU_FEATURE_POWERMETER_PROVIDER_SMAHM2);
    providersPowerMeter["http_sml"] = asBool(OPENDTU_FEATURE_POWERMETER_PROVIDER_HTTP_SML);
    providersPowerMeter["modbus_udp_victron"] = asBool(OPENDTU_FEATURE_POWERMETER_PROVIDER_MODBUS_UDP_VICTRON);

    auto providersBattery = providers["battery"].to<JsonObject>();
    providersBattery["pylontech"] = asBool(OPENDTU_FEATURE_BATTERY_PROVIDER_PYLONTECH);
    providersBattery["jkbms"] = asBool(OPENDTU_FEATURE_BATTERY_PROVIDER_JKBMS);
    providersBattery["mqtt"] = asBool(OPENDTU_FEATURE_BATTERY_PROVIDER_MQTT);
    providersBattery["victron_smartshunt"] = asBool(OPENDTU_FEATURE_BATTERY_PROVIDER_VICTRON_SMARTSHUNT);
    providersBattery["pytes"] = asBool(OPENDTU_FEATURE_BATTERY_PROVIDER_PYTES);
    providersBattery["sbs"] = asBool(OPENDTU_FEATURE_BATTERY_PROVIDER_SBS);
    providersBattery["jbdbms"] = asBool(OPENDTU_FEATURE_BATTERY_PROVIDER_JBDBMS);
    providersBattery["zendure"] = asBool(OPENDTU_FEATURE_BATTERY_PROVIDER_ZENDURE);

    auto providersSolarCharger = providers["solarcharger"].to<JsonObject>();
    providersSolarCharger["vedirect"] = asBool(OPENDTU_FEATURE_SOLARCHARGER_PROVIDER_VEDIRECT);
    providersSolarCharger["mqtt"] = asBool(OPENDTU_FEATURE_SOLARCHARGER_PROVIDER_MQTT);

    auto providersGridCharger = providers["gridcharger"].to<JsonObject>();
    providersGridCharger["huawei"] = asBool(OPENDTU_FEATURE_GRIDCHARGER_PROVIDER_HUAWEI);
    providersGridCharger["trucki"] = asBool(OPENDTU_FEATURE_GRIDCHARGER_PROVIDER_TRUCKI);

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}
