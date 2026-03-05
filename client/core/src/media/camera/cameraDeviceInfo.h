#pragma once

#include <string>

namespace core::media
{
    struct Camera
    {
        std::string deviceId;   // pass to startCameraSharing()
        std::string displayName;
    };
}
