#include "motor.h"
#include "config.h"

Motor::Motor(int pinA, int pinB, uint8_t channelA, uint8_t channelB)
    : pinA(pinA), pinB(pinB), chanA(channelA), chanB(channelB) {}

void Motor::begin(uint32_t freq, uint8_t resolution) {
    ledcSetup(chanA, freq, resolution);
    ledcSetup(chanB, freq, resolution);
    ledcAttachPin(pinA, chanA);
    ledcAttachPin(pinB, chanB);
    stop();
}

uint8_t Motor::maxDuty() {
    return (1 << MOTOR_PWM_RESOLUTION) - 1;
}

void Motor::setSpeed(int16_t speed) {
    // clamp
    if (speed > 255) speed = 255;
    if (speed < -255) speed = -255;

    uint8_t duty = map(abs(speed), 0, 255, 0, maxDuty());

    if (speed > 0) {
        ledcWrite(chanA, duty);
        ledcWrite(chanB, 0);
    } else if (speed < 0) {
        ledcWrite(chanA, 0);
        ledcWrite(chanB, duty);
    } else {
        stop();
    }
}

void Motor::stop() {
    ledcWrite(chanA, 0);
    ledcWrite(chanB, 0);
}
