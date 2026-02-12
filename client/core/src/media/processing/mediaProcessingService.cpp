#include "mediaProcessingService.h"
#include "media/processing/encode/h264Encoder.h"
#include "media/processing/decode/h264Decoder.h"
#include "media/processing/encryption/mediaEncryptionService.h"
#include <libavutil/pixfmt.h>
#include <stdexcept>
#include <opus.h>

namespace core::media
{
    struct MediaProcessingService::Impl {
        // Аудио компоненты (Opus)
        OpusEncoder* audioEncoder = nullptr;
        OpusDecoder* audioDecoder = nullptr;
        bool audioEncoderInitialized = false;
        bool audioDecoderInitialized = false;
        mutable std::mutex audioEncoderMutex;
        mutable std::mutex audioDecoderMutex;
            
        // Видео компоненты (существующие классы)
        std::unique_ptr<H264Encoder> videoEncoder;
        std::unique_ptr<H264Decoder> videoDecoder;
        std::vector<unsigned char> lastEncodedVideoFrame;
        std::vector<unsigned char> lastDecodedVideoFrame;
            
        // Компоненты шифрования
        std::unique_ptr<MediaEncryptionService> encryptionService;
        mutable std::mutex encryptionMutex;
            
        ~Impl() {
            cleanupAll();
        }
            
        void cleanupAudio() {
            std::lock_guard<std::mutex> lock1(audioEncoderMutex);
            std::lock_guard<std::mutex> lock2(audioDecoderMutex);
                
            if (audioEncoder) {
                opus_encoder_destroy(audioEncoder);
                audioEncoder = nullptr;
            }
                
            if (audioDecoder) {
                opus_decoder_destroy(audioDecoder);
                audioDecoder = nullptr;
            }
                
            audioEncoderInitialized = false;
            audioDecoderInitialized = false;
        }
            
        void cleanupVideo() {
            if (videoEncoder) {
                videoEncoder->cleanup();
                videoEncoder.reset();
            }
                
            if (videoDecoder) {
                videoDecoder->cleanup();
                videoDecoder.reset();
            }
                
            lastEncodedVideoFrame.clear();
            lastDecodedVideoFrame.clear();
        }
            
        void cleanupEncryption() {
            std::lock_guard<std::mutex> lock(encryptionMutex);
            encryptionService.reset();
        }
            
        void cleanupAll() {
            cleanupAudio();
            cleanupVideo();
            cleanupEncryption();
        }
    };

    MediaProcessingService::MediaProcessingService()
        : pImpl(std::make_unique<Impl>()), 
            m_sampleRate(0), m_channels(0), m_frameSize(0), m_audioInitialized(false),
            m_width(0), m_height(0), m_videoInitialized(false)
    {
    }

    MediaProcessingService::~MediaProcessingService() = default;

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

    bool MediaProcessingService::initializeVideoProcessing(int width, int height, int bitrate)
    {
        m_width = width;
        m_height = height;
            
        // Создаем и инициализируем видео энкодер
        pImpl->videoEncoder = std::make_unique<H264Encoder>();
        if (!pImpl->videoEncoder->initialize(width, height, 30, bitrate)) {
            pImpl->videoEncoder.reset();
            return false;
        }
            
        // Создаем и инициализируем видео декодер
        pImpl->videoDecoder = std::make_unique<H264Decoder>();
        if (!pImpl->videoDecoder->initialize()) {
            pImpl->videoEncoder.reset();
            pImpl->videoDecoder.reset();
            return false;
        }
            
        // Устанавливаем формат вывода декодера
        pImpl->videoDecoder->setOutputFormat(width, height, 0); // RGB24
            
        m_videoInitialized = true;
        return true;
    }

    void MediaProcessingService::cleanupAudio()
    {
        pImpl->cleanupAudio();
        m_audioInitialized = false;
    }

    void MediaProcessingService::cleanupVideo()
    {
        pImpl->cleanupVideo();
        m_videoInitialized = false;
    }

    std::vector<unsigned char> MediaProcessingService::encodeAudioFrame(const float* pcmData, int frameSize)
    {
        if (!m_audioInitialized || !pImpl->audioEncoderInitialized) {
            return {};
        }
            
        std::lock_guard<std::mutex> lock(pImpl->audioEncoderMutex);
            
        const int maxOutputSize = 4000;
        std::vector<unsigned char> encodedData(maxOutputSize);
            
        int encodedBytes = opus_encode_float(pImpl->audioEncoder, pcmData, frameSize, 
                                            encodedData.data(), maxOutputSize);
            
        if (encodedBytes < 0) {
            return {};
        }
            
        encodedData.resize(encodedBytes);
        return encodedData;
    }

