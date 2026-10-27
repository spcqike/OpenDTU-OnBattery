#include "modbus/sunspec/SunSpecImmediateControlsModel123.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

namespace {

bool near(float lhs, float rhs)
{
    return std::fabs(lhs - rhs) < 0.001f;
}

void testRegisterLayout()
{
    SunSpecImmediateControlsModel123::State state;
    uint16_t payload[SunSpecImmediateControlsModel123::kLength] = {};
    SunSpecImmediateControlsModel123::fill(payload, state);

    assert(payload[3] == 10000);
    assert(payload[7] == 0);
    assert(payload[21] == static_cast<uint16_t>(-2));
}

void testVictronLimitWriteAndReset()
{
    SunSpecImmediateControlsModel123::State state;
    // dbus-fronius writes model offsets 5..9 in one request:
    // WMaxLimPct, window, revert, ramp, enable.
    uint16_t const command[] = { 3750, 0, 60, 0, 1 };
    auto result = SunSpecImmediateControlsModel123::write(state, 5, command, 5, 1000);

    assert(result.Accepted);
    assert(result.ApplyLimit);
    assert(near(result.LimitPercent, 37.5f));
    assert(state.Enabled);
    assert(state.RevertSeconds == 60);

    uint16_t const reset[] = { 0 };
    result = SunSpecImmediateControlsModel123::write(state, 9, reset, 1, 2000);
    assert(result.Accepted);
    assert(result.ApplyLimit);
    assert(near(result.LimitPercent, 100.0f));
    assert(!state.Enabled);
}

void testValidationIsTransactional()
{
    SunSpecImmediateControlsModel123::State state;
    uint16_t const invalidLimit[] = { 10001, 0, 10, 0, 1 };
    auto result = SunSpecImmediateControlsModel123::write(state, 5, invalidLimit, 5, 1000);
    assert(!result.Accepted);
    assert(!state.Enabled);
    assert(state.PowerLimitRaw == 10000);

    uint16_t const readOnlyRegister[] = { 1 };
    result = SunSpecImmediateControlsModel123::write(state, 4, readOnlyRegister, 1, 1000);
    assert(!result.Accepted);
}

void testWatchdog()
{
    SunSpecImmediateControlsModel123::State state;
    uint16_t const command[] = { 5000, 0, 2, 0, 1 };
    auto result = SunSpecImmediateControlsModel123::write(state, 5, command, 5, 0xFFFFFF00U);
    assert(result.Accepted);

    assert(!SunSpecImmediateControlsModel123::expire(state, 0x00000200U));
    assert(SunSpecImmediateControlsModel123::expire(state, 0x00000800U));
    assert(!state.Enabled);
    assert(state.PowerLimitRaw == 10000);
}

} // namespace

int main()
{
    testRegisterLayout();
    testVictronLimitWriteAndReset();
    testValidationIsTransactional();
    testWatchdog();
    std::cout << "SunSpec immediate-controls tests passed\n";
    return 0;
}
