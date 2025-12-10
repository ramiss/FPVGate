#include "laptimer.h"

#include "debug.h"

#ifdef ESP32S3
#include "rgbled.h"
extern RgbLed* g_rgbLed;
#endif

// Light Kalman filtering since hardware cap does most of the work
// Lower Q = less filtering (hardware cap already smooths noise)
// Lower R = faster response to real signal changes
const uint16_t rssi_filter_q = 500;   // Light filtering with hardware cap
const uint16_t rssi_filter_r = 50;    // Fast response

void LapTimer::init(Config *config, RX5808 *rx5808, Buzzer *buzzer, Led *l) {
    conf = config;
    rx = rx5808;
    buz = buzzer;
    led = l;

    filter.setMeasurementNoise(rssi_filter_q * 0.01f);
    filter.setProcessNoise(rssi_filter_r * 0.0001f);

    stop();
    memset(rssi, 0, sizeof(rssi));
    memset(rssi_window, 0, sizeof(rssi_window));
    rssi_window_index = 0;
}

void LapTimer::start() {
    DEBUG("\n=== RACE STARTED ===\n");
    DEBUG("Current Thresholds:\n");
    DEBUG("  Enter RSSI: %u\n", conf->getEnterRssi());
    DEBUG("  Exit RSSI: %u\n", conf->getExitRssi());
    DEBUG("  Min Lap Time: %u ms\n", conf->getMinLapMs());
    DEBUG("\nCurrent RSSI: %u\n", rssi[rssiCount]);
    DEBUG("\nIf laps aren't detected, your thresholds may be too high!\n");
    DEBUG("Suggested values based on typical signal:\n");
    DEBUG("  Enter RSSI: ~55-60 (baseline + 15)\n");
    DEBUG("  Exit RSSI: ~48-50 (baseline + 5)\n");
    DEBUG("Use Calibration Wizard to set optimal values.\n");
    DEBUG("====================\n\n");
    
    raceStartTimeMs = millis();
    startTimeMs = raceStartTimeMs;  // Initialize start time for min lap check
    state = RUNNING;
    rssiPeak = 0;  // Clear any spurious peak values
    rssiPeakTimeMs = 0;
    gateExited = true;  // Start assuming we're outside the gate
    buz->beep(500);
    led->on(500);
#ifdef ESP32S3
    // Flash green for race start
    if (g_rgbLed) g_rgbLed->flashGreen();
#endif
}

void LapTimer::stop() {
    DEBUG("LapTimer stopped\n");
    state = STOPPED;
    lapCountWraparound = false;
    lapCount = 0;
    rssiCount = 0;
    rssiPeak = 0;  // Clear peak tracking
    rssiPeakTimeMs = 0;
    startTimeMs = 0;
    gateExited = true;
    memset(lapTimes, 0, sizeof(lapTimes));
    buz->beep(500);
    led->on(500);
#ifdef ESP32S3
    // Flash red 3 times for race reset, then turn off
    if (g_rgbLed) g_rgbLed->flashReset();
#endif
}

void LapTimer::handleLapTimerUpdate(uint32_t currentTimeMs) {
    // Read RSSI and apply two-stage filtering:
    // 1. Kalman filter for adaptive smoothing
    // 2. Moving average for additional noise reduction
    uint8_t rawRssi = rx->readRssi();
    uint8_t kalman_filtered = round(filter.filter(rawRssi, 0));
    
    // Small moving average (3 samples) - hardware cap provides main filtering
    rssi_window[rssi_window_index] = kalman_filtered;
    rssi_window_index = (rssi_window_index + 1) % 3;
    
    // Calculate moving average over 3 samples
    uint16_t sum = 0;
    for (int i = 0; i < 3; i++) {
        sum += rssi_window[i];
    }
    rssi[rssiCount] = sum / 3;
    
    // RSSI debug output disabled for cleaner serial monitor
    // Uncomment below to re-enable RSSI filtering debug:
    // static uint32_t debugCounter = 0;
    // if (state == RUNNING && debugCounter++ % 50 == 0) {
    //     DEBUG("Raw: %u -> Kalman: %u -> Avg: %u | Peak: %u, Time: %u ms\n", 
    //           rawRssi, kalman_filtered, rssi[rssiCount], rssiPeak, currentTimeMs - startTimeMs);
    // }

    switch (state) {
        case STOPPED:
            break;
        case WAITING:
            // detect hole shot
            lapPeakCapture();
            if (lapPeakCaptured()) {
                state = RUNNING;
                startLap();
            }
            break;
        case RUNNING: {
            // Gate 1 (first lap) bypasses minimum lap time check
            // All subsequent laps must respect minimum lap time
            bool isGate1 = (lapCount == 0 && !lapCountWraparound);
            bool minLapElapsed = (currentTimeMs - startTimeMs) > conf->getMinLapMs();
            
            if (isGate1 || minLapElapsed) {
                // Capture peaks and detect laps
                lapPeakCapture();
                
                // Check for lap completion
                if (lapPeakCaptured()) {
                    DEBUG("Lap triggered! Time: %u ms (Gate 1: %s)\n", 
                          currentTimeMs - startTimeMs, isGate1 ? "YES" : "NO");
                    finishLap();
                    startLap();
                }
            }
            break;
        }
        case CALIBRATION_WIZARD:
            // Record RSSI data without triggering lap detection
            // Sample every 20ms (50Hz) for longer recording duration with good resolution
            if (calibrationRssiCount < LAPTIMER_CALIBRATION_HISTORY && 
                (currentTimeMs - lastCalibrationSampleMs) >= 20) {
                calibrationRssi[calibrationRssiCount] = rssi[rssiCount];
                calibrationTimestamps[calibrationRssiCount] = currentTimeMs;
                calibrationRssiCount++;
                lastCalibrationSampleMs = currentTimeMs;
            }
            break;
        default:
            break;
    }

    rssiCount = (rssiCount + 1) % LAPTIMER_RSSI_HISTORY;
}

