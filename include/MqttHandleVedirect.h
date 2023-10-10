// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "VeDirectMpptController.h"
#include "Configuration.h"
#include <Arduino.h>

#ifndef VICTRON_PIN_RX
#define VICTRON_PIN_RX 22
#endif

#ifndef VICTRON_PIN_TX
#define VICTRON_PIN_TX 21
#endif

class MqttHandleVedirectClass {
public:
    void init();
    void loop();
private:

    VeDirectMpptController::veMpptStruct _kvFrame{};

    // point of time in millis() when updated values will be published
    uint32_t _nextPublishUpdatesOnly = 0;

    // point of time in millis() when all values will be published
    uint32_t _nextPublishFull = 1;

    bool _PublishFull;
};

extern MqttHandleVedirectClass MqttHandleVedirect;