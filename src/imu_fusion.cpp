#include "imu_fusion.h"
#include <Preferences.h>
#include <Arduino.h>

IMUFusion::IMUFusion() : sensor(), filter(0.08f), lastMicros(0), lastHeading(0.0f) {}

bool IMUFusion::begin() {
    if (!sensor.begin()) return false;
    filter.begin(100.0f);
    lastMicros = micros();
    loadMagCalibration();
    return true;
}

bool IMUFusion::update() {
    unsigned long now = micros();
    float dt = (now - lastMicros) / 1000000.0f;
    if (dt <= 0.0f) dt = 0.001f;
    lastMicros = now;

    float ax, ay, az, gx, gy, gz, mx, my, mz;
    if (!sensor.readAccelGyro(ax, ay, az, gx, gy, gz)) return false;
    sensor.readMag(mx, my, mz); // optional

    // apply magnetometer calibration offsets
    mx -= magOffsetX; my -= magOffsetY; mz -= magOffsetZ;

    // Madgwick expects gyro in deg/s, accel in g, mag in arbitrary units
    filter.update(gx, gy, gz, ax, ay, az, mx, my, mz, dt);
    lastHeading = filter.getYaw();
    if (lastHeading < 0) lastHeading += 360.0f;
    return true;
}

float IMUFusion::getHeading() {
    return lastHeading;
}

bool IMUFusion::readRaw(float& ax, float& ay, float& az, float& gx, float& gy, float& gz, float& mx, float& my, float& mz) {
    bool ok = sensor.readAccelGyro(ax, ay, az, gx, gy, gz);
    sensor.readMag(mx, my, mz);
    return ok;
}

bool IMUFusion::calibrateMagnetometer(unsigned int durationSeconds) {
    unsigned long end = millis() + durationSeconds * 1000UL;
    float minx =  1e6f, miny = 1e6f, minz = 1e6f;
    float maxx = -1e6f, maxy = -1e6f, maxz = -1e6f;
    while (millis() < end) {
        float ax, ay, az, gx, gy, gz, mx, my, mz;
        if (sensor.readAccelGyro(ax, ay, az, gx, gy, gz)) {
            if (sensor.readMag(mx, my, mz)) {
                if (mx < minx) minx = mx;
                if (my < miny) miny = my;
                if (mz < minz) minz = mz;
                if (mx > maxx) maxx = mx;
                if (my > maxy) maxy = my;
                if (mz > maxz) maxz = mz;
            }
        }
        delay(50);
    }
    if (minx > maxx) return false; // no data
    magOffsetX = (maxx + minx) * 0.5f;
    magOffsetY = (maxy + miny) * 0.5f;
    magOffsetZ = (maxz + minz) * 0.5f;
    saveMagCalibration();
    return true;
}

void IMUFusion::loadMagCalibration() {
    Preferences prefs;
    prefs.begin("magcal", true);
    magOffsetX = prefs.getFloat("ox", 0.0f);
    magOffsetY = prefs.getFloat("oy", 0.0f);
    magOffsetZ = prefs.getFloat("oz", 0.0f);
    prefs.end();
}

void IMUFusion::saveMagCalibration() {
    Preferences prefs;
    prefs.begin("magcal", false);
    prefs.putFloat("ox", magOffsetX);
    prefs.putFloat("oy", magOffsetY);
    prefs.putFloat("oz", magOffsetZ);
    prefs.end();
}
