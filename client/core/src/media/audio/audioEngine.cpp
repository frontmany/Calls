#include "audioEngine.h"
#include <iostream>
#include <cmath>
#include <cstring>
#include <string>

namespace core 
{
    namespace media 
    {
        AudioEngine::AudioEngine()
        {
            m_inputBuffer.resize(m_framesPerBuffer * m_inputChannels);
        }

        AudioEngine::~AudioEngine() {
            stopStream();
            if (m_stream) {
                Pa_CloseStream(m_stream);
                m_stream = nullptr;
            }
            if (m_isInitialized) {
                Pa_Terminate();
                m_isInitialized = false;
            }
        }

        bool AudioEngine::initializeInternal() {
            m_lastError = Pa_Initialize();
            if (m_lastError != paNoError) {
                return false;
            }

            PaStreamParameters inputParameters, outputParameters;
            memset(&inputParameters, 0, sizeof(inputParameters));
            memset(&outputParameters, 0, sizeof(outputParameters));

            if (m_inputChannels > 0) {
                inputParameters.device = m_inputDeviceIndex.has_value() ? m_inputDeviceIndex.value() : Pa_GetDefaultInputDevice();
                if (inputParameters.device == paNoDevice) {
                    Pa_Terminate();
                    return false;
                }

                inputParameters.channelCount = m_inputChannels;
                inputParameters.sampleFormat = paFloat32;
                inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
                inputParameters.hostApiSpecificStreamInfo = nullptr;
            }

            if (m_outputChannels > 0) {
                outputParameters.device = m_outputDeviceIndex.has_value() ? m_outputDeviceIndex.value() : Pa_GetDefaultOutputDevice();
                if (outputParameters.device == paNoDevice) {
                    Pa_Terminate();
                    return false;
                }

                outputParameters.channelCount = m_outputChannels;
                outputParameters.sampleFormat = paFloat32;
                outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
                outputParameters.hostApiSpecificStreamInfo = nullptr;
            }

            m_lastError = Pa_OpenStream(&m_stream,
                (m_inputChannels > 0) ? &inputParameters : nullptr,
                (m_outputChannels > 0) ? &outputParameters : nullptr,
                m_sampleRate, m_framesPerBuffer,
                paNoFlag,
                &AudioEngine::paInputAudioCallback, this);

            if (m_lastError != paNoError) {
                Pa_Terminate();
                return false;
            }

            m_isInitialized = true;
            return true;
        }

        bool AudioEngine::initialize(int sampleRate, int framesPerBuffer, int inputChannels, int outputChannels) {
            m_sampleRate = sampleRate;
            m_framesPerBuffer = framesPerBuffer;
            m_inputChannels = inputChannels;
            m_outputChannels = outputChannels;
            
            m_inputBuffer.resize(m_framesPerBuffer * m_inputChannels);
            
            return initializeInternal();
        }

        void AudioEngine::setInputAudioCallback(std::function<void(const float* data, int length)> inputCallback) {
            m_inputCallback = inputCallback;
        }

        void AudioEngine::refreshAudioDevices() {
            if (!m_isInitialized) return;

            bool wasStreaming = m_isStream;
            if (wasStreaming) {
                stopStream();
            }

            {
                std::lock_guard<std::mutex> lock(m_outputAudioQueueMutex);
                std::queue<AudioPacket> emptyQueue;
                std::swap(m_outputAudioQueue, emptyQueue);
            }

            if (m_isInitialized) {
                Pa_Terminate();
                m_isInitialized = false;
            }

            initializeInternal();

            if (wasStreaming) {
                startStream();
            }
        }


        void AudioEngine::setInputVolume(int volume) {
            std::lock_guard<std::mutex> lock(m_volumeMutex);
            volume = std::max(0, std::min(200, volume));
            m_inputVolume = static_cast<float>(volume) / 100.0f;
        }

        void AudioEngine::muteMicrophone(bool isMute) {
            m_microphoneMuted = isMute;
        }

        void AudioEngine::muteSpeaker(bool isMute) {
            m_speakerMuted = isMute;
        }

        bool AudioEngine::isMicrophoneMuted() const {
            return  m_microphoneMuted;
        }

        bool AudioEngine::isSpeakerMuted() const {
            return m_speakerMuted;
        }

        void AudioEngine::setOutputVolume(int volume) {
            std::lock_guard<std::mutex> lock(m_volumeMutex);
            volume = std::max(0, std::min(200, volume));
            m_outputVolume = static_cast<float>(volume) / 100.0f;
        }

        int AudioEngine::getInputVolume() const {
            std::lock_guard<std::mutex> lock(m_volumeMutex);
            return static_cast<int>(m_inputVolume * 100.0f);
        }

        int AudioEngine::getOutputVolume() const {
            std::lock_guard<std::mutex> lock(m_volumeMutex);
            return static_cast<int>(m_outputVolume * 100.0f);
        }

