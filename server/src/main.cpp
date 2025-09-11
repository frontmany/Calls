#include <iostream>

#include "callsServer.h"

int main() {
    CallsServer server;

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}
