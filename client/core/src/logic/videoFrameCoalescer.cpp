#include "videoFrameCoalescer.h"

#include <utility>

namespace core
{
    VideoFrameCoalescer::VideoFrameCoalescer(
        ScheduleOnUi scheduleOnUi,
        std::function<void(const VideoFrameBuffer&)> onIncomingScreen,
        std::function<void(const VideoFrameBuffer&)> onLocalScreen,
        std::function<void(const VideoFrameBuffer&)> onLocalCamera,
        std::function<void(const VideoFrameBuffer&, const std::string&)> onIncomingCamera)
        : m_scheduleOnUi(std::move(scheduleOnUi))
        , m_onIncomingScreen(std::move(onIncomingScreen))
        , m_onLocalScreen(std::move(onLocalScreen))
        , m_onLocalCamera(std::move(onLocalCamera))
        , m_onIncomingCamera(std::move(onIncomingCamera))
    {
    }

    bool VideoFrameCoalescer::hasPendingVideoLocked() const
    {
        if (m_incomingScreen.dirty || m_localScreen.dirty || m_localCamera.dirty)
            return true;
        for (const auto& e : m_incomingCameraByNickname)
        {
            if (e.second.dirty)
                return true;
        }
        return false;
    }

    void VideoFrameCoalescer::scheduleFlushIfNeeded()
    {
        if (!m_scheduleOnUi)
            return;
        if (m_uiFlushPending.exchange(true))
            return;
        VideoFrameCoalescer* self = this;
        m_scheduleOnUi([self]() {
            self->flushPendingToCallbacks();
        });
    }

    void VideoFrameCoalescer::flushPendingToCallbacks()
    {
        while (true)
        {
            PendingVideo incomingScreen;
            PendingVideo localScreen;
            PendingVideo localCamera;
            std::vector<std::pair<std::string, VideoFrameBuffer>> incomingCameras;
            bool hadIncomingScreen = false;
            bool hadLocalScreen = false;
            bool hadLocalCamera = false;

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (!hasPendingVideoLocked())
                {
                    m_uiFlushPending.store(false);
                    return;
                }

                if (m_incomingScreen.dirty)
                {
                    incomingScreen = std::move(m_incomingScreen);
                    m_incomingScreen = PendingVideo{};
                    hadIncomingScreen = true;
                }
                if (m_localScreen.dirty)
                {
                    localScreen = std::move(m_localScreen);
                    m_localScreen = PendingVideo{};
                    hadLocalScreen = true;
                }
                if (m_localCamera.dirty)
                {
                    localCamera = std::move(m_localCamera);
                    m_localCamera = PendingVideo{};
                    hadLocalCamera = true;
                }
                for (auto it = m_incomingCameraByNickname.begin(); it != m_incomingCameraByNickname.end();)
                {
                    if (it->second.dirty)
                    {
                        std::string nick = it->first;
                        RemoteCamPending f = std::move(it->second);
                        it = m_incomingCameraByNickname.erase(it);
                        incomingCameras.emplace_back(std::move(nick), std::move(f.frame));
                    }
                    else
                    {
                        ++it;
                    }
                }
            }

            if (hadIncomingScreen && m_onIncomingScreen)
            {
                m_onIncomingScreen(incomingScreen.frame);
            }
            if (hadLocalScreen && m_onLocalScreen)
            {
                m_onLocalScreen(localScreen.frame);
            }
            if (hadLocalCamera && m_onLocalCamera)
            {
                m_onLocalCamera(localCamera.frame);
            }
            if (m_onIncomingCamera)
            {
                for (const auto& e : incomingCameras)
                {
                    m_onIncomingCamera(e.second, e.first);
                }
            }
        }
    }

    void VideoFrameCoalescer::pushIncomingScreen(const VideoFrameBuffer& frame)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_incomingScreen.frame = frame;
            m_incomingScreen.dirty = true;
        }
        scheduleFlushIfNeeded();
    }

    void VideoFrameCoalescer::pushLocalScreen(const VideoFrameBuffer& frame)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_localScreen.frame = frame;
            m_localScreen.dirty = true;
        }
        scheduleFlushIfNeeded();
    }

    void VideoFrameCoalescer::pushIncomingCamera(const VideoFrameBuffer& frame, const std::string& nickname)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            RemoteCamPending p;
            p.frame = frame;
            p.dirty = true;
            m_incomingCameraByNickname[nickname] = std::move(p);
        }
        scheduleFlushIfNeeded();
    }

    void VideoFrameCoalescer::pushLocalCamera(const VideoFrameBuffer& frame)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_localCamera.frame = frame;
            m_localCamera.dirty = true;
        }
        scheduleFlushIfNeeded();
    }

    void VideoFrameCoalescer::dropPendingRemoteCamera(const std::string& nickname)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_incomingCameraByNickname.erase(nickname);
    }
}
