#include <QApplication>
#include "mainWindow.h"

int main(int argc, char* argv[])
{
    // 192.168.1.40 local machine 
    // 192.168.1.48 server internal ip
    // 92.255.165.77 server global ip

    QApplication app(argc, argv);
    MainWindow* mainWindow = new MainWindow(nullptr, "192.168.1.40", "8081");
    mainWindow->showMaximized();
    return app.exec();
}