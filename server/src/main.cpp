#include <iostream>

#include "callsServer.h"

int main() {
    CallsServer server("8081");
    server.run();
    return 0;
}
