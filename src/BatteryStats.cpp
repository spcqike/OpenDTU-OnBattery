// SPDX-License-Identifier: GPL-2.0-or-later
#include <vector>
#include <algorithm>
#include "BatteryStats.h"
#include "Configuration.h"
#include "MqttSettings.h"
#include "JkBmsDataPoints.h"

template<typename T>
void BatteryStats::addLiveViewValue(JsonVariant& root, std::string const& name,
    T&& value, std::string const& unit, uint8_t precision) const
{
    auto jsonValue = root["values"][name];
    jsonValue["v"] = value;
    jsonValue["u"] = unit;
    jsonValue["d"] = precision;
}

void BatteryStats::addLiveViewText(JsonVariant& root, std::string const& name,
    std::string const& text) const
{
    root["values"][name] = text;
}

void BatteryStats::addLiveViewWarning(JsonVariant& root, std::string const& name,
    bool warning) const
{
    if (!warning) { return; }
    root["issues"][name] = 1;
}

void BatteryStats::addLiveViewAlarm(JsonVariant& root, std::string const& name,
    bool alarm) const
{
    if (!alarm) { return; }
    root["issues"][name] = 2;
}

void BatteryStats::getLiveViewData(JsonVariant& root) const
{
    root[F("manufacturer")] = _manufacturer;
    root[F("data_age")] = getAgeSeconds();

    addLiveViewValue(root, "SoC", _SoC, "%", 0);
}

void PylontechBatteryStats::getLiveViewData(JsonVariant& root) const
{
    BatteryStats::getLiveViewData(root);

    // values go into the "Status" card of the web application
    addLiveViewValue(root, "chargeVoltage", _chargeVoltage, "V", 1);
    addLiveViewValue(root, "chargeCurrentLimitation", _chargeCurrentLimitation, "A", 1);
    addLiveViewValue(root, "dischargeCurrentLimitation", _dischargeCurrentLimitation, "A", 1);
    addLiveViewValue(root, "stateOfHealth", _stateOfHealth, "%", 0);
    addLiveViewValue(root, "voltage", _voltage, "V", 2);
    addLiveViewValue(root, "current", _current, "A", 1);
    addLiveViewValue(root, "temperature", _temperature, "°C", 1);

    addLiveViewText(root, "chargeEnabled", (_chargeEnabled?"yes":"no"));
    addLiveViewText(root, "dischargeEnabled", (_dischargeEnabled?"yes":"no"));
    addLiveViewText(root, "chargeImmediately", (_chargeImmediately?"yes":"no"));

    // alarms and warnings go into the "Issues" card of the web application
    addLiveViewWarning(root, "highCurrentDischarge", _warningHighCurrentDischarge);
    addLiveViewAlarm(root, "overCurrentDischarge", _alarmOverCurrentDischarge);

    addLiveViewWarning(root, "highCurrentCharge", _warningHighCurrentCharge);
    addLiveViewAlarm(root, "overCurrentCharge", _alarmOverCurrentCharge);

    addLiveViewWarning(root, "lowTemperature", _warningLowTemperature);
    addLiveViewAlarm(root, "underTemperature", _alarmUnderTemperature);

    addLiveViewWarning(root, "highTemperature", _warningHighTemperature);
    addLiveViewAlarm(root, "overTemperature", _alarmOverTemperature);

    addLiveViewWarning(root, "lowVoltage", _warningLowVoltage);
    addLiveViewAlarm(root, "underVoltage", _alarmUnderVoltage);

    addLiveViewWarning(root, "highVoltage", _warningHighVoltage);
    addLiveViewAlarm(root, "overVoltage", _alarmOverVoltage);

    addLiveViewWarning(root, "bmsInternal", _warningBmsInternal);
    addLiveViewAlarm(root, "bmsInternal", _alarmBmsInternal);
}

