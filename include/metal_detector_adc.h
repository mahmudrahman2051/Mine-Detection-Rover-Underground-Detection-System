#ifndef METAL_DETECTOR_ADC_H
#define METAL_DETECTOR_ADC_H

#include <Arduino.h>
#include "config.h"

class MetalDetectorADC {
public:
    MetalDetectorADC();
    
    // Initialize ADC pin
    void begin();
    
    // Calibration: measure baseline ADC value when no metal present (blocking, ~2 sec)
    bool calibrate();
    
    // Non-blocking ADC update; call from main loop
    void update();
    
    // Returns true if metal detected based on ADC drop
    bool isMetalDetected();
    
    // Detection confidence (0-100%)
    int getConfidence();
    
    // Current ADC value
    int getCurrentValue();
    
    // Baseline ADC value (no metal)
    int getBaselineValue();
    
    // ADC drop from baseline
    int getValueDrop();
    
private:
    bool initialized;
    bool calibrated;
    
    // ADC state
    int baselineValue;
    int currentValue;
    int valueDrop;
    unsigned long lastSampleMs;
    
    // Rolling sample history for noise filtering
    int adcHistory[NE555_SAMPLE_HISTORY_SIZE];
    int historyIndex;
    int sampleCount;
    
    // Detection state with hysteresis
    bool detectionActive;
    unsigned long detectionStartMs;
    unsigned long detectionHysteresisMs;
    
    // Calculate moving average of ADC samples
    int getAverageValue();
};

#endif
