// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <espMqttClient.h>

class MqttHandlePowerLimiterClass {
public:
    void init();
    void loop();

private:
    void onCmdMode(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total);

    uint32_t _lastPublishStats;
    uint32_t _lastPublish;

};

extern MqttHandlePowerLimiterClass MqttHandlePowerLimiter;