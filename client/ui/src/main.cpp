#include <QApplication>

#include "mainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // 192.168.1.44 local machine 
    // 192.168.1.48 server internal ip
    // 92.255.165.77 server global ip

    MainWindow* mainWindow = new MainWindow();
    mainWindow->init();

    return app.exec();
}