#pragma once

#include <cmath>

namespace core::constant {

// RMS above this threshold is considered "speaking" for meeting participant highlight.
constexpr float kSpeakingRmsThreshold = 0.01f;

// Number of consecutive low-RMS frames before switching to "not speaking" (~300-400 ms at typical callback rate).
constexpr int kSpeakingSilenceFrames = 20;

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

}