void LapTimer::lapPeakCapture() {
    // Capture any RSSI above enter threshold as a potential peak
    if (rssi[rssiCount] >= conf->getEnterRssi()) {
        if (rssi[rssiCount] > rssiPeak) {
            rssiPeak = rssi[rssiCount];
            rssiPeakTimeMs = millis();
            DEBUG("*** PEAK CAPTURED: %u at time %u ms (since lap start: %u ms) ***\n", 
                  rssiPeak, rssiPeakTimeMs, rssiPeakTimeMs - startTimeMs);
        }
    }
}

bool LapTimer::lapPeakCaptured() {
    // Lap detection with strict validation:
    // 1. Must have captured a peak
    // 2. Peak must have crossed enter threshold (not just noise)
    // 3. Peak must be significantly above exit threshold (at least 5 RSSI units)
    // 4. Current RSSI must have dropped back below exit threshold
    
    bool validPeak = (rssiPeak > 0) && 
                     (rssiPeak >= conf->getEnterRssi()) && 
                     (rssiPeak > (conf->getExitRssi() + 5));  // Peak must be well above exit
    
    bool droppedBelowExit = (rssi[rssiCount] < conf->getExitRssi());
    
    bool captured = validPeak && droppedBelowExit;
    
    if (captured) {
        DEBUG("\n*** LAP DETECTED! ***\n");
        DEBUG("  Current RSSI: %u\n", rssi[rssiCount]);
        DEBUG("  Peak was: %u\n", rssiPeak);
        DEBUG("  Enter threshold: %u\n", conf->getEnterRssi());
        DEBUG("  Exit threshold: %u\n", conf->getExitRssi());
        DEBUG("  Peak margin above exit: %d\n", rssiPeak - conf->getExitRssi());
        DEBUG("******************\n\n");
    }
    
    return captured;
}

void LapTimer::startLap() {
    DEBUG("Lap started - Peak was %u, new lap begins\n", rssiPeak);
    startTimeMs = rssiPeakTimeMs;
    rssiPeak = 0;  // Reset peak for next lap
    rssiPeakTimeMs = 0;
    buz->beep(200);
    led->on(200);
}

void LapTimer::finishLap() {
    lapTimes[lapCount] = rssiPeakTimeMs - startTimeMs;
    if (lapCount == 0 && lapCountWraparound == false)
    {
        lapTimes[0] = rssiPeakTimeMs - raceStartTimeMs;
    }
    else
    {
        lapTimes[lapCount] = rssiPeakTimeMs - startTimeMs;
    }
    DEBUG("Lap finished, lap time = %u\n", lapTimes[lapCount]);
    if ((lapCount + 1) % LAPTIMER_LAP_HISTORY == 0) {
        lapCountWraparound = true;
    }
    lapCount = (lapCount + 1) % LAPTIMER_LAP_HISTORY;
    lapAvailable = true;
#ifdef ESP32S3
    if (g_rgbLed) g_rgbLed->flashLap();
#endif
}

uint8_t LapTimer::getRssi() {
    return rssi[rssiCount];
}

uint32_t LapTimer::getLapTime() {
    uint32_t lapTime = 0;
    lapAvailable = false;
    if (lapCount == 0) {
        lapTime = lapTimes[LAPTIMER_LAP_HISTORY - 1];
    } else {
        lapTime = lapTimes[lapCount - 1];
    }
    return lapTime;
}

bool LapTimer::isLapAvailable() {
    return lapAvailable;
}

void LapTimer::startCalibrationWizard() {
    DEBUG("Calibration wizard started\n");
    state = CALIBRATION_WIZARD;
    calibrationRssiCount = 0;
    lastCalibrationSampleMs = 0;  // Reset sample timing
    memset(calibrationRssi, 0, sizeof(calibrationRssi));
    memset(calibrationTimestamps, 0, sizeof(calibrationTimestamps));
    buz->beep(300);
    led->on(300);
#ifdef ESP32S3
    if (g_rgbLed) g_rgbLed->flashGreen();
#endif
}

void LapTimer::stopCalibrationWizard() {
    DEBUG("Calibration wizard stopped, recorded %u samples\n", calibrationRssiCount);
    state = STOPPED;
    buz->beep(300);
    led->on(300);
#ifdef ESP32S3
    if (g_rgbLed) g_rgbLed->flashReset();
#endif
}

uint16_t LapTimer::getCalibrationRssiCount() {
    return calibrationRssiCount;
}

uint8_t LapTimer::getCalibrationRssi(uint16_t index) {
    if (index < calibrationRssiCount) {
        return calibrationRssi[index];
    }
    return 0;
}

uint32_t LapTimer::getCalibrationTimestamp(uint16_t index) {
    if (index < calibrationRssiCount) {
        return calibrationTimestamps[index];
    }
    return 0;
}
