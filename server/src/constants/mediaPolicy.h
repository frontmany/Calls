#pragma once

#include <cstdint>

namespace server::constant
{
    static constexpr int kStatsTimeoutMs = 5000;
    static constexpr int kFastProbeHoldMs = 2000;
    static constexpr int kReconnectConservativeCallLayer = 1;
    static constexpr int kReconnectConservativeMeetingLayer = 0;

    struct AbrProfile {
        double ewmaAlpha = 0.25;
        double lossDownToMid = 0.0;
        double lossDownToLow = 0.0;
        double lossUpToMid = 0.0;
        double lossUpToHigh = 0.0;
        int rttDownToMidMs = 0;
        int rttDownToLowMs = 0;
        int rttUpToMidMs = 0;
        int rttUpToHighMs = 0;
        int upgradeHoldMs = 0;
        int maxLayerCap = 2;
    };

    // 1:1 calls: preserve quality longer.
    static constexpr AbrProfile kAbrCallProfile{
        0.25, // ewmaAlpha
        4.0,  // lossDownToMid
        8.0,  // lossDownToLow
        2.0,  // lossUpToMid
        1.0,  // lossUpToHigh
        170,  // rttDownToMidMs
        280,  // rttDownToLowMs
        140,  // rttUpToMidMs
        110,  // rttUpToHighMs
        9000, // upgradeHoldMs
        2     // maxLayerCap
    };

    // Meetings: prefer stability and bandwidth efficiency.
    static constexpr AbrProfile kAbrMeetingProfile{
        0.25, // ewmaAlpha
        2.5,  // lossDownToMid
        6.0,  // lossDownToLow
        1.5,  // lossUpToMid
        0.9,  // lossUpToHigh
        130,  // rttDownToMidMs
        220,  // rttDownToLowMs
        110,  // rttUpToMidMs
        90,   // rttUpToHighMs
        11000,// upgradeHoldMs
        1     // maxLayerCap
    };
}

