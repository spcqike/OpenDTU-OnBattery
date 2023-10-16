// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */

#include "Battery.h"
#include "PowerMeter.h"
#include "PowerLimiter.h"
#include "Configuration.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "Huawei_can.h"
#include <VeDirectMpptController.h>
#include "MessageOutput.h"
#include <ctime>
#include <cmath>
#include <map>

PowerLimiterClass PowerLimiter;

void PowerLimiterClass::init() { }

std::string const& PowerLimiterClass::getStatusText(PowerLimiterClass::Status status)
{
    static const std::string missing =  "programmer error: missing status text";

    static const std::map<Status, const std::string> texts = {
        { Status::Initializing, "initializing (should not see me)" },
        { Status::DisabledByConfig, "disabled by configuration" },
        { Status::DisabledByMqtt, "disabled by MQTT" },
        { Status::WaitingForValidTimestamp, "waiting for valid date and time to be available" },
        { Status::PowerMeterDisabled, "no power meter is configured/enabled" },
        { Status::PowerMeterTimeout, "power meter readings are outdated" },
        { Status::PowerMeterPending, "waiting for sufficiently recent power meter reading" },
        { Status::InverterInvalid, "invalid inverter selection/configuration" },
        { Status::InverterChanged, "target inverter changed" },
        { Status::InverterOffline, "inverter is offline (polling enabled? radio okay?)" },
        { Status::InverterCommandsDisabled, "inverter configuration prohibits sending commands" },
        { Status::InverterLimitPending, "waiting for a power limit command to complete" },
        { Status::InverterPowerCmdPending, "waiting for a start/stop/restart command to complete" },
        { Status::InverterDevInfoPending, "waiting for inverter device information to be available" },
        { Status::InverterStatsPending, "waiting for sufficiently recent inverter data" },
        { Status::UnconditionalSolarPassthrough, "unconditionally passing through all solar power (MQTT override)" },
        { Status::NoVeDirect, "VE.Direct disabled, connection broken, or data outdated" },
        { Status::Settling, "waiting for the system to settle" },
        { Status::Stable, "the system is stable, the last power limit is still valid" },
    };

    auto iter = texts.find(status);
    if (iter == texts.end()) { return missing; }

    return iter->second;
}

void PowerLimiterClass::announceStatus(PowerLimiterClass::Status status)
{
    // this method is called with high frequency. print the status text if
    // the status changed since we last printed the text of another one.
    // otherwise repeat the info with a fixed interval.
    if (_lastStatus == status && millis() < _lastStatusPrinted + 10 * 1000) { return; }

    // after announcing once that the DPL is disabled by configuration, it
    // should just be silent while it is disabled.
    if (status == Status::DisabledByConfig && _lastStatus == status) { return; }

    MessageOutput.printf("[%11.3f] DPL: %s\r\n",
        static_cast<double>(millis())/1000, getStatusText(status).c_str());

    _lastStatus = status;
    _lastStatusPrinted = millis();
}

/**
 * returns true if the inverter state was changed or is about to change, i.e.,
 * if it is actually in need of a shutdown. returns false otherwise, i.e., the
 * inverter is already (assumed to be) shut down.
 */
bool PowerLimiterClass::shutdown(PowerLimiterClass::Status status)
{
    announceStatus(status);

    if (_inverter == nullptr || !_inverter->isProducing() ||
            (_shutdownTimeout > 0 && _shutdownTimeout < millis()) ) {
        // we are actually (already) done with shutting down the inverter,
        // or a shutdown attempt was initiated but it timed out.
        _inverter = nullptr;
        _shutdownTimeout = 0;
        return false;
    }

    if (!_inverter->isReachable()) { return true; } // retry later (until timeout)

    // retry shutdown for a maximum amount of time before giving up
    if (_shutdownTimeout == 0) { _shutdownTimeout = millis() + 10 * 1000; }

    auto lastLimitCommandState = _inverter->SystemConfigPara()->getLastLimitCommandSuccess();
    if (CMD_PENDING == lastLimitCommandState) { return true; }

    auto lastPowerCommandState = _inverter->PowerCommand()->getLastPowerCommandSuccess();
    if (CMD_PENDING == lastPowerCommandState) { return true; }

    CONFIG_T& config = Configuration.get();
    commitPowerLimit(_inverter, config.PowerLimiter_LowerPowerLimit, false);

    return true;
}

