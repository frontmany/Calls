#pragma once

#include <string>

namespace core
{
    namespace audio
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
}