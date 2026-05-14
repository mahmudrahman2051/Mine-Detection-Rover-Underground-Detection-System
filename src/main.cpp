#include <Arduino.h>
#include "config.h"
#include "motor.h"
#include "drive.h"
#include "gps.h"
#include "navigator.h"
#include "imu_fusion.h"
#include "ultrasonic.h"

// PWM channels for ESP32 LEDC
#define CHAN_ML_A 0
#define CHAN_ML_B 1
#define CHAN_MR_A 2
#define CHAN_MR_B 3

// Obstacle handling thresholds (bench-safe defaults)
static const float OBSTACLE_THRESHOLD_CM = 35.0f;
static const unsigned long TELEMETRY_INTERVAL_MS = 500;
static const unsigned long OBSTACLE_CHECK_INTERVAL_MS = 120;

Motor motorL(MOTOR_L_PWM_A, MOTOR_L_PWM_B, CHAN_ML_A, CHAN_ML_B);
Motor motorR(MOTOR_R_PWM_A, MOTOR_R_PWM_B, CHAN_MR_A, CHAN_MR_B);
Drive drive(motorL, motorR);
GPSModule gps(Serial2);
WaypointNavigator navigator(drive);
IMUFusion imu;
UltrasonicSensor frontUltrasonic(ULTRASONIC_TRIG_PIN, ULTRASONIC_ECHO_PIN);

enum RoverMode {
    MODE_IDLE,
    MODE_AUTO,
    MODE_MANUAL,
    MODE_ESTOP
};

enum AvoidancePhase {
    AVOID_NONE,
    AVOID_BACKUP,
    AVOID_TURN
};

RoverMode currentMode = MODE_IDLE;
AvoidancePhase avoidPhase = AVOID_NONE;

bool imuReady = false;
bool missionActive = false;
double targetLat = 0.0;
double targetLon = 0.0;
float targetRadiusM = 1.0f;

unsigned long lastTelemetryMs = 0;
unsigned long lastObstacleCheckMs = 0;
unsigned long avoidPhaseStartMs = 0;
unsigned long manualUntilMs = 0;

float lastDistanceCm = -1.0f;
int avoidTurnDir = 1;
float lastHeadingDeg = 0.0f;
GPSData lastGps = {0.0, 0.0, 0.0, 0.0, 99.9, 0, false};
String serialLine;

static const char* modeToString(RoverMode mode) {
    switch (mode) {
        case MODE_AUTO: return "AUTO";
        case MODE_MANUAL: return "MANUAL";
        case MODE_ESTOP: return "ESTOP";
        default: return "IDLE";
    }
}

