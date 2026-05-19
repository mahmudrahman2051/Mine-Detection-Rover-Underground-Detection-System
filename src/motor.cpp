#include "motor.h"
#include "config.h"

Motor::Motor(int pinA, int pinB, uint8_t channelA, uint8_t channelB)
    : pinA(pinA), pinB(pinB), chanA(channelA), chanB(channelB) {}

Motor::Motor(int in1Pin_, int in2Pin_, int enPin_, uint8_t enChannel_, bool useL298)
    : pinA(-1), pinB(-1), chanA(0), chanB(0), isL298(useL298), in1Pin(in1Pin_), in2Pin(in2Pin_), enPin(enPin_), enChannel(enChannel_) {}

void Motor::begin(uint32_t freq, uint8_t resolution) {
    if (isL298) {
        pinMode(in1Pin, OUTPUT);
        pinMode(in2Pin, OUTPUT);
        ledcSetup(enChannel, freq, resolution);
        ledcAttachPin(enPin, enChannel);
        digitalWrite(in1Pin, LOW);
        digitalWrite(in2Pin, LOW);
        ledcWrite(enChannel, 0);
        return;
    }
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

    if (isL298) {
        if (speed > 0) {
            digitalWrite(in1Pin, HIGH);
            digitalWrite(in2Pin, LOW);
            ledcWrite(enChannel, duty);
        } else if (speed < 0) {
            digitalWrite(in1Pin, LOW);
            digitalWrite(in2Pin, HIGH);
            ledcWrite(enChannel, duty);
        } else {
            // coast
            digitalWrite(in1Pin, LOW);
            digitalWrite(in2Pin, LOW);
            ledcWrite(enChannel, 0);
        }
        return;
    }

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
    if (isL298) {
        ledcWrite(enChannel, 0);
        digitalWrite(in1Pin, LOW);
        digitalWrite(in2Pin, LOW);
        return;
    }
    ledcWrite(chanA, 0);
    ledcWrite(chanB, 0);
}
