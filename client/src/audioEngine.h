#pragma once

#include <vector>
#include <functional>
#include <mutex>
#include <memory>
#include <atomic>
#include <queue>

#include "encoder.h"
#include "decoder.h"
#include "audioPacket.h"

#include <portaudio.h>

namespace calls {

class AudioEngine {
public:
    enum InitializationStatus {
        NO_OUTPUT_DEVICE,
        NO_INPUT_DEVICE,
        OTHER_ERROR,
        INITIALIZED
    };

    AudioEngine(int sampleRate, int framesPerBuffer, int inputChannels, int outputChannels, std::function<void(const unsigned char* data, int length)> returnInputEncodedAudioCallback, Encoder::Config encoderConfig = Encoder::Config(), Decoder::Config decoderConfig = Decoder::Config());
    AudioEngine(std::function<void(const unsigned char* data, int length)> encodedInputCallback);
    ~AudioEngine();
    InitializationStatus initialize();
    void refreshAudioDevices();
    bool isInitialized();
    bool isStream();
    bool startStream();
    bool stopStream();
    void playAudio(const unsigned char* data, int length);
    std::string getLastError() const;
    void mute(bool isMute);
    bool isMuted();
    void setInputVolume(int volume);  
    void setOutputVolume(int volume);
    int getInputVolume() const;
    int getOutputVolume() const;

private:
    void processInputAudio(const float* input, unsigned long frameCount);
    void processOutputAudio(float* output, unsigned long frameCount);

    static int paInputAudioCallback(const void* input, void* output, unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags, void* userData);

private:
    PaStream* m_stream = nullptr;
    std::atomic<bool> m_isInitialized = false;
    std::atomic<bool> m_isStream = false;
    std::atomic<bool> m_muted = false;

    PaError m_lastError = paNoError;
    std::unique_ptr<Encoder> m_encoder;
    std::unique_ptr<Decoder> m_decoder;
    std::queue<AudioPacket> m_outputAudioQueue;
    std::mutex m_outputAudioQueueMutex;
    std::mutex m_inputAudioMutex;
    std::function<void(const unsigned char* data, int length)> m_encodedInputCallback;

    float m_inputVolume = 0.5f;
    float m_outputVolume = 0.5f;
    mutable std::mutex m_volumeMutex;

    int m_sampleRate = 48000;
    int m_framesPerBuffer = 960;
    int m_inputChannels = 1;
    int m_outputChannels = 1;

    std::vector<unsigned char> m_encodedInputBuffer;
    std::vector<float> m_decodedOutputBuffer;
};

}