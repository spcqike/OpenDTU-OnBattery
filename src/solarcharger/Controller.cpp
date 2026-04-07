// SPDX-License-Identifier: GPL-2.0-or-later
#include <Configuration.h>
#include "FeatureFlags.h"
#include <MqttSettings.h>
#include <solarcharger/Controller.h>
#include <solarcharger/DummyStats.h>
#if OPENDTU_FEATURE_SOLARCHARGER_PROVIDER_VEDIRECT
#include <solarcharger/victron/Provider.h>
#endif
#if OPENDTU_FEATURE_SOLARCHARGER_PROVIDER_MQTT
#include <solarcharger/mqtt/Provider.h>
#endif
#include <LogHelper.h>

#undef TAG
static const char* TAG = "solarCharger";
static const char* SUBTAG = "Controller";

#if OPENDTU_FEATURE_SOLARCHARGER
SolarChargers::Controller SolarCharger;
#endif

namespace SolarChargers {

void Controller::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&Controller::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();

    this->updateSettings();
}

void Controller::updateSettings()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_upProvider) {
        _upProvider->deinit();
        _upProvider = nullptr;
    }

    auto const& config = Configuration.get();
    if (!config.SolarCharger.Enabled) { return; }

    switch (config.SolarCharger.Provider) {
#if OPENDTU_FEATURE_SOLARCHARGER_PROVIDER_VEDIRECT
        case SolarChargerProviderType::VEDIRECT:
            _upProvider = std::make_unique<::SolarChargers::Victron::Provider>();
            break;
#endif
#if OPENDTU_FEATURE_SOLARCHARGER_PROVIDER_MQTT
        case SolarChargerProviderType::MQTT:
            _upProvider = std::make_unique<::SolarChargers::Mqtt::Provider>();
            break;
#endif
        default:
            DTU_LOGW("Configured solar charger provider %u not available in this build", static_cast<unsigned>(config.SolarCharger.Provider));
            return;
    }

    if (!_upProvider->init()) { _upProvider = nullptr; }

    _forcePublishSensors = true;
}

std::shared_ptr<Stats const> Controller::getStats() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upProvider) {
        static auto sspDummyStats = std::make_shared<DummyStats>();
        return sspDummyStats;
    }

    return _upProvider->getStats();
}

void Controller::loop()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upProvider) { return; }

    _upProvider->loop();

    // TODO(schlimmchen): this cannot make sure that transient
    // connection problems are actually always noticed.
    if (!MqttSettings.getConnected()) {
        _forcePublishSensors = true;
        return;
    }

    _upProvider->getStats()->mqttLoop();

    auto const& config = Configuration.get();
    if (!config.Mqtt.Hass.Enabled) { return; }

    _upProvider->getStats()->mqttPublishSensors(_forcePublishSensors);

    _forcePublishSensors = false;
}

} // namespace SolarChargers