bool extractFloatField(const String& json, const char* key, float& outVal) {
    String token = String("\"") + key + "\":";
    int start = json.indexOf(token);
    if (start < 0) return false;
    start += token.length();
    while (start < json.length() && (json[start] == ' ' || json[start] == '"')) start++;
    int end = start;
    while (end < json.length()) {
        char c = json[end];
        if ((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E') {
            end++;
        } else {
            break;
        }
    }
    if (end <= start) return false;
    outVal = json.substring(start, end).toFloat();
    return true;
}

bool extractIntField(const String& json, const char* key, int& outVal) {
    float v;
    if (!extractFloatField(json, key, v)) return false;
    outVal = (int)v;
    return true;
}

void sendEvent(const char* level, const char* category, const String& message) {
    Serial.print("{\"type\":\"event\",\"level\":\"");
    Serial.print(level);
    Serial.print("\",\"category\":\"");
    Serial.print(category);
    Serial.print("\",\"message\":\"");
    // Basic escaping for quotes
    String safe = message;
    safe.replace("\"", "'");
    Serial.print(safe);
    Serial.print("\"");
    if (lastGps.valid) {
        Serial.print(",\"lat\":");
        Serial.print(lastGps.latitude, 6);
        Serial.print(",\"lon\":");
        Serial.print(lastGps.longitude, 6);
    }
    Serial.println("}");
}

void sendTelemetryNow() {
    Serial.print("{\"type\":\"telemetry\",\"mode\":\"");
    Serial.print(modeToString(currentMode));
    Serial.print("\",\"mission_active\":");
    Serial.print(missionActive ? "true" : "false");
    Serial.print(",\"distance_cm\":");
    Serial.print(lastDistanceCm, 1);
    Serial.print(",\"heading_deg\":");
    Serial.print(lastHeadingDeg, 1);
    if (lastGps.valid) {
        Serial.print(",\"gps_lat\":");
        Serial.print(lastGps.latitude, 6);
        Serial.print(",\"gps_lon\":");
        Serial.print(lastGps.longitude, 6);
        Serial.print(",\"speed_kph\":");
        Serial.print(lastGps.speedKph, 2);
        Serial.print(",\"satellites\":");
        Serial.print(lastGps.satellites);
    }
    if (missionActive) {
        Serial.print(",\"target_lat\":");
        Serial.print(targetLat, 6);
        Serial.print(",\"target_lon\":");
        Serial.print(targetLon, 6);
        Serial.print(",\"target_radius_m\":");
        Serial.print(targetRadiusM, 3);
    }
    Serial.println("}");
}

void startAvoidance(float obstacleDistanceCm) {
    avoidPhase = AVOID_BACKUP;
    avoidPhaseStartMs = millis();
    avoidTurnDir = ((millis() / 1000) % 2 == 0) ? 1 : -1;
    sendEvent("WARN", "SENSOR", String("Obstacle detected at ") + String(obstacleDistanceCm, 1) + " cm. Starting avoidance.");
}

void runAvoidance() {
    unsigned long elapsed = millis() - avoidPhaseStartMs;
    if (avoidPhase == AVOID_BACKUP) {
        if (elapsed < 700) {
            drive.setSpeed(-120, -120);
            return;
        }
        avoidPhase = AVOID_TURN;
        avoidPhaseStartMs = millis();
    }

    if (avoidPhase == AVOID_TURN) {
        unsigned long turnElapsed = millis() - avoidPhaseStartMs;
        if (turnElapsed < 1000) {
            if (avoidTurnDir > 0) drive.setSpeed(120, -120);
            else drive.setSpeed(-120, 120);
            return;
        }
        avoidPhase = AVOID_NONE;
        sendEvent("INFO", "NAV", "Avoidance complete, resuming navigation");
    }
}

void handleDriveCommand(const String& line) {
    int left = 0;
    int right = 0;
    int durationMs = 800;
    extractIntField(line, "left", left);
    extractIntField(line, "right", right);
    extractIntField(line, "duration_ms", durationMs);
    left = constrain(left, -255, 255);
    right = constrain(right, -255, 255);
    drive.setSpeed(left, right);
    currentMode = MODE_MANUAL;
    manualUntilMs = millis() + (unsigned long)max(durationMs, 200);
}

void handleMissionStart(const String& line) {
    float lat = 0.0f, lon = 0.0f;
    float radiusCm = 100.0f;
    float radiusM = -1.0f;

    bool hasLat = extractFloatField(line, "lat", lat);
    bool hasLon = extractFloatField(line, "lon", lon);
    bool hasRadiusCm = extractFloatField(line, "radius_cm", radiusCm);
    bool hasRadiusM = extractFloatField(line, "radius_m", radiusM);

    if (!hasRadiusCm) {
        float radiusRaw = 0.0f;
        if (extractFloatField(line, "radius", radiusRaw)) {
            // Dashboard sends radius in cm for bench testing.
            radiusCm = radiusRaw;
            hasRadiusCm = true;
        }
    }

    if (!hasLat || !hasLon) {
        sendEvent("ERROR", "MISSION", "Mission start rejected: lat/lon missing");
        return;
    }

    targetLat = lat;
    targetLon = lon;
    targetRadiusM = hasRadiusM ? max(radiusM, 0.1f) : max(radiusCm / 100.0f, 0.1f);

    navigator.setGoal(targetLat, targetLon, targetRadiusM);
    missionActive = true;
    currentMode = MODE_AUTO;
    avoidPhase = AVOID_NONE;

    sendEvent("INFO", "MISSION", String("Mission accepted: target=") + String(targetLat, 6) + "," + String(targetLon, 6));
}

void handleCommandLine(const String& line) {
    if (line.length() == 0) return;

    if (line.indexOf("\"mission\":\"start\"") >= 0) {
        handleMissionStart(line);
        return;
    }

    if (line.indexOf("\"cmd\":\"drive\"") >= 0) {
        if (currentMode != MODE_ESTOP) handleDriveCommand(line);
        return;
    }

    if (line.indexOf("\"cmd\":\"stop\"") >= 0) {
        drive.stop();
        currentMode = missionActive ? MODE_AUTO : MODE_IDLE;
        return;
    }

    if (line.indexOf("\"cmd\":\"estop\"") >= 0) {
        drive.stop();
        currentMode = MODE_ESTOP;
        missionActive = false;
        avoidPhase = AVOID_NONE;
        sendEvent("WARN", "SAFETY", "Emergency stop latched");
        return;
    }

    if (line.indexOf("\"cmd\":\"clear_estop\"") >= 0) {
        if (currentMode == MODE_ESTOP) {
            currentMode = MODE_IDLE;
            sendEvent("INFO", "SAFETY", "Emergency stop cleared");
        }
        return;
    }

    if (line.indexOf("\"cmd\":\"ping\"") >= 0) {
        Serial.println("{\"type\":\"ack\",\"message\":\"pong\"}");
        return;
    }

    if (line.indexOf("\"cmd\":\"status\"") >= 0) {
        sendTelemetryNow();
        return;
    }
}

void readSerialCommands() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\n') {
            String line = serialLine;
            serialLine = "";
            line.trim();
            if (line.length() > 0) handleCommandLine(line);
        } else if (c != '\r') {
            serialLine += c;
            if (serialLine.length() > 400) serialLine = "";
        }
    }
}

