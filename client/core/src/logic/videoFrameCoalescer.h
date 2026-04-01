#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace core
{
    /**
     * Buffers the latest RGB frame per logical stream and schedules a single UI-thread flush per burst.
     * Not an EventListener: UI wires CoreEventListener to forward video callbacks here.
     */
    class VideoFrameCoalescer
    {
    public:
        using ScheduleOnUi = std::function<void(std::function<void()>)>;

        VideoFrameCoalescer(
            ScheduleOnUi scheduleOnUi,
            std::function<void(const std::vector<unsigned char>&, int, int)> onIncomingScreen,
            std::function<void(const std::vector<unsigned char>&, int, int)> onLocalScreen,
            std::function<void(const std::vector<unsigned char>&, int, int)> onLocalCamera,
            std::function<void(const std::vector<unsigned char>&, int, int, const std::string&)> onIncomingCamera);

        void pushIncomingScreen(const std::vector<unsigned char>& data, int width, int height);
        void pushLocalScreen(const std::vector<unsigned char>& data, int width, int height);
        void pushLocalCamera(const std::vector<unsigned char>& data, int width, int height);
        void pushIncomingCamera(const std::vector<unsigned char>& data, int width, int height, const std::string& nickname);

        void dropPendingRemoteCamera(const std::string& nickname);

    private:
        struct RgbFrame
        {
            std::vector<unsigned char> data;
            int width = 0;
            int height = 0;
            bool dirty = false;
        };

        struct RemoteCamFrame
        {
            std::vector<unsigned char> data;
            int width = 0;
            int height = 0;
            std::string nickname;
            bool dirty = false;
        };

        void scheduleFlushIfNeeded();
        bool hasPendingVideoLocked() const;
        void flushPendingToCallbacks();

        ScheduleOnUi m_scheduleOnUi;
        std::function<void(const std::vector<unsigned char>&, int, int)> m_onIncomingScreen;
        std::function<void(const std::vector<unsigned char>&, int, int)> m_onLocalScreen;
        std::function<void(const std::vector<unsigned char>&, int, int)> m_onLocalCamera;
        std::function<void(const std::vector<unsigned char>&, int, int, const std::string&)> m_onIncomingCamera;

        std::mutex m_mutex;
        RgbFrame m_incomingScreen;
        RgbFrame m_localScreen;
        RgbFrame m_localCamera;
        std::unordered_map<std::string, RemoteCamFrame> m_incomingCameraByNickname;

        std::atomic<bool> m_uiFlushPending{false};
    };
}
