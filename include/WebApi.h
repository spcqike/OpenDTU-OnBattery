// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "FeatureFlags.h"
#if OPENDTU_FEATURE_WEBAPI_DEVICE
#include "WebApi_device.h"
#endif
#include "WebApi_devinfo.h"
#include "WebApi_dtu.h"
#include "WebApi_errors.h"
#include "WebApi_eventlog.h"
#include "WebApi_features.h"
#include "WebApi_file.h"
#include "WebApi_firmware.h"
#include "WebApi_gridprofile.h"
#include "WebApi_i18n.h"
#include "WebApi_inverter.h"
#include "WebApi_limit.h"
#include "WebApi_logging.h"
#include "WebApi_maintenance.h"
#if OPENDTU_FEATURE_WEBAPI_MQTT
#include "WebApi_mqtt.h"
#endif
#if OPENDTU_FEATURE_WEBAPI_NETWORK
#include "WebApi_network.h"
#endif
#if OPENDTU_FEATURE_WEBAPI_NTP
#include "WebApi_ntp.h"
#endif
#include "WebApi_power.h"
#if OPENDTU_FEATURE_WEBAPI_PROMETHEUS
#include "WebApi_prometheus.h"
#endif
#include "WebApi_security.h"
#include "WebApi_sysstatus.h"
#if OPENDTU_FEATURE_WEBAPP
#include "WebApi_webapp.h"
#endif
#include "WebApi_ws_console.h"
#include "WebApi_ws_live.h"
#if OPENDTU_FEATURE_BATTERY
#include "WebApi_battery.h"
#include "WebApi_ws_battery.h"
#endif
#if OPENDTU_FEATURE_POWERMETER
#include "WebApi_powermeter.h"
#endif
#if OPENDTU_FEATURE_POWERLIMITER
#include "WebApi_powerlimiter.h"
#endif
#if OPENDTU_FEATURE_SOLARCHARGER
#include "WebApi_ws_solarcharger_live.h"
#include "WebApi_solarcharger.h"
#endif
#if OPENDTU_FEATURE_GRIDCHARGER
#include "WebApi_ws_gridcharger.h"
#include "WebApi_gridcharger.h"
#endif
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiClass {
public:
    WebApiClass();
    void init(Scheduler& scheduler);
    void reload();

    static bool checkCredentials(AsyncWebServerRequest* request);
    static bool checkCredentialsReadonly(AsyncWebServerRequest* request);

    static void sendTooManyRequests(AsyncWebServerRequest* request);

    static void writeConfig(JsonVariant& retMsg, const WebApiError code = WebApiError::GenericSuccess, const String& message = "Settings saved!");

    static bool parseRequestData(AsyncWebServerRequest* request, AsyncJsonResponse* response, JsonDocument& json_document);
    static uint64_t parseSerialFromRequest(AsyncWebServerRequest* request, String param_name = "inv");
    static bool sendJsonResponse(AsyncWebServerRequest* request, AsyncJsonResponse* response, const char* function, const uint16_t line);

private:
    AsyncWebServer _server;

#if OPENDTU_FEATURE_BATTERY
    WebApiBatteryClass _webApiBattery;
#endif
#if OPENDTU_FEATURE_WEBAPI_DEVICE
    WebApiDeviceClass _webApiDevice;
#endif
    WebApiDevInfoClass _webApiDevInfo;
    WebApiDtuClass _webApiDtu;
    WebApiEventlogClass _webApiEventlog;
    WebApiFeaturesClass _webApiFeatures;
    WebApiFileClass _webApiFile;
    WebApiFirmwareClass _webApiFirmware;
    WebApiGridProfileClass _webApiGridprofile;
    WebApiI18nClass _webApiI18n;
    WebApiInverterClass _webApiInverter;
    WebApiLimitClass _webApiLimit;
    WebApiLoggingClass _webApiLogging;
    WebApiMaintenanceClass _webApiMaintenance;
#if OPENDTU_FEATURE_WEBAPI_MQTT
    WebApiMqttClass _webApiMqtt;
#endif
#if OPENDTU_FEATURE_WEBAPI_NETWORK
    WebApiNetworkClass _webApiNetwork;
#endif
#if OPENDTU_FEATURE_WEBAPI_NTP
    WebApiNtpClass _webApiNtp;
#endif
    WebApiPowerClass _webApiPower;
#if OPENDTU_FEATURE_POWERMETER
    WebApiPowerMeterClass _webApiPowerMeter;
#endif
#if OPENDTU_FEATURE_POWERLIMITER
    WebApiPowerLimiterClass _webApiPowerLimiter;
#endif
#if OPENDTU_FEATURE_WEBAPI_PROMETHEUS
    WebApiPrometheusClass _webApiPrometheus;
#endif
    WebApiSecurityClass _webApiSecurity;
    WebApiSysstatusClass _webApiSysstatus;
#if OPENDTU_FEATURE_WEBAPP
    WebApiWebappClass _webApiWebapp;
#endif
    WebApiWsConsoleClass _webApiWsConsole;
    WebApiWsLiveClass _webApiWsLive;
#if OPENDTU_FEATURE_SOLARCHARGER
    WebApiWsSolarChargerLiveClass _webApiWsSolarChargerLive;
    WebApiSolarChargerlass _webApiSolarCharger;
#endif
#if OPENDTU_FEATURE_GRIDCHARGER
    WebApiGridChargerClass _webApiGridCharger;
    WebApiWsGridChargerLiveClass _webApiWsGridChargerLive;
#endif
#if OPENDTU_FEATURE_BATTERY
    WebApiWsBatteryLiveClass _webApiWsBatteryLive;
#endif
};

extern WebApiClass WebApi;