void PowerLimiterClass::loop()
{
    CONFIG_T const& config = Configuration.get();
    _verboseLogging = config.PowerLimiter_VerboseLogging;

    // we know that the Hoymiles library refuses to send any message to any
    // inverter until the system has valid time information. until then we can
    // do nothing, not even shutdown the inverter.
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5)) {
        return announceStatus(Status::WaitingForValidTimestamp);
    }

    if (_shutdownTimeout > 0) {
        // we transition from SHUTDOWN to OFF when we know the inverter was
        // shut down. until then, we retry shutting it down. in this case we
        // preserve the original status that lead to the decision to shut down.
        shutdown();
        return;
    }

    if (!config.PowerLimiter_Enabled) {
        shutdown(Status::DisabledByConfig);
        return;
    }

    if (Mode::Disabled == _mode) {
        shutdown(Status::DisabledByMqtt);
        return;
    }

    std::shared_ptr<InverterAbstract> currentInverter =
        Hoymiles.getInverterByPos(config.PowerLimiter_InverterId);

    // in case of (newly) broken configuration, shut down
    // the last inverter we worked with (if any)
    if (currentInverter == nullptr) {
        shutdown(Status::InverterInvalid);
        return;
    }

    // if the DPL is supposed to manage another inverter now, we first
    // shut down the previous one, if any. then we pick up the new one.
    if (_inverter != nullptr && _inverter->serial() != currentInverter->serial()) {
        shutdown(Status::InverterChanged);
        return;
    }

    // update our pointer as the configuration might have changed
    _inverter = currentInverter;

    // data polling is disabled or the inverter is deemed offline
    if (!_inverter->isReachable()) {
        return announceStatus(Status::InverterOffline);
    }

    // sending commands to the inverter is disabled
    if (!_inverter->getEnableCommands()) {
        return announceStatus(Status::InverterCommandsDisabled);
    }

    // concerns active power commands (power limits) only (also from web app or MQTT)
    auto lastLimitCommandState = _inverter->SystemConfigPara()->getLastLimitCommandSuccess();
    if (CMD_PENDING == lastLimitCommandState) {
        return announceStatus(Status::InverterLimitPending);
    }

    // concerns power commands (start, stop, restart) only (also from web app or MQTT)
    auto lastPowerCommandState = _inverter->PowerCommand()->getLastPowerCommandSuccess();
    if (CMD_PENDING == lastPowerCommandState) {
        return announceStatus(Status::InverterPowerCmdPending);
    }

    // a calculated power limit will always be limited to the reported
    // device's max power. that upper limit is only known after the first
    // DevInfoSimpleCommand succeeded.
    if (_inverter->DevInfo()->getMaxPower() <= 0) {
        return announceStatus(Status::InverterDevInfoPending);
    }

    if (Mode::UnconditionalFullSolarPassthrough == _mode) {
        // handle this mode of operation separately
        return unconditionalSolarPassthrough(_inverter);
    }

    // the normal mode of operation requires a valid
    // power meter reading to calculate a power limit
    if (!config.PowerMeter_Enabled) {
        shutdown(Status::PowerMeterDisabled);
        return;
    }

    if (millis() - PowerMeter.getLastPowerMeterUpdate() > (30 * 1000)) {
        shutdown(Status::PowerMeterTimeout);
        return;
    }

    // concerns both power limits and start/stop/restart commands and is
    // only updated if a respective response was received from the inverter
    auto lastUpdateCmd = std::max(
            _inverter->SystemConfigPara()->getLastUpdateCommand(),
            _inverter->PowerCommand()->getLastUpdateCommand());

    // wait for power meter and inverter stat updates after a settling phase
    auto settlingEnd = lastUpdateCmd + 3 * 1000;

    if (millis() < settlingEnd) { return announceStatus(Status::Settling); }

    if (_inverter->Statistics()->getLastUpdate() <= settlingEnd) {
        return announceStatus(Status::InverterStatsPending);
    }

    if (PowerMeter.getLastPowerMeterUpdate() <= settlingEnd) {
        return announceStatus(Status::PowerMeterPending);
    }

    // since _lastCalculation and _calculationBackoffMs are initialized to
    // zero, this test is passed the first time the condition is checked.
    if (millis() < (_lastCalculation + _calculationBackoffMs)) {
        return announceStatus(Status::Stable);
    }

    if (_verboseLogging) {
        MessageOutput.println("[DPL::loop] ******************* ENTER **********************");
    }

    // Check if next inverter restart time is reached
    if ((_nextInverterRestart > 1) && (_nextInverterRestart <= millis())) {
        MessageOutput.println("[DPL::loop] send inverter restart");
        _inverter->sendRestartControlRequest();
        calcNextInverterRestart();
    }

    // Check if NTP time is set and next inverter restart not calculated yet
    if ((config.PowerLimiter_RestartHour >= 0)  && (_nextInverterRestart == 0) ) {
        // check every 5 seconds
        if (_nextCalculateCheck < millis()) {
            struct tm timeinfo;
            if (getLocalTime(&timeinfo, 5)) {
                calcNextInverterRestart();
            } else {
                MessageOutput.println("[DPL::loop] inverter restart calculation: NTP not ready");
                _nextCalculateCheck += 5000;
            }
        }
    }

    // Battery charging cycle conditions
    // First we always disable discharge if the battery is empty
    if (isStopThresholdReached()) {
      // Disable battery discharge when empty
      _batteryDischargeEnabled = false;
    } else {
      // UI: Solar Passthrough Enabled -> false
      // Battery discharge can be enabled when start threshold is reached
      if (!config.PowerLimiter_SolarPassThroughEnabled && isStartThresholdReached()) {
        _batteryDischargeEnabled = true;
      }

      // UI: Solar Passthrough Enabled -> true && EMPTY_AT_NIGHT
      if (config.PowerLimiter_SolarPassThroughEnabled && config.PowerLimiter_BatteryDrainStategy == EMPTY_AT_NIGHT) {
        if(isStartThresholdReached()) {
            // In this case we should only discharge the battery as long it is above startThreshold
            _batteryDischargeEnabled = true;
        }
        else {
            // In this case we should only discharge the battery when there is no sunshine
            _batteryDischargeEnabled = !canUseDirectSolarPower();
        }
      }

      // UI: Solar Passthrough Enabled -> true && EMPTY_WHEN_FULL
      // Battery discharge can be enabled when start threshold is reached
      if (config.PowerLimiter_SolarPassThroughEnabled && isStartThresholdReached() && config.PowerLimiter_BatteryDrainStategy == EMPTY_WHEN_FULL) {
        _batteryDischargeEnabled = true;
      }
    }

    if (_verboseLogging) {
        MessageOutput.printf("[DPL::loop] battery interface %s, SoC: %d %%, StartTH: %d %%, StopTH: %d %%, SoC age: %d s\r\n",
                (config.Battery_Enabled?"enabled":"disabled"),
                Battery.getStats()->getSoC(),
                config.PowerLimiter_BatterySocStartThreshold,
                config.PowerLimiter_BatterySocStopThreshold,
                Battery.getStats()->getSoCAgeSeconds());

        float dcVoltage = _inverter->Statistics()->getChannelFieldValue(TYPE_DC, (ChannelNum_t)config.PowerLimiter_InverterChannelId, FLD_UDC);
        MessageOutput.printf("[DPL::loop] dcVoltage: %.2f V, loadCorrectedVoltage: %.2f V, StartTH: %.2f V, StopTH: %.2f V\r\n",
                dcVoltage, getLoadCorrectedVoltage(),
                config.PowerLimiter_VoltageStartThreshold,
                config.PowerLimiter_VoltageStopThreshold);

        MessageOutput.printf("[DPL::loop] StartTH reached: %s, StopTH reached: %s, inverter %s producing\r\n",
                (isStartThresholdReached()?"yes":"no"),
                (isStopThresholdReached()?"yes":"no"),
                (_inverter->isProducing()?"is":"is NOT"));

        MessageOutput.printf("[DPL::loop] SolarPT %s, Drain Strategy: %i, canUseDirectSolarPower: %s\r\n",
                (config.PowerLimiter_SolarPassThroughEnabled?"enabled":"disabled"),
                config.PowerLimiter_BatteryDrainStategy, (canUseDirectSolarPower()?"yes":"no"));

        MessageOutput.printf("[DPL::loop] battery discharging %s, PowerMeter: %d W, target consumption: %d W\r\n",
                (_batteryDischargeEnabled?"allowed":"prevented"),
                static_cast<int32_t>(round(PowerMeter.getPowerTotal())),
                config.PowerLimiter_TargetPowerConsumption);
    }

    // Calculate and set Power Limit (NOTE: might reset _inverter to nullptr!)
    int32_t newPowerLimit = calcPowerLimit(_inverter, canUseDirectSolarPower(), _batteryDischargeEnabled);
    bool limitUpdated = setNewPowerLimit(_inverter, newPowerLimit);

    if (_verboseLogging) {
        MessageOutput.printf("[DPL::loop] ******************* Leaving PL, calculated limit: %d W, requested limit: %d W (%s)\r\n",
                newPowerLimit, _lastRequestedPowerLimit,
                (limitUpdated?"updated from calculated":"kept last requested"));
    }

    _lastCalculation = millis();

    if (!limitUpdated) {
        // increase polling backoff if system seems to be stable
        _calculationBackoffMs = std::min<uint32_t>(1024, _calculationBackoffMs * 2);
        return announceStatus(Status::Stable);
    }

    _calculationBackoffMs = _calculationBackoffMsDefault;
}

