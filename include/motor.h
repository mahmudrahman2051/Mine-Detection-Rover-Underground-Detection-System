#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>

class Motor {
public:
    // pins: two PWM inputs for BTS7960 (A and B). channelA/channelB are LEDC channels.
    Motor(int pinA, int pinB, uint8_t channelA, uint8_t channelB);
    void begin(uint32_t freq, uint8_t resolution);
    // speed -255..255 where sign is direction
    void setSpeed(int16_t speed);
    void stop();

private:
    int pinA;
    int pinB;
    uint8_t chanA;
    uint8_t chanB;
    uint8_t maxDuty();
};

#endif
