#pragma once

#include "videoFrameBuffer.h"

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

namespace core
{
    /**
     * Buffers the latest frame per logical stream and schedules a single UI-thread flush per burst.
     */
    class VideoFrameCoalescer
    {
    public:
        using ScheduleOnUi = std::function<void(std::function<void()>)>;

        VideoFrameCoalescer(
            ScheduleOnUi scheduleOnUi,
            std::function<void(const VideoFrameBuffer&)> onIncomingScreen,
            std::function<void(const VideoFrameBuffer&)> onLocalScreen,
            std::function<void(const VideoFrameBuffer&)> onLocalCamera,
            std::function<void(const VideoFrameBuffer&, const std::string&)> onIncomingCamera);

        void pushIncomingScreen(const VideoFrameBuffer& frame);
        void pushLocalScreen(const VideoFrameBuffer& frame);
        void pushLocalCamera(const VideoFrameBuffer& frame);
        void pushIncomingCamera(const VideoFrameBuffer& frame, const std::string& nickname);

        void dropPendingRemoteCamera(const std::string& nickname);

    private:
        struct PendingVideo
        {
            VideoFrameBuffer frame;
            bool dirty = false;
        };

        struct RemoteCamPending
        {
            VideoFrameBuffer frame;
            bool dirty = false;
        };

        void scheduleFlushIfNeeded();
        bool hasPendingVideoLocked() const;
        void flushPendingToCallbacks();

        ScheduleOnUi m_scheduleOnUi;
        std::function<void(const VideoFrameBuffer&)> m_onIncomingScreen;
        std::function<void(const VideoFrameBuffer&)> m_onLocalScreen;
        std::function<void(const VideoFrameBuffer&)> m_onLocalCamera;
        std::function<void(const VideoFrameBuffer&, const std::string&)> m_onIncomingCamera;

        std::mutex m_mutex;
        PendingVideo m_incomingScreen;
        PendingVideo m_localScreen;
        PendingVideo m_localCamera;
        std::unordered_map<std::string, RemoteCamPending> m_incomingCameraByNickname;

        std::atomic<bool> m_uiFlushPending{false};
    };
}