/**
 * calculate the AC output power (limit) to set, such that the inverter uses
 * the given power on its DC side, i.e., adjust the power for the inverter's
 * efficiency.
 */
int32_t PowerLimiterClass::inverterPowerDcToAc(std::shared_ptr<InverterAbstract> inverter, int32_t dcPower)
{
    CONFIG_T& config = Configuration.get();

    float inverterEfficiencyPercent = inverter->Statistics()->getChannelFieldValue(
        TYPE_AC, CH0, FLD_EFF);

    // fall back to hoymiles peak efficiency as per datasheet if inverter
    // is currently not producing (efficiency is zero in that case)
    float inverterEfficiencyFactor = (inverterEfficiencyPercent > 0) ? inverterEfficiencyPercent/100 : 0.967;

    // account for losses between solar charger and inverter (cables, junctions...)
    float lossesFactor = 1.00 - static_cast<float>(config.PowerLimiter_SolarPassThroughLosses)/100;

    return dcPower * inverterEfficiencyFactor * lossesFactor;
}

/**
 * implements the "unconditional solar passthrough" mode of operation, which
 * can currently only be set using MQTT. in this mode of operation, the
 * inverter shall behave as if it was connected to the solar panels directly,
 * i.e., all solar power (and only solar power) is fed to the AC side,
 * independent from the power meter reading.
 */
