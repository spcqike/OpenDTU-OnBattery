// SPDX-License-Identifier: GPL-2.0-or-later
#include <powermeter/Controller.h>
#include <Configuration.h>
#include "FeatureFlags.h"
#include <LogHelper.h>
#if OPENDTU_FEATURE_POWERMETER_PROVIDER_HTTP_JSON
#include <powermeter/json/http/Provider.h>
#endif
#if OPENDTU_FEATURE_POWERMETER_PROVIDER_MQTT
#include <powermeter/json/mqtt/Provider.h>
#endif
#if OPENDTU_FEATURE_POWERMETER_PROVIDER_SDM
#include <powermeter/sdm/serial/Provider.h>
#endif
#if OPENDTU_FEATURE_POWERMETER_PROVIDER_HTTP_SML
#include <powermeter/sml/http/Provider.h>
#endif
#if OPENDTU_FEATURE_POWERMETER_PROVIDER_SERIAL_SML
#include <powermeter/sml/serial/Provider.h>
#endif
#if OPENDTU_FEATURE_POWERMETER_PROVIDER_SMAHM2
#include <powermeter/smahm/udp/Provider.h>
#endif
#if OPENDTU_FEATURE_POWERMETER_PROVIDER_MODBUS_UDP_VICTRON
#include <powermeter/modbus/udp/victron/Provider.h>
#endif

#undef TAG
static const char* TAG = "powerMeter";
static const char* SUBTAG = "Controller";

#if OPENDTU_FEATURE_POWERMETER
PowerMeters::Controller PowerMeter;
#endif

namespace PowerMeters {

void Controller::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&Controller::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();

    updateSettings();
}

void Controller::updateSettings()
{
    std::lock_guard<std::mutex> l(_mutex);

    if (_upProvider) { _upProvider.reset(); }

    auto const& pmcfg = Configuration.get().PowerMeter;

    if (!pmcfg.Enabled) { return; }

#if !OPENDTU_FEATURE_POWERMETER
    DTU_LOGW("Power meter component disabled at compile-time");
    return;
#endif

    switch(static_cast<Provider::Type>(pmcfg.Source)) {
#if OPENDTU_FEATURE_POWERMETER_PROVIDER_MQTT
        case Provider::Type::MQTT:
            _upProvider = std::make_unique<::PowerMeters::Json::Mqtt::Provider>(pmcfg.Mqtt);
            break;
#endif
#if OPENDTU_FEATURE_POWERMETER_PROVIDER_SDM
        case Provider::Type::SDM1PH:
            _upProvider = std::make_unique<::PowerMeters::Sdm::Serial::Provider>(
                    ::PowerMeters::Sdm::Serial::Provider::Phases::One, pmcfg.SerialSdm);
            break;
        case Provider::Type::SDM3PH:
            _upProvider = std::make_unique<::PowerMeters::Sdm::Serial::Provider>(
                    ::PowerMeters::Sdm::Serial::Provider::Phases::Three, pmcfg.SerialSdm);
            break;
#endif
#if OPENDTU_FEATURE_POWERMETER_PROVIDER_HTTP_JSON
        case Provider::Type::HTTP_JSON:
            _upProvider = std::make_unique<::PowerMeters::Json::Http::Provider>(pmcfg.HttpJson);
            break;
#endif
#if OPENDTU_FEATURE_POWERMETER_PROVIDER_SERIAL_SML
        case Provider::Type::SERIAL_SML:
            _upProvider = std::make_unique<::PowerMeters::Sml::Serial::Provider>();
            break;
#endif
#if OPENDTU_FEATURE_POWERMETER_PROVIDER_SMAHM2
        case Provider::Type::SMAHM2:
            _upProvider = std::make_unique<::PowerMeters::SmaHM::Udp::Provider>();
            break;
#endif
#if OPENDTU_FEATURE_POWERMETER_PROVIDER_HTTP_SML
        case Provider::Type::HTTP_SML:
            _upProvider = std::make_unique<::PowerMeters::Sml::Http::Provider>(pmcfg.HttpSml);
            break;
#endif
#if OPENDTU_FEATURE_POWERMETER_PROVIDER_MODBUS_UDP_VICTRON
        case Provider::Type::MODBUS_UDP_VICTRON:
            _upProvider = std::make_unique<::PowerMeters::Modbus::Udp::Victron::Provider>(pmcfg.UdpVictron);
            break;
#endif
        default:
            DTU_LOGW("Configured power meter source %u not available in this build", static_cast<unsigned>(pmcfg.Source));
            break;
    }

    if (!_upProvider) {
        return;
    }

    if (!_upProvider->init()) {
        _upProvider = nullptr;
    }
}

float Controller::getPowerTotal() const
{
    std::lock_guard<std::mutex> l(_mutex);
    if (!_upProvider) { return 0.0; }
    return _upProvider->getPowerTotal();
}

uint32_t Controller::getLastUpdate() const
{
    std::lock_guard<std::mutex> l(_mutex);
    if (!_upProvider) { return 0; }
    return _upProvider->getLastUpdate();
}

bool Controller::isDataValid() const
{
    std::lock_guard<std::mutex> l(_mutex);
    if (!_upProvider) { return false; }
    return _upProvider->isDataValid();
}

void Controller::loop()
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_upProvider) { return; }
    _upProvider->loop();

    auto const& pmcfg = Configuration.get().PowerMeter;
    // we don't need to republish data received from MQTT
    if (pmcfg.Source == static_cast<uint8_t>(Provider::Type::MQTT)) { return; }
    _upProvider->mqttLoop();
}

} // namespace PowerMeters
