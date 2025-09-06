#include "AudioEngine.h"
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
    stopStream();
    if (m_stream) {
        Pa_CloseStream(m_stream);
    }
    if (m_isInitialized) {
        Pa_Terminate();
    }
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
            Pa_Terminate();
            return NO_INPUT_DEVICE;
        }
        inputParameters.channelCount = m_inputChannels;
        inputParameters.sampleFormat = paFloat32;
        inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
        inputParameters.hostApiSpecificStreamInfo = nullptr;
    }

    if (m_outputChannels > 0) {
        outputParameters.device = Pa_GetDefaultOutputDevice();
        if (outputParameters.device == paNoDevice) {
            Pa_Terminate();
            return NO_OUTPUT_DEVICE;
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
        return OTHER_ERROR;
    }

    m_isInitialized = true;
    return INITIALIZED;
}

void AudioEngine::playAudio(const unsigned char* data, int length) {
    AudioPacket packet;

    if (m_decoder && data && length > 0) {
        int samplesDecoded = m_decoder->decode(data, length, m_decodedOutputBuffer.data(), m_decodedOutputBuffer.size(), 0);

        if (samplesDecoded > 0) {
            
            packet.audioData.assign(m_decodedOutputBuffer.data(), m_decodedOutputBuffer.data() + samplesDecoded);
            packet.samples = samplesDecoded;

            std::lock_guard<std::mutex> lock(m_outputAudioQueueMutex);
            m_outputAudioQueue.push(std::move(packet));
        }
    }
}

void AudioEngine::processInputAudio(const float* input, unsigned long frameCount) {
    if (m_encoder && input) {
        int encodedSize = m_encoder->encode(input, m_encodedInputBuffer.data(), m_encodedInputBuffer.size());
        if (encodedSize > 0 && m_encodedInputCallback) {
            m_encodedInputCallback(m_encodedInputBuffer.data(), encodedSize);
        }
    }
}

void AudioEngine::processOutputAudio(float* output, unsigned long frameCount) {
    if (output) {
        std::lock_guard<std::mutex> lock(m_outputAudioQueueMutex);

        if (!m_outputAudioQueue.empty()) {
            const auto& currentPacket = m_outputAudioQueue.front();

            if (size_t samplesToCopy = currentPacket.audioData.size(); samplesToCopy > 0) {
                std::copy_n(currentPacket.audioData.data(), samplesToCopy, output);
            }

            m_outputAudioQueue.pop();
        }
    }
}

bool AudioEngine::startStream() {
    if (!m_isInitialized) return false;

    m_lastError = Pa_StartStream(m_stream);
    if (m_lastError != paNoError) {
        return false;
    }

    m_isStream = true;
    return true;
}

bool AudioEngine::stopStream() {
    if (!m_isInitialized || !m_isStream) return true;

    m_lastError = Pa_StopStream(m_stream);
    if (m_lastError != paNoError) {
        m_lastError = Pa_AbortStream(m_stream);
    }

    m_isStream = false;
    m_stream = nullptr;
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