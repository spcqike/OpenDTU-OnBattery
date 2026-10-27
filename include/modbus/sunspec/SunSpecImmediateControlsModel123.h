// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstddef>
#include <cstdint>

// SunSpec model 123: Immediate Inverter Controls.
//
// Only the active-power limit window used by Victron dbus-fronius is writable.
// Unsupported controls remain present as "not implemented" so the model keeps
// its standard layout while avoiding accidental reactive-power/state commands.
class SunSpecImmediateControlsModel123 {
public:
    static constexpr uint16_t kLength = 24;
    static constexpr int16_t kPowerLimitScaleFactor = -2; // 0.01 % per register unit
    static constexpr uint16_t kFullPowerRaw = 10000;

    // Offsets include the two-register model header because Modbus clients use
    // addresses relative to the model ID register.
    static constexpr uint16_t kPowerLimitOffset = 5;
    static constexpr uint16_t kPowerLimitWindowOffset = 6;
    static constexpr uint16_t kPowerLimitRevertOffset = 7;
    static constexpr uint16_t kPowerLimitRampOffset = 8;
    static constexpr uint16_t kPowerLimitEnableOffset = 9;

    struct State {
        uint16_t PowerLimitRaw = kFullPowerRaw;
        uint16_t RevertSeconds = 0;
        bool Enabled = false;
        uint32_t LastRefreshMillis = 0;
    };

    struct WriteResult {
        bool Accepted = false;
        bool ApplyLimit = false;
        float LimitPercent = 100.0f;
    };

    static uint16_t id() { return 123; }
    static uint16_t length() { return kLength; }

    static void fill(uint16_t* payload, State const& state);

    // Apply an FC 0x10 write. startOffset is relative to the model ID register.
    // Only the contiguous WMaxLimPct..WMaxLim_Ena window is accepted.
    static WriteResult write(State& state, uint16_t startOffset,
                             uint16_t const* values, size_t count,
                             uint32_t nowMillis);

    // Disables an expired limit and asks the caller to restore 100% output.
    static bool expire(State& state, uint32_t nowMillis);
};
