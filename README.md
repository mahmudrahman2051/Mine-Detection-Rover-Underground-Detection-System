# Mine Detection Rover - Underground Detection System

## Overview
This project is an ESP32-based rover platform for underground mine detection, obstacle handling, telemetry, and future laptop control.

## Core Elements
- ESP32 DevKit V1
- BTS7960 motor drivers
- 2x DC geared motors
- HC-SR04 ultrasonic sensor
- DHT11 temperature and humidity sensor
- LoRa SX1278 telemetry module
- NEO-6M GPS module
- MQ-2 smoke sensor
- Mini metal detector module
- 12V battery pack and buck converters
- Emergency stop switch and fuse

## Wiring Snapshot
- Ultrasonic Trigger: GPIO 5
- Ultrasonic Echo: GPIO 18
- DHT11 Data: GPIO 4
- E-STOP: configured in `config.h`
- Motor PWM and direction pins: configured in `config.h`
- LoRa, GPS, MQ-2, metal detector, and battery monitor pins: configured in `config.h`

## Build And Upload
1. Install the PlatformIO extension in VS Code.
2. Connect the ESP32 through USB.
3. Run `pio run` to build.
4. Run `pio run -t upload` to flash the board.
5. Open Serial Monitor at 115200 baud for telemetry.

## Laptop Dashboard
The control station lives in `dashboard/app.py` and uses Streamlit.

Run it with:

```bash
f:/Electronics_Lab/.venv/Scripts/python.exe -m streamlit run dashboard/app.py
```

The dashboard supports live telemetry, manual drive control, map-based target selection, mission upload, and emergency stop commands.

The selected mission is sent as a JSON packet like:

```json
{"mission":"start","lat":23.8103,"lon":90.4125,"radius":20}
```

The rover firmware still needs the mission receiver / navigation loop to act on that packet if you want fully autonomous travel to the selected location.

## Process
1. Power the rover through the fused 12V line and buck converters.
2. Verify sensor readings on Serial before enabling motor motion.
3. Test manual control and emergency stop.
4. Validate obstacle avoidance and heading correction.
5. Move to waypoint, spiral search, and event logging phases.

## Data Collected
- Distance measurements
- Temperature and humidity
- Battery status
- GPS position and heading
- LoRa telemetry packets
- Metal-detect and smoke events
