#include "sd_logger.h"
#include "config.h"
#include <SD.h>
#include <SPI.h>

SdLogger::SdLogger() : initialized(false) {}

bool SdLogger::begin() {
    if (initialized) return true;
    if (!SD.begin(SD_CS_PIN)) {
        initialized = false;
        return false;
    }
    initialized = true;
    return true;
}

void SdLogger::logLine(const String& line) {
    if (!initialized) return;
    File f = SD.open("telemetry.log", FILE_APPEND);
    if (!f) return;
    f.println(line);
    f.close();
}
