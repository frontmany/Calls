#include "screenCaptureService.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#include <winuser.h>
#elif defined(__APPLE__)
#include <CoreGraphics/CGDisplayConfiguration.h>
#else
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#endif

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace core::media
{
    namespace
    {
        bool hasGeometry(const ScreenCaptureTarget& t)
        {
            return t.width > 0 && t.height > 0;
        }
    }

#ifdef _WIN32
    namespace
    {
        struct MonitorCandidate
        {
            int index = -1;
            RECT rect{ 0, 0, 0, 0 };
            std::string deviceName;
        };

        std::string normalizeDisplayId(const std::string& id)
        {
            std::string normalized;
            normalized.reserve(id.size());

            for (char ch : id) {
                if (ch == '\\' || ch == '.') {
                    continue;
                }
                normalized.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
            }

            return normalized;
        }

    }
#endif

    ScreenCaptureService::ScreenCaptureService()
        : m_formatContext(nullptr)
        , m_codecContext(nullptr)
        , m_frame(nullptr)
        , m_frameRGB(nullptr)
        , m_swsContext(nullptr)
        , m_buffer(nullptr)
        , m_isRunning(false)
        , m_shouldStop(false)
        , m_screenIndex(0)
        , m_frameWidth(0)
        , m_frameHeight(0)
        , m_captureThread(nullptr)
    {
    }

    ScreenCaptureService::~ScreenCaptureService()
    {
        stop();
    }

    bool ScreenCaptureService::start(const ScreenCaptureTarget& target)
    {
        if (m_isRunning) {
            std::cerr << "Screen capture is already running" << std::endl;
            return false;
        }

        m_target = target;
        m_screenIndex = target.index;

        if (!initializeScreenCapture(target)) {
            return false;
        }

        m_shouldStop = false;
        m_isRunning = true;

        m_captureThread = new std::thread([this]() {
            while (!m_shouldStop && m_isRunning) {
                captureFrame();
                std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 FPS
            }
        });

        std::cout << "Screen capture started successfully for target index " << target.index
            << ", osId '" << target.osId << "'" << std::endl;
        return true;
    }

    void ScreenCaptureService::stop()
    {
        if (!m_isRunning) {
            return;
        }

        m_shouldStop = true;
        m_isRunning = false;

        if (m_captureThread) {
            m_captureThread->join();
            delete m_captureThread;
            m_captureThread = nullptr;
        }

        cleanup();
        std::cout << "Screen capture stopped" << std::endl;
    }

    bool ScreenCaptureService::isRunning() const
    {
        return m_isRunning;
    }

    void ScreenCaptureService::setFrameCallback(FrameCallback callback)
    {
        m_frameCallback = callback;
    }

    bool ScreenCaptureService::initializeScreenCapture(const ScreenCaptureTarget& target)
    {
        const int screenIndex = target.index;
        int width = 0, height = 0;
        char deviceName[256];

#ifdef _WIN32
        std::vector<MonitorCandidate> monitors;
        EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR hMonitor, HDC, LPRECT, LPARAM dwData) -> BOOL {
            auto* monitorList = reinterpret_cast<std::vector<MonitorCandidate>*>(dwData);
            MONITORINFOEX mi;
            mi.cbSize = sizeof(MONITORINFOEX);

            if (GetMonitorInfo(hMonitor, &mi)) {
                MonitorCandidate candidate;
                candidate.index = static_cast<int>(monitorList->size());
                candidate.rect = mi.rcMonitor;
                candidate.deviceName = mi.szDevice;
                monitorList->push_back(candidate);
            }

            return TRUE;
        }, reinterpret_cast<LPARAM>(&monitors));

        if (monitors.empty()) {
            std::cerr << "No monitors found for screen capture" << std::endl;
            return false;
        }

        const MonitorCandidate* selected = nullptr;

        if (!target.osId.empty()) {
            const std::string normalizedTargetId = normalizeDisplayId(target.osId);
            const auto it = std::find_if(monitors.begin(), monitors.end(), [&normalizedTargetId](const MonitorCandidate& candidate) {
                return normalizeDisplayId(candidate.deviceName) == normalizedTargetId;
            });
            if (it != monitors.end()) {
                selected = &(*it);
                std::cout << "Screen selected by OS id '" << target.osId
                    << "' -> monitor " << selected->index
                    << " (" << selected->deviceName << ")" << std::endl;
            }
        }

        if (!selected && hasGeometry(target)) {
            const auto it = std::find_if(monitors.begin(), monitors.end(), [&target](const MonitorCandidate& candidate) {
                const int candidateWidth = candidate.rect.right - candidate.rect.left;
                const int candidateHeight = candidate.rect.bottom - candidate.rect.top;
                return candidate.rect.left == target.x &&
                    candidate.rect.top == target.y &&
                    candidateWidth == target.width &&
                    candidateHeight == target.height;
            });
            if (it != monitors.end()) {
                selected = &(*it);
                std::cout << "Screen selected by geometry (" << target.x << "," << target.y << ","
                    << target.width << "x" << target.height << ") -> monitor " << selected->index
                    << " (" << selected->deviceName << ")" << std::endl;
            }
        }

        if (!selected && screenIndex >= 0 && screenIndex < static_cast<int>(monitors.size())) {
            selected = &monitors[screenIndex];
            std::cout << "Screen selected by fallback index " << screenIndex
                << " -> monitor (" << selected->deviceName << ")" << std::endl;
        }

        if (!selected) {
            selected = &monitors.front();
            std::cout << "Falling back to first monitor (" << selected->deviceName << ")" << std::endl;
        }

        width = selected->rect.right - selected->rect.left;
        height = selected->rect.bottom - selected->rect.top;

        snprintf(deviceName, sizeof(deviceName), "desktop");

        std::cout << "Monitor " << selected->index << " at ("
            << selected->rect.left << "," << selected->rect.top
            << ") size " << width << "x" << height << std::endl;

        int offsetX = selected->rect.left;
        int offsetY = selected->rect.top;

        m_formatContext = avformat_alloc_context();
        if (!m_formatContext) {
            std::cerr << "Failed to allocate format context" << std::endl;
            return false;
        }

        const AVInputFormat* inputFormat = nullptr;
        inputFormat = av_find_input_format("gdigrab");

        if (!inputFormat) {
            std::cerr << "Failed to find input format" << std::endl;
            cleanup();
            return false;
        }

        AVDictionary* options = nullptr;
        av_dict_set(&options, "framerate", "30", 0);
        std::string videoSize = std::to_string(width) + "x" + std::to_string(height);
        av_dict_set(&options, "video_size", videoSize.c_str(), 0);

        char offsetXStr[32];
        char offsetYStr[32];
        snprintf(offsetXStr, sizeof(offsetXStr), "%d", offsetX);
        snprintf(offsetYStr, sizeof(offsetYStr), "%d", offsetY);
        av_dict_set(&options, "offset_x", offsetXStr, 0);
        av_dict_set(&options, "offset_y", offsetYStr, 0);

        std::cout << "Setting capture offset to (" << offsetX << "," << offsetY << ") for monitor " << selected->index << std::endl;
