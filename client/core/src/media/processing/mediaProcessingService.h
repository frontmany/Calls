#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <mutex>
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
    private:
        struct VideoPipeline {
            std::unique_ptr<H264Encoder> encoder;
            std::unique_ptr<H264Decoder> decoder;
            std::vector<unsigned char> lastEncodedFrame;
            std::vector<unsigned char> lastDecodedFrame;
            int width = 0;
            int height = 0;
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
            
        void cleanupAudio();
        void cleanupVideo(MediaType type);

        std::vector<unsigned char> encodeAudioFrame(const float* pcmData);
        std::vector<float> decodeAudioFrame(const unsigned char* opusData, int dataSize);

        std::vector<unsigned char> encodeVideoFrame(MediaType type, const unsigned char* rawData, int width, int height);
        std::vector<unsigned char> decodeVideoFrame(MediaType type, const unsigned char* h264Data, int dataSize);
        
        std::vector<unsigned char> encryptData(const unsigned char* data, int size, const std::vector<unsigned char>& key);
        std::vector<unsigned char> decryptData(const unsigned char* encryptedData, int size, const CryptoPP::SecByteBlock& key);

        bool isAudioInitialized() const { return m_audioInitialized; }
        bool isVideoInitialized(MediaType type) const;

        int getSampleRate() const { return m_sampleRate; }
        int getChannels() const { return m_channels; }
        int getFrameSize() const { return m_frameSize; }

        int getWidth(MediaType type) const;
        int getHeight(MediaType type) const;

    private:
        std::unique_ptr<OpusEncoder> m_audioEncoder;
        std::unique_ptr<OpusDecoder> m_audioDecoder;
        std::unordered_map<MediaType, VideoPipeline, MediaTypeHash> m_videoPipelines;
        std::unique_ptr<MediaEncryptionService> m_encryptionService;
        mutable std::mutex m_encryptionMutex;

        int m_sampleRate;
        int m_channels;
        int m_frameSize;
        bool m_audioInitialized;

        bool initializeAudioEncoder(int sampleRate, int channels, int frameSize);
        bool initializeAudioDecoder(int sampleRate, int channels);

        VideoPipeline& getPipeline(MediaType type);
    };
}
