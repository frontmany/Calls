#include <QApplication>
#include "mainWindow.h"

int main(int argc, char* argv[])
{
    // 192.168.1.44 local machine 
    // 192.168.1.48 server internal ip
    // 92.255.165.77 server global ip

    const std::string host = "92.255.165.77";

    QApplication app(argc, argv);
    MainWindow* mainWindow = new MainWindow(nullptr);
    mainWindow->connectUpdater(host, "8080");
    mainWindow->connectCallifornia(host, "8081");
    return app.exec();
}