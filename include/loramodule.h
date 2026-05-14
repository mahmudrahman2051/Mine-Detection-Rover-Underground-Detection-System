#ifndef LORAMODULE_H
#define LORAMODULE_H

#include <Arduino.h>
#include <SPI.h>

class LoRaModule {
public:
    LoRaModule(int ssPin, int rstPin, int dio0Pin);
    bool begin(long frequency);
    bool sendPacket(const uint8_t* data, size_t len);
    // send and wait for an ACK reply; returns true if ACK received
    bool sendPacketWithAck(const uint8_t* data, size_t len, int retries = 3, unsigned long timeoutMs = 1500);
    // non-blocking receive; returns length or 0
    int receivePacket(uint8_t* buf, size_t bufsize);

private:
    int ss;
    int rst;
    int dio0;
    uint16_t crc16(const uint8_t* data, size_t len);
};

#endif
