# SPDX-License-Identifier: GPL-2.0-or-later
#
# Compile-time feature aware source pruning for PlatformIO.
# This script excludes source trees/files for disabled components/providers
# to keep build output and firmware size deterministic for lite profiles.
#
Import("env")
import re


def _defines_to_map(cppdefines):
    result = {}
    for define in cppdefines:
        if isinstance(define, tuple):
            if len(define) >= 2:
                key, value = define[0], define[1]
            elif len(define) == 1:
                key, value = define[0], "1"
            else:
                continue
            result[str(key)] = str(value)
        else:
            result[str(define)] = "1"
    return result


def _as_bool(value):
    if isinstance(value, bool):
        return value

    text = str(value).strip().lower()
    return text not in ("0", "false", "off", "no", "")


def _is_enabled(macros, name, default=True):
    if name not in macros:
        return default
    return _as_bool(macros[name])


def _explicit_enabled(macros, name):
    return name in macros and _as_bool(macros[name])


def _add_excludes(excludes, patterns):
    for pattern in patterns:
        item = "-<{}>".format(pattern)
        if item not in excludes:
            excludes.append(item)


macros = _defines_to_map(env.get("CPPDEFINES", []))
env_name = env.get("PIOENV", "<unknown>")

# Some PlatformIO phases provide only partial CPPDEFINES here.
# Merge from resolved BUILD_FLAGS first, then explicit project build_flags.
try:
    parsed_flags = env.ParseFlags(env.subst("$BUILD_FLAGS"))
    macros.update(_defines_to_map(parsed_flags.get("CPPDEFINES", [])))
except Exception as ex:
    print("WARNING: feature_src_filter: unable to parse BUILD_FLAGS for env '{}': {}".format(env_name, ex))

# Merge explicit -D flags from project build_flags as fallback.
for flag in env.GetProjectOption("build_flags", []):
    if not isinstance(flag, str):
        continue
    match = re.match(r"^-D([A-Za-z_][A-Za-z0-9_]*)(?:=(.*))?$", flag.strip())
    if not match:
        continue
    key = match.group(1)
    value = match.group(2) if match.group(2) is not None else "1"
    macros[key] = value

component_enabled = {
    "battery": _is_enabled(macros, "OPENDTU_FEATURE_BATTERY", True),
    "powermeter": _is_enabled(macros, "OPENDTU_FEATURE_POWERMETER", True),
    "solarcharger": _is_enabled(macros, "OPENDTU_FEATURE_SOLARCHARGER", True),
    "gridcharger": _is_enabled(macros, "OPENDTU_FEATURE_GRIDCHARGER", True),
    "powerlimiter": _is_enabled(macros, "OPENDTU_FEATURE_POWERLIMITER", True),
    "display": _is_enabled(macros, "OPENDTU_FEATURE_DISPLAY", True),
    "mqtt_hass": _is_enabled(macros, "OPENDTU_FEATURE_MQTT_HASS", True),
    "webapi_device": _is_enabled(macros, "OPENDTU_FEATURE_WEBAPI_DEVICE", True),
    "webapi_mqtt": _is_enabled(macros, "OPENDTU_FEATURE_WEBAPI_MQTT", True),
    "webapi_network": _is_enabled(macros, "OPENDTU_FEATURE_WEBAPI_NETWORK", True),
    "webapi_ntp": _is_enabled(macros, "OPENDTU_FEATURE_WEBAPI_NTP", True),
    "webapi_prometheus": _is_enabled(macros, "OPENDTU_FEATURE_WEBAPI_PROMETHEUS", True),
}