void PowerLimiterClass::unconditionalSolarPassthrough(std::shared_ptr<InverterAbstract> inverter)
{
    CONFIG_T& config = Configuration.get();

    if (!config.Vedirect_Enabled || !VeDirectMppt.isDataValid()) {
        shutdown(Status::NoVeDirect);
        return;
    }

    int32_t solarPower = VeDirectMppt.veFrame.V * VeDirectMppt.veFrame.I;
    setNewPowerLimit(inverter, inverterPowerDcToAc(inverter, solarPower));
    announceStatus(Status::UnconditionalSolarPassthrough);
}

uint8_t PowerLimiterClass::getPowerLimiterState() {
    if (_inverter == nullptr || !_inverter->isReachable()) {
        return PL_UI_STATE_INACTIVE;
    }

    if (_inverter->isProducing() && _batteryDischargeEnabled) {
      return PL_UI_STATE_USE_SOLAR_AND_BATTERY;
    }

    if (_inverter->isProducing() && !_batteryDischargeEnabled) {
      return PL_UI_STATE_USE_SOLAR_ONLY;
    }

    if(!_inverter->isProducing()) {
       return PL_UI_STATE_CHARGING;
    }

    return PL_UI_STATE_INACTIVE;
}

int32_t PowerLimiterClass::getLastRequestedPowerLimit() {
	    return _lastRequestedPowerLimit;
}

bool PowerLimiterClass::canUseDirectSolarPower()
{
    CONFIG_T& config = Configuration.get();

    if (!config.PowerLimiter_SolarPassThroughEnabled
            || isBelowStopThreshold()
            || !config.Vedirect_Enabled
            || !VeDirectMppt.isDataValid()) {
        return false;
    }

    return VeDirectMppt.veFrame.PPV >= 20; // enough power?
}


