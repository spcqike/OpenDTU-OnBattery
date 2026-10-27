// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"

namespace InverterControlOwnership {

inline bool isGovernedByDpl(CONFIG_T const& config, uint64_t serial)
{
    if (!config.PowerLimiter.Enabled || serial == 0) { return false; }

    for (auto const& inverter : config.PowerLimiter.Inverters) {
        if (inverter.Serial == 0) { break; }
        if (inverter.Serial == serial) { return inverter.IsGoverned; }
    }
    return false;
}

inline bool isControlledByModbus(CONFIG_T const& config, uint64_t serial)
{
    if (!config.ModbusServer.Enabled || serial == 0) { return false; }

    for (auto const& inverter : config.ModbusServer.Inverter) {
        if (inverter.Serial == 0) { break; }
        if (inverter.Serial == serial) { return true; }
    }
    return false;
}

} // namespace InverterControlOwnership
