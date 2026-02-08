#pragma once

#include <vector>
#include <functional>
#include <mutex>
#include <memory>
#include <atomic>
#include <queue>
#include <string>
#include <optional>

#include "deviceInfo.h"
#include "audioPacket.h"

#include <portaudio.h>

namespace core
{
    namespace media 
    {
        class AudioEngine {
        public:
            AudioEngine();
            ~AudioEngine();

            bool initialize(int sampleRate, int framesPerBuffer, int inputChannels, int outputChannels);
            void setInputAudioCallback(std::function<void(const float* data, int length)> inputCallback);
            void refreshAudioDevices();
            bool initialized() const;
            bool isStream() const;
            bool startStream();
            bool stopStream();
            void playAudio(const float* data, int length);
            void muteMicrophone(bool isMute);
            void muteSpeaker(bool isMute);
            bool isSpeakerMuted() const;
            bool isMicrophoneMuted() const;
            void setInputVolume(int volume);  
            void setOutputVolume(int volume);
            int getInputVolume() const;
            int getOutputVolume() const;
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
            bool initializeInternal();
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
            std::queue<AudioPacket> m_outputAudioQueue;
            std::mutex m_outputAudioQueueMutex;
            std::mutex m_inputAudioMutex;
            std::function<void(const float* data, int length)> m_inputCallback;

            float m_inputVolume = 1.0f;
            float m_outputVolume = 1.0f;
            mutable std::mutex m_volumeMutex;

            int m_sampleRate = 48000;
            int m_framesPerBuffer = 960;
            int m_inputChannels = 1;
            int m_outputChannels = 1;

            std::optional<int> m_inputDeviceIndex;
            std::optional<int> m_outputDeviceIndex;

            std::vector<float> m_inputBuffer;
        };
    }
}