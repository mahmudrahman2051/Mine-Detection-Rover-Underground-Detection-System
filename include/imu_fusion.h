#ifndef IMU_FUSION_H
#define IMU_FUSION_H

#include <Arduino.h>
#include "imu_mpu9250.h"
#include "MadgwickAHRS.h"

class IMUFusion {
public:
    IMUFusion();
    bool begin();
    // call periodically to update filter; returns true when valid
    bool update();
    // heading in degrees 0..360
    float getHeading();
    // expose raw sensor reads for diagnostics
    bool readRaw(float& ax, float& ay, float& az, float& gx, float& gy, float& gz, float& mx, float& my, float& mz);
    // magnetometer calibration: collects samples for duration (seconds) and stores offsets
    bool calibrateMagnetometer(unsigned int durationSeconds = 10);
    void loadMagCalibration();
    void saveMagCalibration();
private:
    MPU9250 sensor;
    Madgwick filter;
    unsigned long lastMicros;
    float lastHeading;
    float magOffsetX = 0.0f;
    float magOffsetY = 0.0f;
    float magOffsetZ = 0.0f;
};

#endif
