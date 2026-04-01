#include "videoFrameCoalescer.h"

#include <utility>

namespace core
{
    VideoFrameCoalescer::VideoFrameCoalescer(
        ScheduleOnUi scheduleOnUi,
        std::function<void(const std::vector<unsigned char>&, int, int)> onIncomingScreen,
        std::function<void(const std::vector<unsigned char>&, int, int)> onLocalScreen,
        std::function<void(const std::vector<unsigned char>&, int, int)> onLocalCamera,
        std::function<void(const std::vector<unsigned char>&, int, int, const std::string&)> onIncomingCamera)
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
            RgbFrame incomingScreen;
            RgbFrame localScreen;
            RgbFrame localCamera;
            std::vector<std::pair<std::string, RemoteCamFrame>> incomingCameras;
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
                    m_incomingScreen = RgbFrame{};
                    hadIncomingScreen = true;
                }
                if (m_localScreen.dirty)
                {
                    localScreen = std::move(m_localScreen);
                    m_localScreen = RgbFrame{};
                    hadLocalScreen = true;
                }
                if (m_localCamera.dirty)
                {
                    localCamera = std::move(m_localCamera);
                    m_localCamera = RgbFrame{};
                    hadLocalCamera = true;
                }
                for (auto it = m_incomingCameraByNickname.begin(); it != m_incomingCameraByNickname.end();)
                {
                    if (it->second.dirty)
                    {
                        std::string nick = it->first;
                        RemoteCamFrame f = std::move(it->second);
                        it = m_incomingCameraByNickname.erase(it);
                        incomingCameras.emplace_back(std::move(nick), std::move(f));
                    }
                    else
                    {
                        ++it;
                    }
                }
            }

            if (hadIncomingScreen && m_onIncomingScreen)
            {
                m_onIncomingScreen(incomingScreen.data, incomingScreen.width, incomingScreen.height);
            }
            if (hadLocalScreen && m_onLocalScreen)
            {
                m_onLocalScreen(localScreen.data, localScreen.width, localScreen.height);
            }
            if (hadLocalCamera && m_onLocalCamera)
            {
                m_onLocalCamera(localCamera.data, localCamera.width, localCamera.height);
            }
            if (m_onIncomingCamera)
            {
                for (const auto& e : incomingCameras)
                {
                    m_onIncomingCamera(e.second.data, e.second.width, e.second.height, e.first);
                }
            }
        }
    }

    void VideoFrameCoalescer::pushIncomingScreen(const std::vector<unsigned char>& data, int width, int height)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_incomingScreen.data = data;
            m_incomingScreen.width = width;
            m_incomingScreen.height = height;
            m_incomingScreen.dirty = true;
        }
        scheduleFlushIfNeeded();
    }

    void VideoFrameCoalescer::pushLocalScreen(const std::vector<unsigned char>& data, int width, int height)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_localScreen.data = data;
            m_localScreen.width = width;
            m_localScreen.height = height;
            m_localScreen.dirty = true;
        }
        scheduleFlushIfNeeded();
    }

    void VideoFrameCoalescer::pushIncomingCamera(
        const std::vector<unsigned char>& data,
        int width,
        int height,
        const std::string& nickname)
    {
        RemoteCamFrame frame;
        frame.data = data;
        frame.width = width;
        frame.height = height;
        frame.nickname = nickname;
        frame.dirty = true;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_incomingCameraByNickname[nickname] = std::move(frame);
        }
        scheduleFlushIfNeeded();
    }

    void VideoFrameCoalescer::pushLocalCamera(const std::vector<unsigned char>& data, int width, int height)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_localCamera.data = data;
            m_localCamera.width = width;
            m_localCamera.height = height;
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