void JkBmsBatteryStats::getLiveViewData(JsonVariant& root) const
{
    BatteryStats::getLiveViewData(root);

    using Label = JkBms::DataPointLabel;

    auto oVoltage = _dataPoints.get<Label::BatteryVoltageMilliVolt>();
    if (oVoltage.has_value()) {
        addLiveViewValue(root, "voltage",
                static_cast<float>(*oVoltage) / 1000, "V", 2);
    }

    auto oCurrent = _dataPoints.get<Label::BatteryCurrentMilliAmps>();
    if (oCurrent.has_value()) {
        addLiveViewValue(root, "current",
                static_cast<float>(*oCurrent) / 1000, "A", 2);
    }

    auto oTemperature = _dataPoints.get<Label::BatteryTempOneCelsius>();
    if (oTemperature.has_value()) {
        addLiveViewValue(root, "temperature", *oTemperature, "°C", 0);
    }
}

void BatteryStats::mqttPublish() const
{
    MqttSettings.publish(F("battery/manufacturer"), _manufacturer);
    MqttSettings.publish(F("battery/dataAge"), String(getAgeSeconds()));
    MqttSettings.publish(F("battery/stateOfCharge"), String(_SoC));
}

void PylontechBatteryStats::mqttPublish() const
{
    BatteryStats::mqttPublish();

    MqttSettings.publish(F("battery/settings/chargeVoltage"), String(_chargeVoltage));
    MqttSettings.publish(F("battery/settings/chargeCurrentLimitation"), String(_chargeCurrentLimitation));
    MqttSettings.publish(F("battery/settings/dischargeCurrentLimitation"), String(_dischargeCurrentLimitation));
    MqttSettings.publish(F("battery/stateOfHealth"), String(_stateOfHealth));
    MqttSettings.publish(F("battery/voltage"), String(_voltage));
    MqttSettings.publish(F("battery/current"), String(_current));
    MqttSettings.publish(F("battery/temperature"), String(_temperature));
    MqttSettings.publish(F("battery/alarm/overCurrentDischarge"), String(_alarmOverCurrentDischarge));
    MqttSettings.publish(F("battery/alarm/overCurrentCharge"), String(_alarmOverCurrentCharge));
    MqttSettings.publish(F("battery/alarm/underTemperature"), String(_alarmUnderTemperature));
    MqttSettings.publish(F("battery/alarm/overTemperature"), String(_alarmOverTemperature));
    MqttSettings.publish(F("battery/alarm/underVoltage"), String(_alarmUnderVoltage));
    MqttSettings.publish(F("battery/alarm/overVoltage"), String(_alarmOverVoltage));
    MqttSettings.publish(F("battery/alarm/bmsInternal"), String(_alarmBmsInternal));
    MqttSettings.publish(F("battery/warning/highCurrentDischarge"), String(_warningHighCurrentDischarge));
    MqttSettings.publish(F("battery/warning/highCurrentCharge"), String(_warningHighCurrentCharge));
    MqttSettings.publish(F("battery/warning/lowTemperature"), String(_warningLowTemperature));
    MqttSettings.publish(F("battery/warning/highTemperature"), String(_warningHighTemperature));
    MqttSettings.publish(F("battery/warning/lowVoltage"), String(_warningLowVoltage));
    MqttSettings.publish(F("battery/warning/highVoltage"), String(_warningHighVoltage));
    MqttSettings.publish(F("battery/warning/bmsInternal"), String(_warningBmsInternal));
    MqttSettings.publish(F("battery/charging/chargeEnabled"), String(_chargeEnabled));
    MqttSettings.publish(F("battery/charging/dischargeEnabled"), String(_dischargeEnabled));
    MqttSettings.publish(F("battery/charging/chargeImmediately"), String(_chargeImmediately));
}

void JkBmsBatteryStats::mqttPublish() const
{
    BatteryStats::mqttPublish();

    using Label = JkBms::DataPointLabel;

    static std::vector<Label> mqttSkip = {
        Label::CellsMilliVolt, // complex data format
        Label::ModificationPassword, // sensitive data
        Label::BatterySoCPercent // already published by base class
    };

    CONFIG_T& config = Configuration.get();

    // publish all topics every minute, unless the retain flag is enabled
    bool fullPublish = _lastFullMqttPublish + 60 * 1000 < millis();
    fullPublish &= !config.Mqtt_Retain;

    for (auto iter = _dataPoints.cbegin(); iter != _dataPoints.cend(); ++iter) {
        // skip data points that did not change since last published
        if (!fullPublish && iter->second.getTimestamp() < _lastMqttPublish) { continue; }

        auto skipMatch = std::find(mqttSkip.begin(), mqttSkip.end(), iter->first);
        if (skipMatch != mqttSkip.end()) { continue; }

        String topic((std::string("battery/") + iter->second.getLabelText()).c_str());
        MqttSettings.publish(topic, iter->second.getValueText().c_str());
    }

    _lastMqttPublish = millis();
    if (fullPublish) { _lastFullMqttPublish = _lastMqttPublish; }
}