#elif defined(__APPLE__)
        CGDirectDisplayID displays[32];
        uint32_t displayCount = 0;

        CGError error = CGGetActiveDisplayList(32, displays, &displayCount);
        if (error != kCGErrorSuccess || displayCount == 0) {
            std::cerr << "Failed to get display list" << std::endl;
            return false;
        }

        int selectedIndex = screenIndex >= 0 && screenIndex < static_cast<int>(displayCount) ? screenIndex : 0;

        if (hasGeometry(target) && target.dpr > 0) {
            const double scale = target.dpr;
            const int lx = static_cast<int>(std::lround(target.x / scale));
            const int ly = static_cast<int>(std::lround(target.y / scale));
            const int lw = static_cast<int>(std::lround(target.width / scale));
            const int lh = static_cast<int>(std::lround(target.height / scale));
            for (uint32_t i = 0; i < displayCount; ++i) {
                CGRect bounds = CGDisplayBounds(displays[i]);
                if (static_cast<int>(bounds.origin.x) == lx && static_cast<int>(bounds.origin.y) == ly &&
                    static_cast<int>(bounds.size.width) == lw && static_cast<int>(bounds.size.height) == lh) {
                    selectedIndex = static_cast<int>(i);
                    std::cout << "Screen selected by geometry (" << target.x << "," << target.y << ","
                        << target.width << "x" << target.height << ") -> display " << selectedIndex << std::endl;
                    break;
                }
            }
        } else if (screenIndex >= static_cast<int>(displayCount)) {
            std::cerr << "Invalid display index: " << screenIndex << " (max: " << (displayCount - 1) << "), using 0" << std::endl;
            selectedIndex = 0;
        }

        CGDirectDisplayID displayID = displays[selectedIndex];
        width = CGDisplayPixelsWide(displayID);
        height = CGDisplayPixelsHigh(displayID);

        snprintf(deviceName, sizeof(deviceName), "display:%d", selectedIndex);

        std::cout << "Display " << selectedIndex << " size " << width << "x" << height << std::endl;

        m_formatContext = avformat_alloc_context();
        if (!m_formatContext) {
            std::cerr << "Failed to allocate format context" << std::endl;
            return false;
        }

        const AVInputFormat* inputFormat = av_find_input_format("avfoundation");

        if (!inputFormat) {
            std::cerr << "Failed to find input format" << std::endl;
            cleanup();
            return false;
        }

        AVDictionary* options = nullptr;
        av_dict_set(&options, "framerate", "30", 0);
        std::string videoSize = std::to_string(width) + "x" + std::to_string(height);
        av_dict_set(&options, "video_size", videoSize.c_str(), 0);
        av_dict_set(&options, "pixel_format", "bgr0", 0);