// Logic table
// | Case # | batteryDischargeEnabled | solarPowerEnabled | useFullSolarPassthrough | Result                                                      |
// | 1      | false                   | false             | doesn't matter          | PL = 0                                                      |
// | 2      | false                   | true              | doesn't matter          | PL = Victron Power                                          |
// | 3      | true                    | doesn't matter    | false                   | PL = PowerMeter value (Battery can supply unlimited energy) |
// | 4      | true                    | false             | true                    | PL = PowerMeter value                                       |
// | 5      | true                    | true              | true                    | PL = max(PowerMeter value, Victron Power)                   |

int32_t PowerLimiterClass::calcPowerLimit(std::shared_ptr<InverterAbstract> inverter, bool solarPowerEnabled, bool batteryDischargeEnabled)
{
    CONFIG_T& config = Configuration.get();
    
    int32_t acPower = 0;
    int32_t newPowerLimit = round(PowerMeter.getPowerTotal(true));

    if (!solarPowerEnabled && !batteryDischargeEnabled) {
      // Case 1 - No energy sources available
      return 0;
    }

    if (config.PowerLimiter_IsInverterBehindPowerMeter) {
        // If the inverter the behind the power meter (part of measurement),
        // the produced power of this inverter has also to be taken into account.
        // We don't use FLD_PAC from the statistics, because that
        // data might be too old and unreliable.
        acPower = static_cast<int>(inverter->Statistics()->getChannelFieldValue(TYPE_AC, CH0, FLD_PAC)); 
        newPowerLimit += acPower;
    }

    // We're not trying to hit 0 exactly but take an offset into account
    // This means we never fully compensate the used power with the inverter 
    // Case 3
    newPowerLimit -= config.PowerLimiter_TargetPowerConsumption;

    // At this point we've calculated the required energy to compensate for household consumption. 
    // If the battery is enabled this can always be supplied since we assume that the battery can supply unlimited power
    // The next step is to determine if the Solar power as provided by the Victron charger
    // actually constrains or dictates another inverter power value
    int32_t adjustedVictronChargePower = inverterPowerDcToAc(inverter, getSolarChargePower());

    // Battery can be discharged and we should output max (Victron solar power || power meter value)
    if(batteryDischargeEnabled && useFullSolarPassthrough()) {
      // Case 5
      newPowerLimit = newPowerLimit > adjustedVictronChargePower ? newPowerLimit : adjustedVictronChargePower;
    } else {
      // We check if the PSU is on and disable the Power Limiter in this case. 
      // The PSU should reduce power or shut down first before the Power Limiter kicks in
      // The only case where this is not desired is if the battery is over the Full Solar Passthrough Threshold
      // In this case the Power Limiter should start. The PSU will shutdown when the Power Limiter is active
      if (HuaweiCan.getAutoPowerStatus()) {
        return 0;
      }
    }

    // We should use Victron solar power only (corrected by efficiency factor)
    if (solarPowerEnabled && !batteryDischargeEnabled) {
        // Case 2 - Limit power to solar power only
        if (_verboseLogging) {
            MessageOutput.printf("[DPL::loop] Consuming Solar Power Only -> adjustedVictronChargePower: %d W, newPowerLimit: %d W\r\n",
                adjustedVictronChargePower, newPowerLimit);
        }

        newPowerLimit = std::min(newPowerLimit, adjustedVictronChargePower);
    }

    return newPowerLimit;
}

void PowerLimiterClass::commitPowerLimit(std::shared_ptr<InverterAbstract> inverter, int32_t limit, bool enablePowerProduction)
{
    // disable power production as soon as possible.
    // setting the power limit is less important.
    if (!enablePowerProduction && inverter->isProducing()) {
        MessageOutput.println("[DPL::commitPowerLimit] Stopping inverter...");
        inverter->sendPowerControlRequest(false);
    }

    inverter->sendActivePowerControlRequest(static_cast<float>(limit),
            PowerLimitControlType::AbsolutNonPersistent);

    _lastRequestedPowerLimit = limit;
    _lastPowerLimitMillis = millis();

    // enable power production only after setting the desired limit,
    // such that an older, greater limit will not cause power spikes.
    if (enablePowerProduction && !inverter->isProducing()) {
        MessageOutput.println("[DPL::commitPowerLimit] Starting up inverter...");
        inverter->sendPowerControlRequest(true);
    }
}

