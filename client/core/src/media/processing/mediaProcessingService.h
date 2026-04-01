#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

#include "secblock.h"
#include "media/mediaType.h"

namespace core::media
{
    class H264Encoder;
    class H264Decoder;
    class MediaEncryptionService;
    class OpusEncoder;
    class OpusDecoder;

    class MediaProcessingService {
    public:
        struct VideoProfile {
            int width = 0;
            int height = 0;
            int fps = 30;
            int bitrate = 1800000;
        };
        enum class CameraLayer : uint8_t {
            Low = 0,
            Mid = 1,
            High = 2,
        };

    private:
        struct VideoPipeline {
            std::unique_ptr<H264Encoder> encoder;
            std::unique_ptr<H264Decoder> decoder;
            std::vector<unsigned char> lastEncodedFrame;
            std::vector<unsigned char> lastDecodedFrame;
            int width = 0;
            int height = 0;
            int fps = 30;
            int bitrate = 1800000;
            bool initialized = false;

            void cleanup();
        };

        struct MediaTypeHash {
            std::size_t operator()(MediaType t) const {
                return std::hash<int>()(static_cast<int>(t));
            }
        };

    public:
        MediaProcessingService();
        ~MediaProcessingService();

        bool initializeAudioProcessing(int sampleRate = 48000, int channels = 1, int frameSize = 960);
        bool initializeVideoProcessing(MediaType type, int bitrate);
        void setCameraQualityProfiles(const VideoProfile& low, const VideoProfile& mid, const VideoProfile& high);
        void setScreenQualityProfile(const VideoProfile& baseProfile, const VideoProfile& minProfile);
        void setCameraTargetLayer(CameraLayer layer);
            
        void cleanupAudio();
        void cleanupVideo(MediaType type);

        std::vector<unsigned char> encodeAudioFrame(const float* pcmData);
        std::vector<float> decodeAudioFrame(const unsigned char* opusData, int dataSize);

        std::vector<unsigned char> encodeVideoFrame(MediaType type, const unsigned char* rawData, int width, int height);
        std::vector<std::pair<CameraLayer, std::vector<unsigned char>>> encodeCameraSimulcastFrames(const unsigned char* rawData, int width, int height);
        std::vector<unsigned char> decodeVideoFrame(MediaType type, const unsigned char* h264Data, int dataSize);
        std::vector<unsigned char> decodeVideoFrame(MediaType type, const std::string& streamKey, const unsigned char* h264Data, int dataSize);
        
        std::vector<unsigned char> encryptData(const unsigned char* data, int size, const std::vector<unsigned char>& key);
        std::vector<unsigned char> decryptData(const unsigned char* encryptedData, int size, const CryptoPP::SecByteBlock& key);

        bool isAudioInitialized() const { return m_audioInitialized; }
        bool isVideoInitialized(MediaType type) const;

        int getSampleRate() const { return m_sampleRate; }
        int getChannels() const { return m_channels; }
        int getFrameSize() const { return m_frameSize; }

        int getWidth(MediaType type) const;
        int getHeight(MediaType type) const;
        int getWidth(MediaType type, const std::string& streamKey) const;
        int getHeight(MediaType type, const std::string& streamKey) const;

    private:
        std::unique_ptr<OpusEncoder> m_audioEncoder;
        std::unique_ptr<OpusDecoder> m_audioDecoder;
        std::unordered_map<MediaType, VideoPipeline, MediaTypeHash> m_videoPipelines;
        std::unordered_map<CameraLayer, VideoPipeline> m_cameraEncodePipelines;
        std::unordered_map<std::string, VideoPipeline> m_cameraDecodePipelines;
        std::unique_ptr<MediaEncryptionService> m_encryptionService;
        mutable std::mutex m_encryptionMutex;
        VideoProfile m_cameraLowProfile{ 320, 180, 30, 300000 };
        VideoProfile m_cameraMidProfile{ 640, 360, 30, 900000 };
        VideoProfile m_cameraHighProfile{ 1280, 720, 30, 2200000 };
        VideoProfile m_screenBaseProfile{ 1920, 1080, 30, 3500000 };
        VideoProfile m_screenMinProfile{ 1280, 720, 20, 1800000 };
        CameraLayer m_cameraTargetLayer = CameraLayer::High;

        int m_sampleRate;
        int m_channels;
        int m_frameSize;
        bool m_audioInitialized;

        bool initializeAudioEncoder(int sampleRate, int channels, int frameSize);
        bool initializeAudioDecoder(int sampleRate, int channels);

        VideoPipeline& getPipeline(MediaType type);
    };
}
