#pragma once

#include <vector>
#include <functional>
#include <mutex>
#include <memory>
#include <atomic>
#include <queue>
#include <string>
#include <optional>

#include "../processing/encode/opusEncoder.h"
#include "../processing/decode/opusDecoder.h"
#include "deviceInfo.h"
#include "audioPacket.h"

#include <portaudio.h>

namespace core
{
    namespace media 
    {
        class AudioEngine {
        public:
            AudioEngine(int sampleRate, int framesPerBuffer, int inputChannels, int outputChannels, std::function<void(const unsigned char* data, int length)> returnInputEncodedAudioCallback, OpusEncoder::Config encoderConfig = OpusEncoder::Config(), OpusDecoder::Config decoderConfig = OpusDecoder::Config());
            AudioEngine();
            ~AudioEngine();

            bool initialize(std::function<void(const unsigned char* data, int length)> encodedInputCallback);
            void refreshAudioDevices();
            bool initialized() const;
            bool isStream() const;
            bool startStream();
            bool stopStream();
            void playAudio(const unsigned char* data, int length);
            void muteMicrophone(bool isMute);
            void muteSpeaker(bool isMute);
            bool isSpeakerMuted() const;
            bool isMicrophoneMuted() const;
            void setInputVolume(int volume);  
            void setOutputVolume(int volume);
            int getInputVolume() const;
            int getOutputVolume() const;

            // Device selection API
            static int getDeviceCount();
            static std::optional<DeviceInfo> getDeviceInfo(int deviceIndex);
            static std::vector<DeviceInfo> getInputDevices();
            static std::vector<DeviceInfo> getOutputDevices();
            static std::vector<DeviceInfo> getAllDevices();
            static int getDefaultInputDeviceIndex();
            static int getDefaultOutputDeviceIndex();
            
            bool isFormatSupported(int inputDevice, int outputDevice, double sampleRate, 
                                   int inputChannels, int outputChannels) const;
            
            bool setInputDevice(int deviceIndex);
            bool setOutputDevice(int deviceIndex);
            int getCurrentInputDevice() const;
            int getCurrentOutputDevice() const;

        private:
            bool init();
            float softClip(float x);
            void processInputAudio(const float* input, unsigned long frameCount);
            void processOutputAudio(float* output, unsigned long frameCount);

            static int paInputAudioCallback(const void* input, void* output, unsigned long frameCount,
                const PaStreamCallbackTimeInfo* timeInfo,
                PaStreamCallbackFlags statusFlags, void* userData);

        private:
            PaStream* m_stream = nullptr;
            std::atomic<bool> m_isInitialized = false;
            std::atomic<bool> m_isStream = false;
            std::atomic<bool> m_microphoneMuted = false;
            std::atomic<bool> m_speakerMuted = false;

            PaError m_lastError = paNoError;
            std::unique_ptr<OpusEncoder> m_encoder;
            std::unique_ptr<OpusDecoder> m_decoder;
            std::queue<AudioPacket> m_outputAudioQueue;
            std::mutex m_outputAudioQueueMutex;
            std::mutex m_inputAudioMutex;
            std::function<void(const unsigned char* data, int length)> m_encodedInputCallback;

            float m_inputVolume = 1.0f;
            float m_outputVolume = 1.0f;
            mutable std::mutex m_volumeMutex;

            int m_sampleRate = 48000;
            int m_framesPerBuffer = 960;
            int m_inputChannels = 1;
            int m_outputChannels = 1;

            std::optional<int> m_inputDeviceIndex;
            std::optional<int> m_outputDeviceIndex;

            std::vector<unsigned char> m_encodedInputBuffer;
            std::vector<float> m_decodedOutputBuffer;
        };
    }
}