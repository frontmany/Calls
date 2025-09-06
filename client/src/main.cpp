#include "audioEngine.h"
#include <iostream>

int main() {
    AudioEngine audioEngine([](const unsigned char* data, int length) {
        //std::cout << "Sending " << length << " bytes to network" << std::endl;
        // networkManager.send(data, length);
    });

    audioEngine.initialize();

    if (audioEngine.startStream()) {
        //std::vector<unsigned char> networkData;
        //audioEngine.playAudio(networkData.data(), networkData.size());

        std::cin.get();
        audioEngine.stopStream();
    }

    return 0;
}