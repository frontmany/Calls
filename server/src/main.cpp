#include <iostream>

#include "callsServer.h"

int main() {
    CallsServer server;

    while (true) {
        std::this_thread::yield();
    }
}
