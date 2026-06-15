# Final ESP32 Rover Setup & Wiring

This is the **final configuration** for your rover with all sensors connected to the ESP32 DOIT DevKit V1.

## Pin Assignment Summary

### Motor Control (L298N or similar)
| Function | GPIO | Notes |
|----------|------|-------|
| Motor L IN1 | 26 | Motor left forward |
| Motor L IN2 | 27 | Motor left reverse |
| Motor R IN1 | 14 | Motor right forward |
| Motor R IN2 | 12 | Motor right reverse |

### Communication
| Module | GPIO | Function | Baud |
|--------|------|----------|------|
| GPS | Serial1 RX=13, TX=25 | NEO-6M UART | 9600 |
| LoRa SPI | 23(MOSI), 19(MISO), 18(SCK) | Data lines | - |
| LoRa Control | 5(SS/CS), 22(RST), 21(DIO0) | Chip select, reset, interrupt | - |

### Sensors
| Sensor | GPIO | Type | Notes |
|--------|------|------|-------|
| Ultrasonic TRIG | 33 | Output | HC-SR04 trigger |
| Ultrasonic ECHO | 32 | Input | HC-SR04 echo |
| DHT11 | 15 | Data | Temperature/humidity |
| NE555 Metal Detector | 35 | ADC Input | Analog pulse detector output |

### Power & Ground
- **ESP32 3.3V**: LoRa module, DHT11, GPIO pull-ups
- **5V regulated supply**: NE555 board, Ultrasonic modules, GPS (check module specs)
- **Common GND**: All modules share the same ground with battery

## NE555 Metal Detector (ADC-Based)

Your setup uses a simpler **ADC-based approach** (not frequency counting):

### Wiring
1. **NE555 Board Power**
   - VCC → 5V regulated supply
   - GND → ESP32 GND (common with battery)

2. **NE555 Output to ESP32**
   - OUT (pulse signal) → GPIO 35 (ADC pin)
   - **No level shifting needed**: the ADC input is more forgiving than GPIO interrupt pins

3. **Search Coil**
   - Connect coil to NE555 timing network as per your oscillator circuit
   - Keep coil away from metal during startup calibration

### Detection Method
- Firmware reads ADC every 50 ms
- Baseline calibrated at startup (2 seconds, keep metal away)
- Metal detected when ADC value **drops ≥ 500** from baseline
- Confidence calculated from drop magnitude
- Rolling average of 8 samples reduces noise

### Testing Procedure
1. Power up rover
2. Watch serial monitor at 115200 baud
3. Confirm message: `"Metal detector calibrated. Baseline ADC: ####"`
4. Wait 2 seconds for calibration to complete
5. Move metal object near the coil
6. ADC value should drop 500–1000 points
7. Serial should show: `"*** METAL DETECTED ***"` with confidence %
8. Rover motors stop and alert is sent over LoRa

## GPS (NEO-6M)

Changed from Serial2 (GPIO 16/17) to **Serial1 (GPIO 13/25)** to free up pins:

### Wiring
- GPS TX → ESP32 GPIO 13 (Serial1 RX)
- GPS RX → ESP32 GPIO 25 (Serial1 TX)
- GND → Common ground
- VCC → 3.3V or 5V (check your module)
- Baud rate: 9600

## DHT11 Sensor

Moved from GPIO 4 → **GPIO 15**:
- Data pin → GPIO 15
- GND → Common ground
- VCC → 3.3V

## Build & Flash

```bash
# Build for ESP32
platformio run

# Upload (adjust COM port for your system)
platformio run -t upload --upload-port COM3

# Monitor serial output
platformio device monitor --baud 115200 --port COM3
```

## Manual Control via LoRa

Send these JSON commands from a second LoRa node:

```json
{"cmd":"drive","left":200,"right":200,"duration_ms":400}
{"cmd":"drive","left":200,"right":-200,"duration_ms":400}
{"cmd":"stop"}
{"cmd":"estop"}
{"cmd":"clear_estop"}
```

## Dashboard (Optional)

Connect USB serial and run the Streamlit dashboard:

```bash
python -m venv .venv
.venv\Scripts\activate
pip install -r dashboard/requirements.txt
streamlit run dashboard/app.py
```

Then:
1. Select serial port (COM3, etc.) in dashboard
2. Click map to set mission waypoint
3. Send mission to rover
4. Watch live telemetry, GPS track, and metal detection alerts

## Troubleshooting

**GPS fix takes > 1 minute**
- Move rover outdoors or near a window
- Ensure antenna is not covered
- Wait for ≥ 4 satellites (shown in telemetry)

**No metal detection**
- Confirm ADC value changes when metal approaches coil
- Baseline ADC should be 2000-3500 (open space)
- Drop should be 500+ when metal is close
- Increase detection threshold in config.h if too noisy

**Motors not responding to serial commands**
- Confirm baud rate is 115200
- Check motor driver power supply voltage
- Verify motor enable pins are active (PWM or always high)

**LoRa not receiving commands**
- Check frequency: 433 MHz vs 868/915 MHz
- Ensure both modules use same frequency
- Verify antenna connections
- Test with Phase4 test code first

## Hardware Checklist

- [ ] ESP32 DOIT DevKit V1
- [ ] L298N or similar motor driver (2 motors)
- [ ] NEO-6M GPS module
- [ ] NE555-based metal detector board with search coil
- [ ] DHT11 temperature/humidity sensor
- [ ] SX1278 LoRa module (433 MHz or 868/915 MHz)
- [ ] HC-SR04 ultrasonic sensor
- [ ] 12V battery with buck converters for 5V and 3.3V
- [ ] Resistor dividers or level shifters where needed
- [ ] USB cable for ESP32 programming

## Next Steps

1. Flash the firmware
2. Test each sensor via serial monitor
3. Test GPS fix outdoors
4. Test metal detector with metal objects
5. Test LoRa commands if available
6. Run complete mission test on bench
7. Field test with motors running
