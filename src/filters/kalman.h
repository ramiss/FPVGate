#ifndef KALMAN_H
#define KALMAN_H

#include <stdint.h>
#include <math.h>

class KalmanFilter {
public:
    KalmanFilter();
    float filter(uint16_t z, uint16_t u = 0);
    float lastMeasurement();
    void setMeasurementNoise(float noise);
    void setProcessNoise(float noise);

private:
    float R;  // Process noise (noise power desirable)
    float Q;  // Measurement noise (noise power estimated)
    float A;  // State transition matrix
    float C;  // Observation matrix
    float B;  // Control input matrix
    float cov;  // Covariance (initialized to NaN)
    float x;    // Estimated signal without noise (initialized to NaN)
};

#endif // KALMAN_H