provider_component_map = {
    "OPENDTU_FEATURE_BATTERY_PROVIDER_PYLONTECH": "battery",
    "OPENDTU_FEATURE_BATTERY_PROVIDER_JKBMS": "battery",
    "OPENDTU_FEATURE_BATTERY_PROVIDER_MQTT": "battery",
    "OPENDTU_FEATURE_BATTERY_PROVIDER_VICTRON_SMARTSHUNT": "battery",
    "OPENDTU_FEATURE_BATTERY_PROVIDER_PYTES": "battery",
    "OPENDTU_FEATURE_BATTERY_PROVIDER_SBS": "battery",
    "OPENDTU_FEATURE_BATTERY_PROVIDER_JBDBMS": "battery",
    "OPENDTU_FEATURE_BATTERY_PROVIDER_ZENDURE": "battery",
    "OPENDTU_FEATURE_POWERMETER_PROVIDER_MQTT": "powermeter",
    "OPENDTU_FEATURE_POWERMETER_PROVIDER_SDM": "powermeter",
    "OPENDTU_FEATURE_POWERMETER_PROVIDER_HTTP_JSON": "powermeter",
    "OPENDTU_FEATURE_POWERMETER_PROVIDER_SERIAL_SML": "powermeter",
    "OPENDTU_FEATURE_POWERMETER_PROVIDER_SMAHM2": "powermeter",
    "OPENDTU_FEATURE_POWERMETER_PROVIDER_HTTP_SML": "powermeter",
    "OPENDTU_FEATURE_POWERMETER_PROVIDER_MODBUS_UDP_VICTRON": "powermeter",
    "OPENDTU_FEATURE_SOLARCHARGER_PROVIDER_VEDIRECT": "solarcharger",
    "OPENDTU_FEATURE_SOLARCHARGER_PROVIDER_MQTT": "solarcharger",
    "OPENDTU_FEATURE_GRIDCHARGER_PROVIDER_HUAWEI": "gridcharger",
    "OPENDTU_FEATURE_GRIDCHARGER_PROVIDER_TRUCKI": "gridcharger",
}

provider_enabled = {}
for provider, component in provider_component_map.items():
    parent_enabled = component_enabled[component]
    current_enabled = _is_enabled(macros, provider, True) and parent_enabled
    provider_enabled[provider] = current_enabled

    if not parent_enabled and _explicit_enabled(macros, provider):
        print(
            "WARNING: {}=1 but parent component '{}' is disabled in env '{}'; provider is effectively disabled.".format(
                provider, component, env_name
            )
        )

excludes = []

# Component-level pruning
if not component_enabled["battery"]:
    _add_excludes(excludes, ["battery/**", "WebApi_battery.cpp", "WebApi_ws_battery.cpp"])
else:
    if not provider_enabled["OPENDTU_FEATURE_BATTERY_PROVIDER_PYLONTECH"]:
        _add_excludes(excludes, ["battery/pylontech/**"])
    if not provider_enabled["OPENDTU_FEATURE_BATTERY_PROVIDER_JKBMS"]:
        _add_excludes(excludes, ["battery/jkbms/**"])
    if not provider_enabled["OPENDTU_FEATURE_BATTERY_PROVIDER_MQTT"]:
        _add_excludes(excludes, ["battery/mqtt/**"])
    if not provider_enabled["OPENDTU_FEATURE_BATTERY_PROVIDER_VICTRON_SMARTSHUNT"]:
        _add_excludes(excludes, ["battery/victronsmartshunt/**"])
    if not provider_enabled["OPENDTU_FEATURE_BATTERY_PROVIDER_PYTES"]:
        _add_excludes(excludes, ["battery/pytes/**"])
    if not provider_enabled["OPENDTU_FEATURE_BATTERY_PROVIDER_SBS"]:
        _add_excludes(excludes, ["battery/sbs/**"])
    if not provider_enabled["OPENDTU_FEATURE_BATTERY_PROVIDER_JBDBMS"]:
        _add_excludes(excludes, ["battery/jbdbms/**"])
    if not provider_enabled["OPENDTU_FEATURE_BATTERY_PROVIDER_ZENDURE"]:
        _add_excludes(excludes, ["battery/zendure/**"])

if not component_enabled["powermeter"]:
    _add_excludes(excludes, ["powermeter/**", "WebApi_powermeter.cpp"])
else:
    if not provider_enabled["OPENDTU_FEATURE_POWERMETER_PROVIDER_MQTT"]:
        _add_excludes(excludes, ["powermeter/json/mqtt/**"])
    if not provider_enabled["OPENDTU_FEATURE_POWERMETER_PROVIDER_SDM"]:
        _add_excludes(excludes, ["powermeter/sdm/**"])
    if not provider_enabled["OPENDTU_FEATURE_POWERMETER_PROVIDER_HTTP_JSON"]:
        _add_excludes(excludes, ["powermeter/json/http/**"])
    if not provider_enabled["OPENDTU_FEATURE_POWERMETER_PROVIDER_SERIAL_SML"]:
        _add_excludes(excludes, ["powermeter/sml/serial/**"])
    if not provider_enabled["OPENDTU_FEATURE_POWERMETER_PROVIDER_SMAHM2"]:
        _add_excludes(excludes, ["powermeter/smahm/**"])
    if not provider_enabled["OPENDTU_FEATURE_POWERMETER_PROVIDER_HTTP_SML"]:
        _add_excludes(excludes, ["powermeter/sml/http/**"])
    if not provider_enabled["OPENDTU_FEATURE_POWERMETER_PROVIDER_MODBUS_UDP_VICTRON"]:
        _add_excludes(excludes, ["powermeter/modbus/udp/victron/**"])
    if (
        not provider_enabled["OPENDTU_FEATURE_POWERMETER_PROVIDER_SERIAL_SML"]
        and not provider_enabled["OPENDTU_FEATURE_POWERMETER_PROVIDER_HTTP_SML"]
    ):
        _add_excludes(excludes, ["powermeter/sml/Provider.cpp"])

