#include <QApplication>
#include "mainWindow.h"

int main(int argc, char* argv[])
{
    // 192.168.1.44 local machine 
    // 192.168.1.48 server internal ip
    // 92.255.165.77 server global ip

    QApplication app(argc, argv);
    MainWindow* mainWindow = new MainWindow(nullptr);
    mainWindow->initializeUpdater("192.168.1.48", "8081");
    mainWindow->initializeCallifornia("192.168.1.48", "8081");
    return app.exec();
}