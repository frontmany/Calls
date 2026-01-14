#include <QApplication>
#include <algorithm>
#include <string_view>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <WinSock2.h>
#endif

#include "CrashCatch.hpp"
#include "mainWindow.h"

void initialieCrashDump() {
    CrashCatch::Config crashConfig;
    crashConfig.appVersion = "1.2.0";
    crashConfig.onCrash = [](const CrashCatch::CrashContext& context)
    {
        std::cout << "Crash captured. Dump: " << context.dumpFilePath << " Log: " << context.logFilePath << std::endl;
    };

    CrashCatch::initialize(crashConfig);
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    MainWindow* mainWindow = new MainWindow();
    mainWindow->init();

    // 192.168.1.44 local machine 
    // 192.168.1.48 server internal ip
    // 92.255.165.77 server global ip

    return app.exec();
}