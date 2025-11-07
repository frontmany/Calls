#include <QApplication>
#include <QTimer>

#include <filesystem>

#include "mainWindow.h"
#include "logger.h"

int main(int argc, char* argv[])
{
    std::filesystem::create_directories("logs");
    
    LOG_INFO("=== Callifornia Application Starting ===");
    
    // 192.168.1.44 local machine 
    // 192.168.1.48 server internal ip
    // 92.255.165.77 server global ip

    QApplication app(argc, argv);
    MainWindow* mainWindow = new MainWindow(nullptr);
    mainWindow->init();

    QTimer::singleShot(100, [mainWindow]() {
        mainWindow->connectCallifornia("192.168.1.44", "8081"); 
    });

    LOG_INFO("Entering Qt event loop");
    return app.exec();
}