#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <mutex>

namespace core
{
    namespace media
    {
        // Унифицированный процессор для H.264 кодирования/декодирование видео, Opus кодирования/декодирование аудио и шифрования
        class MediaProcessingService {
        public:
            MediaProcessingService();
            ~MediaProcessingService();

            // Инициализация процессоров
            bool initializeAudio(int sampleRate = 48000, int channels = 1, int frameSize = 960);
            bool initializeVideo(int width = 1920, int height = 1080, int bitrate = 1800000);
            
            // Очистка ресурсов
            void cleanupAudio();
            void cleanupVideo();

            // Кодирование/декодирование аудио
            std::vector<unsigned char> encodeAudioFrame(const float* pcmData, int frameSize);
            std::vector<float> decodeAudioFrame(const unsigned char* opusData, int dataSize);

            // Кодирование/декодирование видео
            std::vector<unsigned char> encodeVideoFrame(const unsigned char* rawData, int width, int height);
            std::vector<unsigned char> decodeVideoFrame(const unsigned char* h264Data, int dataSize);

            // Шифрование/дешифрование медиа данных
            std::vector<unsigned char> encryptData(const unsigned char* data, int size);
            std::vector<unsigned char> decryptData(const unsigned char* encryptedData, int size);
            
            // Получение информации о процессорах
            bool isAudioInitialized() const { return m_audioInitialized; }
            bool isVideoInitialized() const { return m_videoInitialized; }

            // Аудио параметры
            int getSampleRate() const { return m_sampleRate; }
            int getChannels() const { return m_channels; }
            int getFrameSize() const { return m_frameSize; }

            // Видео параметры
            int getWidth() const { return m_width; }
            int getHeight() const { return m_height; }

            // Управление ключом шифрования
            void setEncryptionKey(const std::vector<unsigned char>& key);
            bool hasEncryptionKey() const;

        private:
            struct Impl;
            std::unique_ptr<Impl> pImpl;

            // Аудио параметры
            int m_sampleRate;
            int m_channels;
            int m_frameSize;
            bool m_audioInitialized;

            // Видео параметры
            int m_width;
            int m_height;
            bool m_videoInitialized;

            // Вспомогательные методы
            bool initializeAudioEncoder(int sampleRate, int channels, int frameSize);
            bool initializeAudioDecoder(int sampleRate, int channels);
            bool initializeVideoEncoder(int width, int height, int bitrate);
            bool initializeVideoDecoder();
        };
    }
}
