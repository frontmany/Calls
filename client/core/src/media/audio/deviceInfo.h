#pragma once

#include <string>

namespace core::media
{
    struct DeviceInfo {
        int deviceIndex;
        // PortAudio device "host api" index (WASAPI/DirectSound/MME/etc).
        // Using it allows filtering duplicates that come from multiple host APIs.
        int hostApiIndex;
        std::string name;
        int maxInputChannels;
        int maxOutputChannels;
        double defaultLowInputLatency;
        double defaultLowOutputLatency;
        double defaultHighInputLatency;
        double defaultHighOutputLatency;
        double defaultSampleRate;
        bool isDefaultInput;
        bool isDefaultOutput;
    };
}