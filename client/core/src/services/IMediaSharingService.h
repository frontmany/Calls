#pragma once

#include <vector>
#include <system_error>

namespace core
{
    namespace services
    {
        // Интерфейс для управления screen/camera sharing
        class IMediaSharingService {
        public:
            virtual ~IMediaSharingService() = default;

            virtual std::error_code startScreenSharing() = 0;
            virtual std::error_code stopScreenSharing() = 0;
            virtual std::error_code sendScreen(const std::vector<unsigned char>& data) = 0;
            virtual std::error_code startCameraSharing() = 0;
            virtual std::error_code stopCameraSharing() = 0;
            virtual std::error_code sendCamera(const std::vector<unsigned char>& data) = 0;
        };
    }
}