        void AudioEngine::playAudio(const float* data, int length) {
            if (!m_isInitialized || !m_isStream || !data || length <= 0) return;

            AudioPacket packet;
            packet.audioData.assign(data, data + length);
            packet.samples = length;

            std::lock_guard<std::mutex> lock(m_outputAudioQueueMutex);
            m_outputAudioQueue.push(std::move(packet));
        }

        float AudioEngine::softClip(float x) {
            return std::tanh(x);
        }

        void AudioEngine::processInputAudio(const float* input, unsigned long frameCount) {
            std::lock_guard<std::mutex> lock(m_inputAudioMutex);
            std::lock_guard<std::mutex> volumeLock(m_volumeMutex);

            if (input && m_inputCallback) {
                if (m_inputVolume != 1.0f) {
                    std::vector<float> adjustedInput(frameCount * m_inputChannels);
                    for (unsigned long i = 0; i < frameCount * m_inputChannels; ++i) {
                        adjustedInput[i] = softClip(input[i] * m_inputVolume);
                    }
                    m_inputCallback(adjustedInput.data(), static_cast<int>(adjustedInput.size()));
                }
                else {
                    m_inputCallback(input, static_cast<int>(frameCount * m_inputChannels));
                }
            }
        }

        void AudioEngine::processOutputAudio(float* output, unsigned long frameCount) {
            std::lock_guard<std::mutex> volumeLock(m_volumeMutex);

            std::fill_n(output, frameCount * m_outputChannels, 0.0f);

            if (output) {
                std::lock_guard<std::mutex> lock(m_outputAudioQueueMutex);

                if (!m_outputAudioQueue.empty()) {
                    const auto& currentPacket = m_outputAudioQueue.front();

                    if (!m_speakerMuted) {
                        if (size_t samplesToCopy = currentPacket.audioData.size(); samplesToCopy > 0) {
                            if (m_outputVolume != 1.0f) {
                                for (size_t i = 0; i < samplesToCopy; ++i) {
                                    output[i] = softClip(currentPacket.audioData[i] * m_outputVolume);
                                }
                            }
                            else {
                                for (size_t i = 0; i < samplesToCopy; ++i) {
                                    output[i] = currentPacket.audioData[i];
                                }
                            }
                        }
                    }

                    m_outputAudioQueue.pop();
                }
            }
        }

        bool AudioEngine::startAudioCapture() {
            std::lock_guard<std::mutex> lock(m_inputAudioMutex);

            if (!m_isInitialized) {
                if (!initializeInternal()) {
                    return false;
                }
            }

            if (m_stream == nullptr) {
                return false;
            }

            m_lastError = Pa_StartStream(m_stream);
            if (m_lastError != paNoError) {
                Pa_CloseStream(m_stream);
                m_stream = nullptr;

                if (!initializeInternal()) {
                    m_lastError = Pa_StartStream(m_stream);
                    if (m_lastError != paNoError) {
                        return false;
                    }
                }
                else {
                    return false;
                }
            }

            m_isStream = true;

            return true;
        }

        bool AudioEngine::stopAudioCapture() {
            std::lock_guard<std::mutex> lock(m_inputAudioMutex);

            if (!m_isInitialized || !m_isStream) return true;

            m_lastError = Pa_StopStream(m_stream);
            if (m_lastError != paNoError) {
                m_lastError = Pa_AbortStream(m_stream);
            }

            m_isStream = false;
            return m_lastError == paNoError;
        }


        int AudioEngine::paInputAudioCallback(const void* input, void* output, unsigned long frameCount,
            const PaStreamCallbackTimeInfo* timeInfo,
            PaStreamCallbackFlags statusFlags, void* userData)
        {
            AudioEngine* engine = reinterpret_cast<AudioEngine*>(userData);

            if (!engine) {
                if (output) {
                    float* out = static_cast<float*>(output);
                    std::fill(out, out + frameCount, 0.0f);
                }
                return paContinue;
            }

            if (input && !engine->m_microphoneMuted) {
                const float* in = static_cast<const float*>(input);
                engine->processInputAudio(in, frameCount);
            }

            if (output) {
                float* out = static_cast<float*>(output);
                engine->processOutputAudio(out, frameCount);
            }

            return paContinue;
        }

        bool AudioEngine::initialized() const {
            return m_isInitialized;
        }

        bool AudioEngine::isStream() const {
            return  m_isStream;
        }

        int AudioEngine::getDeviceCount() {
            bool needsInit = (Pa_GetDeviceCount() < 0);
            if (needsInit) {
                PaError err = Pa_Initialize();
                if (err != paNoError) {
                    return 0;
                }
            }

            int count = Pa_GetDeviceCount();
            int result = (count < 0) ? 0 : count;

            if (needsInit) {
                Pa_Terminate();
            }

            return result;
        }