if not component_enabled["solarcharger"]:
    _add_excludes(excludes, ["solarcharger/**", "WebApi_solarcharger.cpp", "WebApi_ws_solarcharger_live.cpp"])
else:
    if not provider_enabled["OPENDTU_FEATURE_SOLARCHARGER_PROVIDER_VEDIRECT"]:
        _add_excludes(excludes, ["solarcharger/victron/**"])
    if not provider_enabled["OPENDTU_FEATURE_SOLARCHARGER_PROVIDER_MQTT"]:
        _add_excludes(excludes, ["solarcharger/mqtt/**"])

if not component_enabled["gridcharger"]:
    _add_excludes(excludes, ["gridcharger/**", "WebApi_gridcharger.cpp", "WebApi_ws_gridcharger.cpp"])
else:
    if not provider_enabled["OPENDTU_FEATURE_GRIDCHARGER_PROVIDER_HUAWEI"]:
        _add_excludes(excludes, ["gridcharger/huawei/**"])
    if not provider_enabled["OPENDTU_FEATURE_GRIDCHARGER_PROVIDER_TRUCKI"]:
        _add_excludes(excludes, ["gridcharger/trucki/**"])

if not component_enabled["powerlimiter"]:
    _add_excludes(
        excludes,
        [
            "PowerLimiter.cpp",
            "PowerLimiterBatteryInverter.cpp",
            "PowerLimiterInverter.cpp",
            "PowerLimiterOverscalingInverter.cpp",
            "PowerLimiterSmartBufferInverter.cpp",
            "PowerLimiterSolarInverter.cpp",
            "MqttHandlePowerLimiter.cpp",
            "MqttHandlePowerLimiterHass.cpp",
            "WebApi_powerlimiter.cpp",
        ],
    )
elif not component_enabled["mqtt_hass"]:
    _add_excludes(excludes, ["MqttHandlePowerLimiterHass.cpp"])

if not component_enabled["display"]:
    _add_excludes(excludes, ["Display_Graphic.cpp", "Display_Graphic_Diagram.cpp"])

if not component_enabled["mqtt_hass"]:
    _add_excludes(excludes, ["MqttHandleHass.cpp"])

if not component_enabled["webapi_device"]:
    _add_excludes(excludes, ["WebApi_device.cpp"])
if not component_enabled["webapi_mqtt"]:
    _add_excludes(excludes, ["WebApi_mqtt.cpp"])
if not component_enabled["webapi_network"]:
    _add_excludes(excludes, ["WebApi_network.cpp"])
if not component_enabled["webapi_ntp"]:
    _add_excludes(excludes, ["WebApi_ntp.cpp"])
if not component_enabled["webapi_prometheus"]:
    _add_excludes(excludes, ["WebApi_prometheus.cpp"])

# Optional local library pruning: VeDirect shunt/mppt parser library.
uses_vedirect = (
    provider_enabled["OPENDTU_FEATURE_BATTERY_PROVIDER_VICTRON_SMARTSHUNT"]
    or provider_enabled["OPENDTU_FEATURE_SOLARCHARGER_PROVIDER_VEDIRECT"]
)
if not uses_vedirect:
    env.AppendUnique(LIB_IGNORE=["VeDirectFrameHandler"])
    print("INFO: feature_src_filter: ignoring local library 'VeDirectFrameHandler' for env '{}'".format(env_name))

if excludes:
    # Ensure a baseline include exists, otherwise a SRC_FILTER list with only
    # excludes can lead to no user sources being compiled (missing setup/loop).
    src_filter = env.get("SRC_FILTER")
    if not src_filter:
        env.Replace(SRC_FILTER=["+<*>"] + excludes)
    else:
        if isinstance(src_filter, str):
            src_filter = [src_filter]
        has_include = any(
            isinstance(item, str) and item.strip().startswith("+<")
            for item in src_filter
        )
        if not has_include:
            env.Prepend(SRC_FILTER=["+<*>"])
        env.Append(SRC_FILTER=excludes)

    print("INFO: feature_src_filter: applying {} source excludes for env '{}'".format(len(excludes), env_name))
else:
    print("INFO: feature_src_filter: no source excludes for env '{}'".format(env_name))
