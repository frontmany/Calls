#include "audioEngine.h"
#include <iostream>
#include <cstring>

AudioEngine::AudioEngine(int sampleRate, int framesPerBuffer, int inputChannels, int outputChannels, std::function<void(const unsigned char* data, int length)> encodedInputCallback, Encoder::Config encoderConfig, Decoder::Config decoderConfig)
    : m_sampleRate(sampleRate),
    m_framesPerBuffer(framesPerBuffer),
    m_inputChannels(inputChannels),
    m_outputChannels(outputChannels),
    m_encoder(std::make_unique<Encoder>(encoderConfig)),
    m_decoder(std::make_unique<Decoder>(decoderConfig))
{
    m_encodedInputBuffer.resize(4000);
    m_decodedOutputBuffer.resize(4096);
    m_encodedInputCallback = encodedInputCallback;
}

AudioEngine::AudioEngine(std::function<void(const unsigned char* data, int length)> encodedInputCallback)
    : m_encoder(std::make_unique<Encoder>()),
    m_decoder(std::make_unique<Decoder>()) 
{
    m_encodedInputBuffer.resize(4000);
    m_decodedOutputBuffer.resize(4096);
    m_encodedInputCallback = encodedInputCallback;
}

AudioEngine::~AudioEngine() {
    m_devicesCheckerRunning = false;
    if (m_devicesCheckerThread.joinable()) {
        m_devicesCheckerThread.join();
    }
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

void AudioEngine::startDevicesAvailabilityChecker() {
    m_devicesCheckerRunning = true;
    m_devicesCheckerThread = std::thread([this]() {
        while (m_devicesCheckerRunning) {
            std::this_thread::sleep_for(std::chrono::seconds(2)); 
            bool isInputDevice = true;
            bool isOutputDevice = true;

            int defaultInput = -1;
            if (m_inputChannels > 0) {
                defaultInput = Pa_GetDefaultInputDevice();
                if (defaultInput == paNoDevice) {
                    isInputDevice = false;
                }
            }

            int defaultOutput = -1;
            if (m_outputChannels > 0) {
                defaultOutput = Pa_GetDefaultOutputDevice();
                if (defaultOutput == paNoDevice) {
                    isOutputDevice = false;
                }
            }

            if (m_inputDevice.isDevice && !isInputDevice) {
                m_inputDevice.isDevice = false;
            }
            else if (m_outputDevice.isDevice && !isOutputDevice) {
                m_outputDevice.isDevice = false;
            }
            else if ((!m_outputDevice.isDevice && isOutputDevice) ||
                (!m_inputDevice.isDevice && isInputDevice) ||
                (m_inputDevice.device != defaultInput) ||
                (m_outputDevice.device != defaultOutput)) 
            {
                stopStream();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                startStream();
            }
            else continue;
        }
    });
}

AudioEngine::InitializationStatus AudioEngine::initialize() {
    m_lastError = Pa_Initialize();
    if (m_lastError != paNoError) {
        return OTHER_ERROR;
    }

    PaStreamParameters inputParameters, outputParameters;
    memset(&inputParameters, 0, sizeof(inputParameters));
    memset(&outputParameters, 0, sizeof(outputParameters));

    if (m_inputChannels > 0) {
        inputParameters.device = Pa_GetDefaultInputDevice();
        if (inputParameters.device == paNoDevice) {
            m_inputDevice.isDevice = false;
            Pa_Terminate();
            return NO_INPUT_DEVICE;
        }

        m_inputDevice.isDevice = true;
        m_inputDevice.device = inputParameters.device;

        inputParameters.channelCount = m_inputChannels;
        inputParameters.sampleFormat = paFloat32;
        inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
        inputParameters.hostApiSpecificStreamInfo = nullptr;
    }

    if (m_outputChannels > 0) {
        outputParameters.device = Pa_GetDefaultOutputDevice();
        if (outputParameters.device == paNoDevice) {
            m_outputDevice.isDevice = false;
            Pa_Terminate();
            return NO_OUTPUT_DEVICE;
        }

        m_outputDevice.isDevice = true;
        m_outputDevice.device = inputParameters.device;

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
        return OTHER_ERROR;
    }

    m_isInitialized = true;
    startDevicesAvailabilityChecker();

    return INITIALIZED;
}

void AudioEngine::setInputVolume(int volume) {
    std::lock_guard<std::mutex> lock(m_volumeMutex);
    volume = std::max(0, std::min(100, volume));
    m_inputVolume = static_cast<float>(volume) / 100.0f;
}

void AudioEngine::setOutputVolume(int volume) {
    std::lock_guard<std::mutex> lock(m_volumeMutex);
    volume = std::max(0, std::min(100, volume));
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

void AudioEngine::playAudio(const unsigned char* data, int length) {
    AudioPacket packet;

    if (m_decoder && data && length > 0) {
        int samplesDecoded = m_decoder->decode(data, length, m_decodedOutputBuffer.data(), static_cast<int>(m_decodedOutputBuffer.size()), 0);

        if (samplesDecoded > 0) {
            
            packet.audioData.assign(m_decodedOutputBuffer.data(), m_decodedOutputBuffer.data() + samplesDecoded);
            packet.samples = samplesDecoded;

            std::lock_guard<std::mutex> lock(m_outputAudioQueueMutex);
            m_outputAudioQueue.push(std::move(packet));
        }
    }
}

void AudioEngine::processInputAudio(const float* input, unsigned long frameCount) {
    std::lock_guard<std::mutex> lock(m_inputAudioMutex);
    std::lock_guard<std::mutex> volumeLock(m_volumeMutex);
    
    if (m_encoder && input) {
        std::vector<float> adjustedInput(frameCount * m_inputChannels);
        for (unsigned long i = 0; i < frameCount * m_inputChannels; ++i) {
            adjustedInput[i] = input[i] * m_inputVolume;
        }
        
        int encodedSize = m_encoder->encode(adjustedInput.data(), m_encodedInputBuffer.data(), static_cast<int>(m_encodedInputBuffer.size()));
        if (encodedSize > 0 && m_encodedInputCallback) {
            m_encodedInputCallback(m_encodedInputBuffer.data(), encodedSize);
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

            if (size_t samplesToCopy = currentPacket.audioData.size(); samplesToCopy > 0) {
                for (size_t i = 0; i < samplesToCopy; ++i) {
                    output[i] = currentPacket.audioData[i] * m_outputVolume;
                }
            }

            m_outputAudioQueue.pop();
        }
    }
}

bool AudioEngine::startStream() {
    std::lock_guard<std::mutex> lock(m_inputAudioMutex);

    if (!m_isInitialized) {
        if (initialize() != INITIALIZED) {
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

        if (initialize() == INITIALIZED) {
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

    if (!m_devicesCheckerRunning) {
        startDevicesAvailabilityChecker();
    }

    return true;
}

bool AudioEngine::stopStream() {
    std::lock_guard<std::mutex> lock(m_inputAudioMutex);

    if (!m_isInitialized || !m_isStream) return true;

    m_lastError = Pa_StopStream(m_stream);
    if (m_lastError != paNoError) {
        m_lastError = Pa_AbortStream(m_stream);
    }

    m_isStream = false;
    return m_lastError == paNoError;
}

std::string AudioEngine::getLastError() const {
    return Pa_GetErrorText(m_lastError);
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

    if (input) {
        const float* in = static_cast<const float*>(input);
        engine->processInputAudio(in, frameCount);
    }

    if (output) {
        float* out = static_cast<float*>(output);
        engine->processOutputAudio(out, frameCount);
    }

    return paContinue;
}

bool AudioEngine::isInitialized() {
    return m_isInitialized;
}

bool AudioEngine::isStream() {
    return  m_isStream;
}