        std::optional<DeviceInfo> AudioEngine::getDeviceInfo(int deviceIndex) {
            bool needsInit = (Pa_GetDeviceCount() < 0);
            if (needsInit) {
                PaError err = Pa_Initialize();
                if (err != paNoError) {
                    return std::nullopt;
                }
            }

            int deviceCount = Pa_GetDeviceCount();
            if (deviceCount < 0 || deviceIndex < 0 || deviceIndex >= deviceCount) {
                if (needsInit) {
                    Pa_Terminate();
                }
                return std::nullopt;
            }

            const PaDeviceInfo* paDeviceInfo = Pa_GetDeviceInfo(deviceIndex);
            if (!paDeviceInfo) {
                if (needsInit) {
                    Pa_Terminate();
                }
                return std::nullopt;
            }

            DeviceInfo info;
            info.deviceIndex = deviceIndex;
            info.name = paDeviceInfo->name ? paDeviceInfo->name : "";
            info.maxInputChannels = paDeviceInfo->maxInputChannels;
            info.maxOutputChannels = paDeviceInfo->maxOutputChannels;
            info.defaultLowInputLatency = paDeviceInfo->defaultLowInputLatency;
            info.defaultLowOutputLatency = paDeviceInfo->defaultLowOutputLatency;
            info.defaultHighInputLatency = paDeviceInfo->defaultHighInputLatency;
            info.defaultHighOutputLatency = paDeviceInfo->defaultHighOutputLatency;
            info.defaultSampleRate = paDeviceInfo->defaultSampleRate;
            info.isDefaultInput = (deviceIndex == Pa_GetDefaultInputDevice());
            info.isDefaultOutput = (deviceIndex == Pa_GetDefaultOutputDevice());

            if (needsInit) {
                Pa_Terminate();
            }

            return info;
        }

        std::vector<DeviceInfo> AudioEngine::getInputDevices() {
            std::vector<DeviceInfo> devices;
            int deviceCount = getDeviceCount();

            for (int i = 0; i < deviceCount; ++i) {
                auto info = getDeviceInfo(i);
                if (info.has_value() && info->maxInputChannels > 0) {
                    devices.push_back(info.value());
                }
            }

            return devices;
        }

        std::vector<DeviceInfo> AudioEngine::getOutputDevices() {
            std::vector<DeviceInfo> devices;
            int deviceCount = getDeviceCount();

            for (int i = 0; i < deviceCount; ++i) {
                auto info = getDeviceInfo(i);
                if (info.has_value() && info->maxOutputChannels > 0) {
                    devices.push_back(info.value());
                }
            }

            return devices;
        }

        std::vector<DeviceInfo> AudioEngine::getAllDevices() {
            std::vector<DeviceInfo> devices;
            int deviceCount = getDeviceCount();

            for (int i = 0; i < deviceCount; ++i) {
                auto info = getDeviceInfo(i);
                if (info.has_value()) {
                    devices.push_back(info.value());
                }
            }

            return devices;
        }

        int AudioEngine::getDefaultInputDeviceIndex() {
            bool needsInit = (Pa_GetDeviceCount() < 0);
            if (needsInit) {
                PaError err = Pa_Initialize();
                if (err != paNoError) {
                    return -1;
                }
            }

            PaDeviceIndex device = Pa_GetDefaultInputDevice();
            int result = (device == paNoDevice) ? -1 : static_cast<int>(device);

            if (needsInit) {
                Pa_Terminate();
            }

            return result;
        }

        int AudioEngine::getDefaultOutputDeviceIndex() {
            bool needsInit = (Pa_GetDeviceCount() < 0);
            if (needsInit) {
                PaError err = Pa_Initialize();
                if (err != paNoError) {
                    return -1;
                }
            }

            PaDeviceIndex device = Pa_GetDefaultOutputDevice();
            int result = (device == paNoDevice) ? -1 : static_cast<int>(device);

            if (needsInit) {
                Pa_Terminate();
            }

            return result;
        }

