#include "mediaProcessingService.h"
#include "media/processing/encode/h264Encoder.h"
#include "media/processing/decode/h264Decoder.h"
#include "media/processing/encryption/mediaEncryptionService.h"
#include "media/processing/encode/opusEncoder.h"
#include "media/processing/decode/opusDecoder.h"
#include <libavutil/pixfmt.h>
#include <cstring>
#include <stdexcept>

namespace core::media
{
    void MediaProcessingService::VideoPipeline::cleanup() {
        if (encoder) {
            encoder->cleanup();
            encoder.reset();
        }
        if (decoder) {
            decoder->cleanup();
            decoder.reset();
        }
        lastEncodedFrame.clear();
        lastDecodedFrame.clear();
        width = 0;
        height = 0;
        initialized = false;
    }

    MediaProcessingService::MediaProcessingService()
        : m_sampleRate(0), m_channels(0), m_frameSize(0), m_audioInitialized(false)
    {
    }

    MediaProcessingService::~MediaProcessingService()
    {
        m_audioEncoder.reset();
        m_audioDecoder.reset();
        for (auto& [type, pipeline] : m_videoPipelines) {
            pipeline.cleanup();
        }

        m_videoPipelines.clear();
        m_encryptionService.reset();
    }

    MediaProcessingService::VideoPipeline& MediaProcessingService::getPipeline(MediaType type) {
        return m_videoPipelines[type];
    }

    bool MediaProcessingService::initializeAudioProcessing(int sampleRate, int channels, int frameSize)
    {
        m_sampleRate = sampleRate;
        m_channels = channels;
        m_frameSize = frameSize;
            
        if (!initializeAudioEncoder(sampleRate, channels, frameSize)) {
            return false;
        }
            
        if (!initializeAudioDecoder(sampleRate, channels)) {
            return false;
        }
            
        m_audioInitialized = true;
        return true;
    }

    bool MediaProcessingService::initializeVideoProcessing(MediaType type, int bitrate)
    {
        auto& pipeline = getPipeline(type);
        pipeline.cleanup();
        pipeline.bitrate = bitrate;

        pipeline.encoder = std::make_unique<H264Encoder>();
        pipeline.decoder = std::make_unique<H264Decoder>();

        if (!pipeline.decoder->initialize()) {
            pipeline.encoder.reset();
            pipeline.decoder.reset();
            return false;
        }

        pipeline.initialized = true;
        return true;
    }

    void MediaProcessingService::cleanupAudio()
    {
        m_audioEncoder.reset();
        m_audioDecoder.reset();
        m_audioInitialized = false;
    }

    void MediaProcessingService::cleanupVideo(MediaType type)
    {
        auto it = m_videoPipelines.find(type);
        if (it != m_videoPipelines.end()) {
            it->second.cleanup();
        }
    }

    bool MediaProcessingService::isVideoInitialized(MediaType type) const
    {
        auto it = m_videoPipelines.find(type);
        return it != m_videoPipelines.end() && it->second.initialized;
    }

    int MediaProcessingService::getWidth(MediaType type) const
    {
        auto it = m_videoPipelines.find(type);
        return (it != m_videoPipelines.end()) ? it->second.width : 0;
    }

    int MediaProcessingService::getHeight(MediaType type) const
    {
        auto it = m_videoPipelines.find(type);
        return (it != m_videoPipelines.end()) ? it->second.height : 0;
    }

    std::vector<unsigned char> MediaProcessingService::encodeAudioFrame(const float* pcmData)
    {
        if (!m_audioInitialized || !m_audioEncoder || !m_audioEncoder->isInitialized()) {
            return {};
        }
        const int maxOutputSize = 4000;
        std::vector<unsigned char> encodedData(maxOutputSize);
        int encodedBytes = m_audioEncoder->encode(pcmData, encodedData.data(), maxOutputSize);
        if (encodedBytes < 0) {
            return {};
        }
        encodedData.resize(encodedBytes);
        return encodedData;
    }

    std::vector<float> MediaProcessingService::decodeAudioFrame(const unsigned char* opusData, int dataSize)
    {
        if (!m_audioInitialized || !m_audioDecoder || !m_audioDecoder->isInitialized()) {
            return {};
        }
        std::vector<float> pcmData(m_frameSize * m_channels);
        int decodedSamples = m_audioDecoder->decode(opusData, dataSize, pcmData.data(), m_frameSize, 0);
        if (decodedSamples < 0) {
            return {};
        }
        pcmData.resize(decodedSamples * m_channels);
        return pcmData;
    }

    bool MediaProcessingService::initializeAudioEncoder(int sampleRate, int channels, int frameSize)
    {
        OpusEncoder::Config config;
        config.sampleRate = sampleRate;
        config.channels = channels;
        config.frameSize = frameSize;
        config.application = OpusEncoder::EncoderMode::AUDIO;
        config.bitrate = 64000;
        config.complexity = 5;
        m_audioEncoder = std::make_unique<OpusEncoder>(config);
        return m_audioEncoder->isInitialized();
    }