void JkBmsBatteryStats::updateFrom(JkBms::DataPointContainer const& dp)
{
    using Label = JkBms::DataPointLabel;

    _manufacturer = "JKBMS";
    auto oProductId = dp.get<Label::ProductId>();
    if (oProductId.has_value()) {
        _manufacturer = oProductId->c_str();
        auto pos = oProductId->rfind("JK");
        if (pos != std::string::npos) {
            _manufacturer = oProductId->substr(pos).c_str();
        }
    }

    auto oSoCValue = dp.get<Label::BatterySoCPercent>();
    if (oSoCValue.has_value()) {
        _SoC = *oSoCValue;
        auto oSoCDataPoint = dp.getDataPointFor<Label::BatterySoCPercent>();
        _lastUpdateSoC = oSoCDataPoint->getTimestamp();
    }

    _dataPoints.updateFrom(dp);

    _lastUpdate = millis();
}

void VictronSmartShuntStats::updateFrom(VeDirectShuntController::veShuntStruct const& shuntData) {
    _SoC = shuntData.SOC / 10;
    _voltage = shuntData.V;
    _current = shuntData.I;
    _modelName = VeDirectShunt.getPidAsString(shuntData.PID);
    _chargeCycles = shuntData.H4;
    _timeToGo = shuntData.TTG / 60;
    _chargedEnergy = shuntData.H18 / 100;
    _dischargedEnergy = shuntData.H17 / 100;
    _manufacturer = "Victron " + _modelName;
    
    // shuntData.AR is a bitfield, so we need to check each bit individually
    _alarmLowVoltage = shuntData.AR & 1;
    _alarmHighVoltage = shuntData.AR & 2;
    _alarmLowSOC = shuntData.AR & 4;
    _alarmLowTemperature = shuntData.AR & 32;
    _alarmHighTemperature = shuntData.AR & 64;

    _lastUpdate = VeDirectShunt.getLastUpdate();
    _lastUpdateSoC = VeDirectShunt.getLastUpdate();
}

void VictronSmartShuntStats::getLiveViewData(JsonVariant& root) const {
    BatteryStats::getLiveViewData(root);

    // values go into the "Status" card of the web application
    addLiveViewValue(root, "voltage", _voltage, "V", 2);
    addLiveViewValue(root, "current", _current, "A", 1);   
    addLiveViewValue(root, "chargeCycles", _chargeCycles, "", 0);
    addLiveViewValue(root, "chargedEnergy", _chargedEnergy, "KWh", 1);
    addLiveViewValue(root, "dischargedEnergy", _dischargedEnergy, "KWh", 1);
    
    addLiveViewAlarm(root, "lowVoltage", _alarmLowVoltage);
    addLiveViewAlarm(root, "highVoltage", _alarmHighVoltage);
    addLiveViewAlarm(root, "lowSOC", _alarmLowSOC);
    addLiveViewAlarm(root, "lowTemperature", _alarmLowTemperature);
    addLiveViewAlarm(root, "highTemperature", _alarmHighTemperature);     
}

void VictronSmartShuntStats::mqttPublish() const {
    BatteryStats::mqttPublish();

    MqttSettings.publish(F("battery/voltage"), String(_voltage));
    MqttSettings.publish(F("battery/current"), String(_current));
    MqttSettings.publish(F("battery/chargeCycles"), String(_chargeCycles));
    MqttSettings.publish(F("battery/chargedEnergy"), String(_chargedEnergy));
    MqttSettings.publish(F("battery/dischargedEnergy"), String(_dischargedEnergy));
}
