# Venus OS SunSpec Power Control

The Modbus TCP server exposes every configured inverter as a SunSpec PV
inverter. In addition to the read-only models 1, 101/103 and 120, model 123
(Immediate Controls) lets Victron Venus OS limit active inverter power.

## Venus OS behavior

Victron's `dbus-fronius` driver detects model 123 and writes the following
standard fields with Modbus function code 16 (`0x10`):

- `WMaxLimPct`: requested output as a percentage of model 120 `WRtg`
- `WMaxLimPct_RvrtTms`: watchdog timeout in seconds
- `WMaxLim_Ena`: enable or release the limit

OpenDTU-OnBattery converts the percentage to a non-persistent Hoymiles active
power command. Disabling the control or reaching the watchdog timeout restores
the inverter to 100%. The timeout remains a safety fallback if Venus OS stops
refreshing its command or loses the TCP connection.

Power limiting must also be enabled for the PV inverter in Venus OS. Venus OS
derives the percentage from its requested watt value and the inverter rating
reported by SunSpec model 120.

## Exclusive ownership

A single inverter cannot be controlled by both SunSpec/Modbus and the Dynamic
Power Limiter (DPL):

- The Modbus settings reject an inverter currently governed by an enabled DPL.
- The DPL settings reject an inverter currently mapped to an enabled Modbus
  server.
- Both settings pages disable conflicting selections and explain the conflict.
- On startup, the Modbus runtime ignores any conflicting legacy configuration,
  leaving control with the DPL.

Disable the inverter in its current control mode before assigning it to the
other mode.

## Safety requirements

- Inverter command transmission must be enabled in OpenDTU-OnBattery.
- The inverter nameplate power must be known before commands are accepted.
- Writes outside the model 123 active-power window are rejected.
- Limits outside 0–100% and invalid enable values are rejected.