    bool MediaProcessingService::initializeAudioDecoder(int sampleRate, int channels)
    {
        OpusDecoder::Config config;
        config.sampleRate = sampleRate;
        config.channels = channels;
        m_audioDecoder = std::make_unique<OpusDecoder>(config);
        return m_audioDecoder->isInitialized();
    }

    std::vector<unsigned char> MediaProcessingService::encodeVideoFrame(MediaType type, const unsigned char* rawData, int width, int height)
    {
        auto it = m_videoPipelines.find(type);
        if (it == m_videoPipelines.end() || !it->second.initialized || !it->second.encoder) {
            return {};
        }

        auto& pipeline = it->second;

        // Ленивая инициализация энкодера при первом кадре или реинициализация при смене разрешения
        // (H264Encoder::encodeFrame сам обнаружит смену разрешения и вызовет reinitialize)
        if (!pipeline.encoder->isInitialized()) {
            if (!pipeline.encoder->initialize(width, height, 30, pipeline.bitrate)) {
                return {};
            }
        }
            
        // Очищаем предыдущий результат
        pipeline.lastEncodedFrame.clear();
            
        // Устанавливаем callback для получения закодированных данных
        pipeline.encoder->setEncodedDataCallback([&pipeline](const uint8_t* data, size_t size, int64_t pts) {
            pipeline.lastEncodedFrame.assign(data, data + size);
        });
            
        // Создаем FrameData (AV_PIX_FMT_RGB24 обязателен для sws_scale)
        Frame frame(rawData, width * height * 3, width, height, AV_PIX_FMT_RGB24);
            
        // Кодируем кадр (энкодер сам реинициализируется при смене разрешения)
        if (!pipeline.encoder->encodeFrame(frame)) {
            return {};
        }

        // Обновляем текущие размеры из энкодера
        pipeline.width = pipeline.encoder->getWidth();
        pipeline.height = pipeline.encoder->getHeight();
             
        return pipeline.lastEncodedFrame;
    }

    std::vector<unsigned char> MediaProcessingService::decodeVideoFrame(MediaType type, const unsigned char* h264Data, int dataSize)
    {
        auto it = m_videoPipelines.find(type);
        if (it == m_videoPipelines.end() || !it->second.initialized || !it->second.decoder) {
            return {};
        }

        auto& pipeline = it->second;
            
        pipeline.lastDecodedFrame.clear();
            
        pipeline.decoder->setDecodedFrameCallback([&pipeline](const Frame& frame) {
            if (!frame.isValid()) return;
            pipeline.width = frame.width;
            pipeline.height = frame.height;
            const int bytesPerPixel = 3;
            const int rowBytes = frame.width * bytesPerPixel;
            const int srcStride = (frame.linesize > 0) ? frame.linesize : rowBytes;
            if (srcStride == rowBytes) {
                pipeline.lastDecodedFrame.assign(frame.data, frame.data + frame.size);
                return;
            }
            pipeline.lastDecodedFrame.resize(static_cast<size_t>(frame.height) * rowBytes);
            unsigned char* dst = pipeline.lastDecodedFrame.data();
            for (int y = 0; y < frame.height; ++y) {
                memcpy(dst, frame.data + static_cast<size_t>(y) * srcStride, rowBytes);
                dst += rowBytes;
            }
        });
            
        if (!pipeline.decoder->decodePacket(h264Data, dataSize, 0)) {
            return {};
        }
            
        return pipeline.lastDecodedFrame;
    }

    std::vector<unsigned char> MediaProcessingService::encryptData(const unsigned char* data, int size, const std::vector<unsigned char>& key)
    {
        if (key.empty() || !data || size <= 0) {
            return {};
        }
            
        std::lock_guard<std::mutex> lock(m_encryptionMutex);
            
        if (!m_encryptionService) {
            m_encryptionService = std::make_unique<MediaEncryptionService>();
        }
            
        CryptoPP::SecByteBlock encryptionKey(key.data(), key.size());
        return m_encryptionService->encryptMedia(data, size, encryptionKey);
    }

    std::vector<unsigned char> MediaProcessingService::decryptData(const unsigned char* encryptedData, int size, const CryptoPP::SecByteBlock& key)
    {
        if (key.size() == 0 || !encryptedData || size <= 0) {
            return {};
        }
            
        std::lock_guard<std::mutex> lock(m_encryptionMutex);
            
        if (!m_encryptionService) {
            m_encryptionService = std::make_unique<MediaEncryptionService>();
        }
            
        return m_encryptionService->decryptMedia(encryptedData, size, key);
    }
}