/**
 * enforces limits and a hystersis on the requested power limit, after scaling
 * the power limit to the ratio of total and producing inverter channels.
 * commits the sanitized power limit. returns true if a limit update was
 * committed, false otherwise.
 */
bool PowerLimiterClass::setNewPowerLimit(std::shared_ptr<InverterAbstract> inverter, int32_t newPowerLimit)
{
    CONFIG_T& config = Configuration.get();

    // Stop the inverter if limit is below threshold.
    if (newPowerLimit < config.PowerLimiter_LowerPowerLimit) {
        // the status must not change outside of loop(). this condition is
        // communicated through log messages already.
        return shutdown();
    }

    // enforce configured upper power limit
    int32_t effPowerLimit = std::min(newPowerLimit, config.PowerLimiter_UpperPowerLimit);

    // scale the power limit by the amount of all inverter channels devided by
    // the amount of producing inverter channels. the inverters limit each of
    // the n channels to 1/n of the total power limit. scaling the power limit
    // ensures the total inverter output is what we are asking for.
    std::list<ChannelNum_t> dcChnls = inverter->Statistics()->getChannelsByType(TYPE_DC);
    int dcProdChnls = 0, dcTotalChnls = dcChnls.size();
    for (auto& c : dcChnls) {
        if (inverter->Statistics()->getChannelFieldValue(TYPE_DC, c, FLD_PDC) > 2.0) {
            dcProdChnls++;
        }
    }
    if ((dcProdChnls > 0) && (dcProdChnls != dcTotalChnls)) {
        MessageOutput.printf("[DPL::setNewPowerLimit] %d channels total, %d producing channels, scaling power limit\r\n",
                dcTotalChnls, dcProdChnls);
        effPowerLimit = round(effPowerLimit * static_cast<float>(dcTotalChnls) / dcProdChnls);
    }

    effPowerLimit = std::min<int32_t>(effPowerLimit, inverter->DevInfo()->getMaxPower());

    // Check if the new value is within the limits of the hysteresis
    auto diff = std::abs(effPowerLimit - _lastRequestedPowerLimit);
    auto hysteresis = config.PowerLimiter_TargetPowerConsumptionHysteresis;

    // (re-)send power limit in case the last was sent a long time ago. avoids
    // staleness in case a power limit update was not received by the inverter.
    auto ageMillis = millis() - _lastPowerLimitMillis;

    if (diff < hysteresis && ageMillis < 60 * 1000) {
        if (_verboseLogging) {
            MessageOutput.printf("[DPL::setNewPowerLimit] requested: %d W, last limit: %d W, diff: %d W, hysteresis: %d W, age: %ld ms\r\n",
                    newPowerLimit, _lastRequestedPowerLimit, diff, hysteresis, ageMillis);
        }
        return false;
    }

    if (_verboseLogging) {
        MessageOutput.printf("[DPL::setNewPowerLimit] requested: %d W, (re-)sending limit: %d W\r\n",
                newPowerLimit, effPowerLimit);
    }

    commitPowerLimit(inverter, effPowerLimit, true);
    return true;
}

int32_t PowerLimiterClass::getSolarChargePower()
{
    if (!canUseDirectSolarPower()) {
        return 0;
    }

    return VeDirectMppt.veFrame.V * VeDirectMppt.veFrame.I;
}

float PowerLimiterClass::getLoadCorrectedVoltage()
{
    if (!_inverter) {
        // there should be no need to call this method if no target inverter is known
        MessageOutput.println(F("DPL getLoadCorrectedVoltage: no inverter (programmer error)"));
        return 0.0;
    }

    CONFIG_T& config = Configuration.get();

    auto channel = static_cast<ChannelNum_t>(config.PowerLimiter_InverterChannelId);
    float acPower = _inverter->Statistics()->getChannelFieldValue(TYPE_AC, CH0, FLD_PAC);
    float dcVoltage = _inverter->Statistics()->getChannelFieldValue(TYPE_DC, channel, FLD_UDC);

    if (dcVoltage <= 0.0) {
        return 0.0;
    }

    return dcVoltage + (acPower * config.PowerLimiter_VoltageLoadCorrectionFactor);
}

