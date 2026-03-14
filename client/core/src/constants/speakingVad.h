#pragma once

#include <cmath>

namespace core::constant {

// RMS above this threshold (applied to EMA-smoothed value) is considered "speaking".
constexpr float kSpeakingRmsThreshold = 0.005f;

// Number of consecutive low-RMS frames before switching to "not speaking" (~500-600 ms at typical callback rate).
constexpr int kSpeakingSilenceFrames = 30;

// EMA coefficient: lower = more smoothing, higher = more responsive.
// 0.15 gives ~100 ms half-life at 20 ms frame intervals, bridging natural speech micro-pauses.
constexpr float kSpeakingRmsAlpha = 0.15f;

// Compute RMS of float audio buffer. Returns 0 if length <= 0.
inline float computeRms(const float* data, int length) {
    if (!data || length <= 0) return 0.f;
    double sumSq = 0.;
    for (int i = 0; i < length; ++i) {
        const double x = static_cast<double>(data[i]);
        sumSq += x * x;
    }
    return static_cast<float>(std::sqrt(sumSq / length));
}

// Exponential moving average for RMS to avoid flickering on natural speech micro-pauses.
inline float smoothRms(float previousSmoothed, float currentRms) {
    return kSpeakingRmsAlpha * currentRms + (1.f - kSpeakingRmsAlpha) * previousSmoothed;
}

}
