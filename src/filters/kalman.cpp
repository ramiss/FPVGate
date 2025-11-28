#include "kalman.h"

KalmanFilter::KalmanFilter() {
    R = 1.0f;  // Process noise (noise power desirable)
    Q = 1.0f;  // Measurement noise (noise power estimated)
    A = 1.0f;  // State transition (no change)
    B = 0.0f;  // No control input
    C = 1.0f;  // Direct observation
    cov = NAN; // Initialize to NaN to indicate uninitialized
    x = NAN;   // Initialize to NaN to indicate uninitialized
}

float KalmanFilter::filter(uint16_t z, uint16_t u) {
    if (isnan(x)) {
        // First measurement - initialize state
        x = (1.0f / C) * z;
        cov = (1.0f / C) * Q * (1.0f / C);
    } else {
        // Compute prediction
        const float predX = (A * x) + (B * u);
        const float predCov = ((A * cov) * A) + R;

        // Calculate Kalman gain
        const float K = predCov * C * (1.0f / ((C * predCov * C) + Q));

        // Apply correction
        x = predX + K * (z - (C * predX));
        cov = predCov - (K * C * predCov);
    }

    return x;
}

float KalmanFilter::lastMeasurement() {
    return x;
}

void KalmanFilter::setMeasurementNoise(float noise) {
    Q = noise;
}

void KalmanFilter::setProcessNoise(float noise) {
    R = noise;
}