bool PowerLimiterClass::testThreshold(float socThreshold, float voltThreshold,
        std::function<bool(float, float)> compare)
{
    CONFIG_T& config = Configuration.get();

    // prefer SoC provided through battery interface
    if (config.Battery_Enabled && socThreshold > 0.0
            && Battery.getStats()->isValid()
            && Battery.getStats()->getSoCAgeSeconds() < 60) {
              return compare(Battery.getStats()->getSoC(), socThreshold);
    }

    // use voltage threshold as fallback
    if (voltThreshold <= 0.0) { return false; }

    return compare(getLoadCorrectedVoltage(), voltThreshold);
}

bool PowerLimiterClass::isStartThresholdReached()
{
    CONFIG_T& config = Configuration.get();

    return testThreshold(
            config.PowerLimiter_BatterySocStartThreshold,
            config.PowerLimiter_VoltageStartThreshold,
            [](float a, float b) -> bool { return a >= b; }
    );
}

bool PowerLimiterClass::isStopThresholdReached()
{
    CONFIG_T& config = Configuration.get();

    return testThreshold(
            config.PowerLimiter_BatterySocStopThreshold,
            config.PowerLimiter_VoltageStopThreshold,
            [](float a, float b) -> bool { return a <= b; }
    );
}

bool PowerLimiterClass::isBelowStopThreshold()
{
    CONFIG_T& config = Configuration.get();

    return testThreshold(
            config.PowerLimiter_BatterySocStopThreshold,
            config.PowerLimiter_VoltageStopThreshold,
            [](float a, float b) -> bool { return a < b; }
    );
}

/// @brief calculate next inverter restart in millis
void PowerLimiterClass::calcNextInverterRestart()
{
    CONFIG_T& config = Configuration.get();

    // first check if restart is configured at all
    if (config.PowerLimiter_RestartHour < 0) {
        _nextInverterRestart = 1;
        MessageOutput.println("[DPL::calcNextInverterRestart] _nextInverterRestart disabled");
        return;
    }

    // read time from timeserver, if time is not synced then return
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5)) {
        // calculation first step is offset to next restart in minutes
        uint16_t dayMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
        uint16_t targetMinutes = config.PowerLimiter_RestartHour * 60;
        if (config.PowerLimiter_RestartHour > timeinfo.tm_hour) {
            // next restart is on the same day
            _nextInverterRestart = targetMinutes - dayMinutes;
        } else {
            // next restart is on next day
            _nextInverterRestart = 1440 - dayMinutes + targetMinutes;
        }
        if (_verboseLogging) {
            MessageOutput.printf("[DPL::calcNextInverterRestart] Localtime read %d %d / configured RestartHour %d\r\n", timeinfo.tm_hour, timeinfo.tm_min, config.PowerLimiter_RestartHour);
            MessageOutput.printf("[DPL::calcNextInverterRestart] dayMinutes %d / targetMinutes %d\r\n", dayMinutes, targetMinutes);
            MessageOutput.printf("[DPL::calcNextInverterRestart] next inverter restart in %d minutes\r\n", _nextInverterRestart);
        }
        // then convert unit for next restart to milliseconds and add current uptime millis()
        _nextInverterRestart *= 60000;
        _nextInverterRestart += millis();
    } else {
        MessageOutput.println("[DPL::calcNextInverterRestart] getLocalTime not successful, no calculation");
        _nextInverterRestart = 0;
    }
    MessageOutput.printf("[DPL::calcNextInverterRestart] _nextInverterRestart @ %d millis\r\n", _nextInverterRestart);
}

bool PowerLimiterClass::useFullSolarPassthrough()
{
    CONFIG_T& config = Configuration.get();

    // We only do full solar PT if general solar PT is enabled
    if(!config.PowerLimiter_SolarPassThroughEnabled) {
      return false;
    }

    if (testThreshold(config.PowerLimiter_FullSolarPassThroughSoc,
                      config.PowerLimiter_FullSolarPassThroughStartVoltage,
                      [](float a, float b) -> bool { return a >= b; })) {
        _fullSolarPassThroughEnabled = true;
    }

    if (testThreshold(config.PowerLimiter_FullSolarPassThroughSoc,
                      config.PowerLimiter_FullSolarPassThroughStopVoltage,
                      [](float a, float b) -> bool { return a < b; })) {
        _fullSolarPassThroughEnabled = false;
    }

    return _fullSolarPassThroughEnabled;
}
