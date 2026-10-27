// SPDX-License-Identifier: GPL-2.0-or-later
#include "modbus/sunspec/SunSpecImmediateControlsModel123.h"
#include "modbus/sunspec/SunSpecUtils.h"
#include <algorithm>

using SunSpec::NI_S;
using SunSpec::NI_U;

void SunSpecImmediateControlsModel123::fill(uint16_t* n, State const& state)
{
    std::fill(n, n + kLength, NI_U);

    n[0] = 0; // Conn_WinTms - connection switching is unsupported
    n[1] = 0; // Conn_RvrtTms
    n[2] = 1; // Conn - connected
    n[3] = state.PowerLimitRaw; // WMaxLimPct
    n[4] = 0; // WMaxLimPct_WinTms - immediate
    n[5] = state.RevertSeconds; // WMaxLimPct_RvrtTms
    n[6] = 0; // WMaxLimPct_RmpTms - immediate
    n[7] = state.Enabled ? 1 : 0; // WMaxLim_Ena

    // dbus-fronius reads exactly model-header offset 23 to discover this
    // scale factor (payload index 21).
    n[21] = static_cast<uint16_t>(kPowerLimitScaleFactor); // WMaxLimPct_SF
    n[22] = NI_S; // OutPFSet_SF
    n[23] = NI_S; // VArPct_SF
}

SunSpecImmediateControlsModel123::WriteResult
SunSpecImmediateControlsModel123::write(State& state, uint16_t startOffset,
                                        uint16_t const* values, size_t count,
                                        uint32_t nowMillis)
{
    WriteResult result;
    if (values == nullptr || count == 0) { return result; }

    auto const endOffset = static_cast<uint32_t>(startOffset) + count;
    if (startOffset < kPowerLimitOffset || endOffset > kPowerLimitEnableOffset + 1) {
        return result;
    }

    State next = state;
    bool limitWritten = false;
    bool enableWritten = false;

    for (size_t i = 0; i < count; ++i) {
        auto const offset = static_cast<uint16_t>(startOffset + i);
        auto const value = values[i];
        switch (offset) {
            case kPowerLimitOffset:
                if (value > kFullPowerRaw) { return result; }
                next.PowerLimitRaw = value;
                limitWritten = true;
                break;
            case kPowerLimitWindowOffset:
            case kPowerLimitRampOffset:
                // Accepted for SunSpec compatibility; limits are applied immediately.
                break;
            case kPowerLimitRevertOffset:
                next.RevertSeconds = value;
                break;
            case kPowerLimitEnableOffset:
                if (value > 1) { return result; }
                next.Enabled = value == 1;
                enableWritten = true;
                break;
            default:
                return result;
        }
    }

    next.LastRefreshMillis = nowMillis;
    result.Accepted = true;
    result.ApplyLimit = (next.Enabled && (limitWritten || enableWritten))
        || (enableWritten && state.Enabled && !next.Enabled);
    result.LimitPercent = next.Enabled
        ? static_cast<float>(next.PowerLimitRaw) / 100.0f
        : 100.0f;
    state = next;
    return result;
}

bool SunSpecImmediateControlsModel123::expire(State& state, uint32_t nowMillis)
{
    if (!state.Enabled || state.RevertSeconds == 0) { return false; }

    auto const timeoutMillis = static_cast<uint32_t>(state.RevertSeconds) * 1000U;
    if (nowMillis - state.LastRefreshMillis < timeoutMillis) { return false; }

    state.Enabled = false;
    state.PowerLimitRaw = kFullPowerRaw;
    return true;
}