        bool AudioEngine::isFormatSupported(int inputDevice, int outputDevice, double sampleRate,
                                           int inputChannels, int outputChannels) const {
            if (!m_isInitialized) {
                PaError err = Pa_Initialize();
                if (err != paNoError) {
                    return false;
                }
            }

            PaStreamParameters inputParams = {};
            PaStreamParameters outputParams = {};
            const PaStreamParameters* inputParamsPtr = nullptr;
            const PaStreamParameters* outputParamsPtr = nullptr;

            if (inputChannels > 0 && inputDevice >= 0) {
                const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(inputDevice);
                if (!deviceInfo || deviceInfo->maxInputChannels < inputChannels) {
                    if (!m_isInitialized) {
                        Pa_Terminate();
                    }
                    return false;
                }

                inputParams.device = inputDevice;
                inputParams.channelCount = inputChannels;
                inputParams.sampleFormat = paFloat32;
                inputParams.suggestedLatency = deviceInfo->defaultLowInputLatency;
                inputParams.hostApiSpecificStreamInfo = nullptr;
                inputParamsPtr = &inputParams;
            }

            if (outputChannels > 0 && outputDevice >= 0) {
                const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(outputDevice);
                if (!deviceInfo || deviceInfo->maxOutputChannels < outputChannels) {
                    if (!m_isInitialized) {
                        Pa_Terminate();
                    }
                    return false;
                }

                outputParams.device = outputDevice;
                outputParams.channelCount = outputChannels;
                outputParams.sampleFormat = paFloat32;
                outputParams.suggestedLatency = deviceInfo->defaultLowOutputLatency;
                outputParams.hostApiSpecificStreamInfo = nullptr;
                outputParamsPtr = &outputParams;
            }

            PaError err = Pa_IsFormatSupported(inputParamsPtr, outputParamsPtr, sampleRate);

            if (!m_isInitialized) {
                Pa_Terminate();
            }

            return err == paFormatIsSupported;
        }

        bool AudioEngine::setInputDevice(int deviceIndex) {
            bool needsInit = !m_isInitialized;
            if (needsInit) {
                PaError err = Pa_Initialize();
                if (err != paNoError) {
                    return false;
                }
            }

            int deviceCount = Pa_GetDeviceCount();
            if (deviceCount < 0 || deviceIndex < 0 || deviceIndex >= deviceCount) {
                if (needsInit) {
                    Pa_Terminate();
                }
                return false;
            }

            const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceIndex);
            if (!deviceInfo || deviceInfo->maxInputChannels < m_inputChannels) {
                if (needsInit) {
                    Pa_Terminate();
                }
                return false;
            }

            bool wasStreaming = m_isStream;
            if (wasStreaming) {
                stopStream();
            }

            bool wasInitialized = m_isInitialized;
            if (wasInitialized) {
                if (m_stream) {
                    Pa_CloseStream(m_stream);
                    m_stream = nullptr;
                }
                Pa_Terminate();
                m_isInitialized = false;
            }

            m_inputDeviceIndex = deviceIndex;

            if (needsInit) {
                Pa_Terminate();
            }

            if (wasInitialized) {
                if (!initializeInternal()) {
                    m_inputDeviceIndex = std::nullopt;
                    return false;
                }

                if (wasStreaming) {
                    startStream();
                }
            }

            return true;
        }

        bool AudioEngine::setOutputDevice(int deviceIndex) {
            bool needsInit = !m_isInitialized;
            if (needsInit) {
                PaError err = Pa_Initialize();
                if (err != paNoError) {
                    return false;
                }
            }

            int deviceCount = Pa_GetDeviceCount();
            if (deviceCount < 0 || deviceIndex < 0 || deviceIndex >= deviceCount) {
                if (needsInit) {
                    Pa_Terminate();
                }
                return false;
            }

            const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceIndex);
            if (!deviceInfo || deviceInfo->maxOutputChannels < m_outputChannels) {
                if (needsInit) {
                    Pa_Terminate();
                }
                return false;
            }

            bool wasStreaming = m_isStream;
            if (wasStreaming) {
                stopStream();
            }

            bool wasInitialized = m_isInitialized;
            if (wasInitialized) {
                if (m_stream) {
                    Pa_CloseStream(m_stream);
                    m_stream = nullptr;
                }
                Pa_Terminate();
                m_isInitialized = false;
            }

            m_outputDeviceIndex = deviceIndex;

            if (needsInit) {
                Pa_Terminate();
            }

            if (wasInitialized) {
                if (!initializeInternal()) {
                    m_outputDeviceIndex = std::nullopt;
                    return false;
                }

                if (wasStreaming) {
                    startStream();
                }
            }

            return true;
        }

        int AudioEngine::getCurrentInputDevice() const {
            if (m_inputDeviceIndex.has_value()) {
                return m_inputDeviceIndex.value();
            }
        
            if (m_isInitialized) {
                PaDeviceIndex device = Pa_GetDefaultInputDevice();
                return (device == paNoDevice) ? -1 : static_cast<int>(device);
            }
        
            return getDefaultInputDeviceIndex();
        }

        int AudioEngine::getCurrentOutputDevice() const {
            if (m_outputDeviceIndex.has_value()) {
                return m_outputDeviceIndex.value();
            }
        
            if (m_isInitialized) {
                PaDeviceIndex device = Pa_GetDefaultOutputDevice();
                return (device == paNoDevice) ? -1 : static_cast<int>(device);
            }
        
            return getDefaultOutputDeviceIndex();
        }
    }
}