#else
        int offsetX = 0;
        int offsetY = 0;

        if (hasGeometry(target)) {
            offsetX = target.x;
            offsetY = target.y;
            width = target.width;
            height = target.height;
            snprintf(deviceName, sizeof(deviceName), ":0.0+%d,%d", offsetX, offsetY);
            std::cout << "Screen selected by geometry: offset (" << offsetX << "," << offsetY
                << ") size " << width << "x" << height << std::endl;
        } else {
            Display* display = XOpenDisplay(nullptr);
            if (!display) {
                std::cerr << "Failed to open X11 display" << std::endl;
                return false;
            }

            int screenCount = ScreenCount(display);
            if (screenIndex >= screenCount || screenIndex < 0) {
                std::cerr << "Invalid screen index: " << screenIndex << " (max: " << (screenCount - 1) << "), using 0" << std::endl;
                screenIndex = 0;
            }

            Screen* screen = ScreenOfDisplay(display, screenIndex);
            width = screen->width;
            height = screen->height;
            offsetX = screenIndex > 0 ? screen->x : 0;
            offsetY = screenIndex > 0 ? screen->y : 0;

            snprintf(deviceName, sizeof(deviceName), ":0.0+%d,%d", offsetX, offsetY);
            XCloseDisplay(display);
            std::cout << "Screen " << screenIndex << " at (" << offsetX << "," << offsetY << ") size " << width << "x" << height << std::endl;
        }

        m_formatContext = avformat_alloc_context();
        if (!m_formatContext) {
            std::cerr << "Failed to allocate format context" << std::endl;
            return false;
        }

        const AVInputFormat* inputFormat = av_find_input_format("x11grab");

        if (!inputFormat) {
            std::cerr << "Failed to find input format" << std::endl;
            cleanup();
            return false;
        }

        AVDictionary* options = nullptr;
        av_dict_set(&options, "framerate", "30", 0);
        std::string videoSize = std::to_string(width) + "x" + std::to_string(height);
        av_dict_set(&options, "video_size", videoSize.c_str(), 0);