    std::vector<float> MediaProcessingService::decodeAudioFrame(const unsigned char* opusData, int dataSize)
    {
        if (!m_audioInitialized || !pImpl->audioDecoderInitialized) {
            return {};
        }
            
        std::lock_guard<std::mutex> lock(pImpl->audioDecoderMutex);
            
        std::vector<float> pcmData(m_frameSize * m_channels);
            
        int decodedSamples = opus_decode_float(pImpl->audioDecoder, opusData, dataSize,
                                                pcmData.data(), m_frameSize, 0);
            
        if (decodedSamples < 0) {
            return {};
        }
            
        pcmData.resize(decodedSamples * m_channels);
        return pcmData;
    }

    bool MediaProcessingService::initializeAudioEncoder(int sampleRate, int channels, int frameSize)
    {
        std::lock_guard<std::mutex> lock(pImpl->audioEncoderMutex);
            
        int error;
        pImpl->audioEncoder = opus_encoder_create(sampleRate, channels, OPUS_APPLICATION_AUDIO, &error);
        if (error != OPUS_OK) {
            return false;
        }
            
        opus_encoder_ctl(pImpl->audioEncoder, OPUS_SET_BITRATE(64000));
        opus_encoder_ctl(pImpl->audioEncoder, OPUS_SET_COMPLEXITY(5));
        opus_encoder_ctl(pImpl->audioEncoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
            
        pImpl->audioEncoderInitialized = true;
        return true;
    }

    bool MediaProcessingService::initializeAudioDecoder(int sampleRate, int channels)
    {
        std::lock_guard<std::mutex> lock(pImpl->audioDecoderMutex);
            
        int error;
        pImpl->audioDecoder = opus_decoder_create(sampleRate, channels, &error);
        if (error != OPUS_OK) {
            return false;
        }
            
        pImpl->audioDecoderInitialized = true;
        return true;
    }

    std::vector<unsigned char> MediaProcessingService::encodeVideoFrame(const unsigned char* rawData, int width, int height)
    {
        if (!m_videoInitialized || !pImpl->videoEncoder) {
            return {};
        }
            
        // Очищаем предыдущий результат
        pImpl->lastEncodedVideoFrame.clear();
            
        // Устанавливаем callback для получения закодированных данных
        pImpl->videoEncoder->setEncodedDataCallback([this](const uint8_t* data, size_t size, int64_t pts) {
            pImpl->lastEncodedVideoFrame.assign(data, data + size);
        });
            
        // Создаем FrameData (AV_PIX_FMT_RGB24 обязателен для sws_scale)
        Frame frame(rawData, width * height * 3, width, height, AV_PIX_FMT_RGB24);
            
        // Кодируем кадр
        if (!pImpl->videoEncoder->encodeFrame(frame)) {
            return {};
        }
            
        return pImpl->lastEncodedVideoFrame;
    }

    std::vector<unsigned char> MediaProcessingService::decodeVideoFrame(const unsigned char* h264Data, int dataSize)
    {
        if (!m_videoInitialized || !pImpl->videoDecoder) {
            return {};
        }
            
        // Очищаем предыдущий результат
        pImpl->lastDecodedVideoFrame.clear();
            
        // Устанавливаем callback для получения декодированных данных
        pImpl->videoDecoder->setDecodedFrameCallback([this](const Frame& frame) {
            if (frame.isValid()) {
                pImpl->lastDecodedVideoFrame.assign(frame.data, frame.data + frame.size);
            }
        });
            
        // Декодируем пакет
        if (!pImpl->videoDecoder->decodePacket(h264Data, dataSize, 0)) {
            return {};
        }
            
        return pImpl->lastDecodedVideoFrame;
    }

    std::vector<unsigned char> MediaProcessingService::encryptData(const unsigned char* data, int size, const std::vector<unsigned char>& key)
    {
        if (key.empty() || !data || size <= 0) {
            return {};
        }
            
        std::lock_guard<std::mutex> lock(pImpl->encryptionMutex);
            
        // Создаем сервис шифрования если нужен
        if (!pImpl->encryptionService) {
            pImpl->encryptionService = std::make_unique<MediaEncryptionService>();
        }
            
        CryptoPP::SecByteBlock encryptionKey(key.data(), key.size());
        return pImpl->encryptionService->encryptMedia(data, size, encryptionKey);
    }

    std::vector<unsigned char> MediaProcessingService::decryptData(const unsigned char* encryptedData, int size, const CryptoPP::SecByteBlock& key)
    {
        if (key.size() == 0 || !encryptedData || size <= 0) {
            return {};
        }
            
        std::lock_guard<std::mutex> lock(pImpl->encryptionMutex);
            
        // Создаем сервис шифрования если нужен
        if (!pImpl->encryptionService) {
            pImpl->encryptionService = std::make_unique<MediaEncryptionService>();
        }
            
        return pImpl->encryptionService->decryptMedia(encryptedData, size, key);
    }
}
