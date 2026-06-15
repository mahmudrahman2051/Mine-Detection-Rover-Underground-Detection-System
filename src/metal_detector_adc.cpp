#include "metal_detector_adc.h"
#include "config.h"

MetalDetectorADC::MetalDetectorADC()
    : initialized(false), calibrated(false), baselineValue(0), currentValue(0),
      valueDrop(0), lastSampleMs(0), historyIndex(0), sampleCount(0),
            detectionActive(false), detectionStartMs(0), detectionHysteresisMs(NE555_DETECTION_HYSTERESIS_MS) {
    memset(adcHistory, 0, sizeof(adcHistory));
}

void MetalDetectorADC::begin() {
    if (initialized) return;
    pinMode(NE555_OUTPUT_PIN, INPUT);
    analogReadResolution(12); // 12-bit ADC (0-4095)
    lastSampleMs = millis();
    initialized = true;
}

bool MetalDetectorADC::calibrate() {
    if (!initialized) return false;
    
    // Collect baseline ADC value over NE555_CALIBRATION_DURATION_MS
    unsigned long calStart = millis();
    unsigned long calEnd = calStart + NE555_CALIBRATION_DURATION_MS;
    
    int valueSum = 0;
    int sampleCount = 0;
    
    while (millis() < calEnd) {
        int val = analogRead(NE555_OUTPUT_PIN);
        valueSum += val;
        sampleCount++;
        delay(10);
    }
    
    if (sampleCount > 0) {
        baselineValue = valueSum / sampleCount;
        currentValue = baselineValue;
        calibrated = true;
        return true;
    }
    
    return false;
}

void MetalDetectorADC::update() {
    if (!initialized || !calibrated) return;
    
    unsigned long now = millis();
    
    // Sample ADC every NE555_SAMPLE_WINDOW_MS
    if (now - lastSampleMs >= NE555_SAMPLE_WINDOW_MS) {
        // Read raw ADC value
        currentValue = analogRead(NE555_OUTPUT_PIN);
        lastSampleMs = now;
        
        // Add to rolling history
        adcHistory[historyIndex] = currentValue;
        historyIndex = (historyIndex + 1) % NE555_SAMPLE_HISTORY_SIZE;
        if (sampleCount < NE555_SAMPLE_HISTORY_SIZE) {
            sampleCount++;
        }
        
        // Compute drop from the current sample for immediate detection
        int avgValue = getAverageValue();
        valueDrop = baselineValue - currentValue;  // positive when value drops (metal near)
        int filteredDrop = baselineValue - avgValue;
        
        // Metal detection logic with hysteresis.
        // Trigger from the current ADC sample so detection starts as soon as the drop happens.
        bool shouldDetect = (valueDrop >= NE555_DETECTION_THRESHOLD) && (sampleCount >= 2);
        
        if (shouldDetect && !detectionActive) {
            // Rising edge: metal detected
            detectionActive = true;
            detectionStartMs = now;
        } else if (!shouldDetect && detectionActive && (now - detectionStartMs) > detectionHysteresisMs) {
            // Falling edge: metal no longer detected (after hysteresis)
            detectionActive = false;
        }

        // Keep the filtered drop available for confidence-style reporting if needed.
        if (!detectionActive && filteredDrop > valueDrop) {
            valueDrop = filteredDrop;
        }
    }
}

bool MetalDetectorADC::isMetalDetected() {
    return detectionActive;
}

int MetalDetectorADC::getConfidence() {
    if (!calibrated || sampleCount == 0) return 0;
    
    // Confidence increases as ADC drop increases beyond threshold.
    // Use the current sample drop for speed; the small history smooths the output.
    int absDrop = abs(valueDrop);
    
    // Max confidence at 2x threshold drop.
    int conf = (int)((float)(absDrop - NE555_DETECTION_THRESHOLD) / (float)NE555_DETECTION_THRESHOLD * 100.0f);
    if (conf < 0) conf = 0;
    if (conf > 100) conf = 100;
    
    // Reduce confidence during early samples (less reliable), but keep it responsive.
    if (sampleCount < 3) {
        conf = (conf * sampleCount) / 3;
    }
    
    return conf;
}

int MetalDetectorADC::getCurrentValue() {
    return currentValue;
}

int MetalDetectorADC::getBaselineValue() {
    return baselineValue;
}

int MetalDetectorADC::getValueDrop() {
    return valueDrop;
}

int MetalDetectorADC::getAverageValue() {
    if (sampleCount == 0) return 0;
    
    int sum = 0;
    for (int i = 0; i < sampleCount; ++i) {
        sum += adcHistory[i];
    }
    return sum / sampleCount;
}