#endif
        if (avformat_open_input(&m_formatContext, deviceName, inputFormat, &options) != 0) {
            std::cerr << "Failed to open screen capture device: " << deviceName << std::endl;
            cleanup();
            return false;
        }

        if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
            std::cerr << "Failed to find stream information" << std::endl;
            cleanup();
            return false;
        }

        int videoStreamIndex = av_find_best_stream(m_formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (videoStreamIndex < 0) {
            std::cerr << "Failed to find video stream" << std::endl;
            cleanup();
            return false;
        }

        AVCodecParameters* codecParams = m_formatContext->streams[videoStreamIndex]->codecpar;

        const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
        if (!codec) {
            std::cerr << "Failed to find codec" << std::endl;
            cleanup();
            return false;
        }

        m_codecContext = avcodec_alloc_context3(codec);
        if (!m_codecContext) {
            std::cerr << "Failed to allocate codec context" << std::endl;
            cleanup();
            return false;
        }

        if (avcodec_parameters_to_context(m_codecContext, codecParams) < 0) {
            std::cerr << "Failed to copy codec parameters" << std::endl;
            cleanup();
            return false;
        }

        if (avcodec_open2(m_codecContext, codec, nullptr) < 0) {
            std::cerr << "Failed to open codec" << std::endl;
            cleanup();
            return false;
        }

        m_frame = av_frame_alloc();
        m_frameRGB = av_frame_alloc();
        if (!m_frame || !m_frameRGB) {
            std::cerr << "Failed to allocate frames" << std::endl;
            cleanup();
            return false;
        }

        int frameWidth = m_codecContext->width;
        int frameHeight = m_codecContext->height;

        std::cout << "Codec context dimensions: " << frameWidth << "x" << frameHeight << std::endl;

        if (frameWidth <= 0 || frameHeight <= 0) {
            AVStream* videoStream = m_formatContext->streams[videoStreamIndex];
            if (videoStream && videoStream->codecpar) {
                frameWidth = videoStream->codecpar->width;
                frameHeight = videoStream->codecpar->height;
                std::cout << "Stream dimensions: " << frameWidth << "x" << frameHeight << std::endl;
            }
        }

        if (frameWidth <= 0 || frameHeight <= 0) {
            frameWidth = width;
            frameHeight = height;
            std::cout << "Using screen dimensions: " << frameWidth << "x" << frameHeight << std::endl;
        }

        m_frameWidth = frameWidth;
        m_frameHeight = frameHeight;

        int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, frameWidth, frameHeight, 1);
        m_buffer = (uint8_t*)av_malloc(bufferSize * sizeof(uint8_t));

        if (!m_buffer) {
            std::cerr << "Failed to allocate buffer" << std::endl;
            cleanup();
            return false;
        }

        av_image_fill_arrays(m_frameRGB->data, m_frameRGB->linesize, m_buffer,
            AV_PIX_FMT_RGB24, frameWidth, frameHeight, 1);

        m_swsContext = sws_getContext(frameWidth, frameHeight, m_codecContext->pix_fmt,
            frameWidth, frameHeight, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, nullptr, nullptr, nullptr);

        if (!m_swsContext) {
            std::cerr << "Failed to initialize scale context" << std::endl;
            cleanup();
            return false;
        }

        std::cout << "Screen capture initialized: " << width << "x" << height << std::endl;
        return true;
    }

    void ScreenCaptureService::captureFrame()
    {
        if (!m_formatContext || !m_codecContext || !m_frame) {
            return;
        }

        AVPacket* packet = av_packet_alloc();
        if (!packet) return;

        int ret = av_read_frame(m_formatContext, packet);
        if (ret < 0) {
            av_packet_free(&packet);
            return;
        }

        ret = avcodec_send_packet(m_codecContext, packet);
        av_packet_free(&packet);

        if (ret < 0) {
            return;
        }

        ret = avcodec_receive_frame(m_codecContext, m_frame);
        if (ret < 0) {
            return;
        }

        sws_scale(m_swsContext, m_frame->data, m_frame->linesize, 0, m_frame->height,
            m_frameRGB->data, m_frameRGB->linesize);

        Frame frameData;
        frameData.data = m_frameRGB->data[0];
        frameData.width = m_frameWidth;
        frameData.height = m_frameHeight;
        frameData.format = AV_PIX_FMT_RGB24;
        frameData.pts = m_frame->pts;
        frameData.size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, m_frameWidth, m_frameHeight, 1);

        if (m_frameCallback) {
            m_frameCallback(frameData);
        }
    }

    void ScreenCaptureService::cleanup()
    {
        if (m_swsContext) {
            sws_freeContext(m_swsContext);
            m_swsContext = nullptr;
        }

        if (m_buffer) {
            av_free(m_buffer);
            m_buffer = nullptr;
        }

        if (m_frameRGB) {
            av_frame_free(&m_frameRGB);
        }

        if (m_frame) {
            av_frame_free(&m_frame);
        }

        if (m_codecContext) {
            avcodec_free_context(&m_codecContext);
        }

        if (m_formatContext) {
            avformat_close_input(&m_formatContext);
            avformat_free_context(m_formatContext);
            m_formatContext = nullptr;
        }
    }
}