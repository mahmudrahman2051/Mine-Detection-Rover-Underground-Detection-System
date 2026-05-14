#include "loramodule.h"
#include "config.h"
#include <LoRa.h>

LoRaModule::LoRaModule(int ssPin, int rstPin, int dio0Pin)
    : ss(ssPin), rst(rstPin), dio0(dio0Pin) {}

bool LoRaModule::begin(long frequency) {
    SPI.begin();
    LoRa.setPins(ss, rst, dio0);
    if (!LoRa.begin(frequency)) {
        return false;
    }
    LoRa.enableCrc();
    return true;
}

uint16_t LoRaModule::crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; ++j) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
            else crc <<= 1;
        }
    }
    return crc;
}

bool LoRaModule::sendPacket(const uint8_t* data, size_t len) {
    if (len == 0 || len > 240) return false;
    uint16_t crc = crc16(data, len);
    LoRa.beginPacket();
    LoRa.write(data, len);
    LoRa.write((uint8_t)(crc >> 8));
    LoRa.write((uint8_t)(crc & 0xFF));
    LoRa.endPacket();
    return true;
}

int LoRaModule::receivePacket(uint8_t* buf, size_t bufsize) {
    int packetSize = LoRa.parsePacket();
    if (packetSize <= 0) return 0;
    if (packetSize < 3) return 0; // at least 2 bytes CRC
    int payloadSize = packetSize - 2;
    if (payloadSize > (int)bufsize) return 0;
    for (int i = 0; i < payloadSize; ++i) {
        buf[i] = LoRa.read();
    }
    uint8_t crcHi = LoRa.read();
    uint8_t crcLo = LoRa.read();
    uint16_t recvCrc = (crcHi << 8) | crcLo;
    uint16_t calc = crc16(buf, payloadSize);
    if (calc != recvCrc) return 0;
    return payloadSize;
}