void setup() {
    Serial.begin(BAUD_RATE);
    delay(200);
    Serial.println("Rover mission controller booting...");

    drive.begin();
    gps.begin(9600);
    navigator.begin();
    frontUltrasonic.init();

    imuReady = imu.begin();
    if (imuReady) sendEvent("INFO", "IMU", "IMU fusion initialized");
    else sendEvent("WARN", "IMU", "IMU init failed, heading fallback enabled");

    sendEvent("INFO", "SYSTEM", "Mission controller ready");
}

void loop() {
    static unsigned long lastMicros = micros();
    unsigned long nowMicros = micros();
    float dt = (nowMicros - lastMicros) / 1000000.0f;
    if (dt <= 0.0f) dt = 0.02f;
    lastMicros = nowMicros;

    readSerialCommands();

    gps.feed();
    lastGps = gps.getData();

    if (imuReady) {
        imu.update();
        lastHeadingDeg = imu.getHeading();
    }

    if (millis() - lastObstacleCheckMs >= OBSTACLE_CHECK_INTERVAL_MS) {
        lastObstacleCheckMs = millis();
        lastDistanceCm = frontUltrasonic.getDistance();
    }

    if (currentMode == MODE_ESTOP) {
        drive.stop();
    } else if (currentMode == MODE_MANUAL) {
        if (millis() > manualUntilMs) {
            drive.stop();
            currentMode = missionActive ? MODE_AUTO : MODE_IDLE;
        }
    } else if (missionActive && currentMode == MODE_AUTO) {
        if (avoidPhase != AVOID_NONE) {
            runAvoidance();
        } else if (lastDistanceCm > 0 && lastDistanceCm < OBSTACLE_THRESHOLD_CM) {
            startAvoidance(lastDistanceCm);
            runAvoidance();
        } else if (lastGps.valid) {
            bool arrived = navigator.update(lastGps, lastHeadingDeg, dt);
            if (arrived) {
                missionActive = false;
                currentMode = MODE_IDLE;
                drive.stop();
                sendEvent("INFO", "MISSION", "Target reached");
            }
        } else {
            // Hold if GPS fix is unavailable while in auto mode
            drive.stop();
        }
    } else {
        drive.stop();
    }

    if (millis() - lastTelemetryMs >= TELEMETRY_INTERVAL_MS) {
        lastTelemetryMs = millis();
        sendTelemetryNow();
    }

    delay(10);
}
