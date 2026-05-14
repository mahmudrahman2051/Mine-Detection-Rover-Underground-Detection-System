#ifndef CONFIG_H
#define CONFIG_H

#define ULTRASONIC_TRIG_PIN 33
#define ULTRASONIC_ECHO_PIN 32

#define DHT11_PIN 4
#define DHT_TYPE DHT11

#define BAUD_RATE 115200

// Actuator pins
#define HEATER_LED_PIN 12
#define COOLER_LED_PIN 13

// Temperature thresholds (°C)
#define TEMP_HEATER_THRESHOLD 18
#define TEMP_COOLER_THRESHOLD 28

#define MAX_DISTANCE 400
#define MIN_DISTANCE 2

#endif

// Motor driver (BTS7960) pins - Phase 1
#define MOTOR_L_PWM_A 25
#define MOTOR_L_PWM_B 26
#define MOTOR_R_PWM_A 27
#define MOTOR_R_PWM_B 14

// Emergency stop (active LOW)
#define E_STOP_PIN 12

// PWM settings
#define MOTOR_PWM_FREQ 20000
#define MOTOR_PWM_RESOLUTION 8

// Battery monitoring
#define BATTERY_ADC_PIN 35
// voltage divider: R1 (top) = 100k, R2 (bottom) = 33k -> ratio = (R1+R2)/R2
#define BATTERY_VOLT_DIVIDER_RATIO  ( (100.0 + 33.0) / 33.0 )
#define BATTERY_FULL_VOLTAGE 12.6
#define BATTERY_LOW_VOLTAGE 10.5

// LoRa (SX1278) pins (VSPI)
#define LORA_SS_PIN 18
#define LORA_RST_PIN 22
#define LORA_DIO0_PIN 21

// GPS (NEO-6M) Serial2 pins
#define GPS_RX_PIN 16 // ESP32 RX2 (connect to TX of GPS)
#define GPS_TX_PIN 17 // ESP32 TX2 (connect to RX of GPS)

// Metal detector module
#define METAL_DETECTOR_PIN 34 // digital input (also ADC-capable if analog desired)

// MQ-2 smoke/gas sensor analog pin
#define MQ2_PIN 36

// SD card
#define SD_CS_PIN 13



