#pragma once

#include <string>

namespace core::media
{
    struct DeviceInfo {
        int deviceIndex;
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