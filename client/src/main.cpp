#include "audioEngine.h"
#include <iostream>

int main() {
    std::pair<int, int> keys = crypto::generateKeys();

    std::cout << "Create nickname:\n";

    std::string nickname;
    std::cin >> nickname;

    CallsClient client;

    while (true) {
        bool authorized = client.authorize(keys.first, nickname);
        if (!authorized) {
            std::cout << "This nickname is already taken\n";
            std::cout << "Try a different nickname:\n";
            std::cin >> nickname;
        }
    }


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