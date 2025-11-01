#include <QApplication>
#include <QTimer>
#include "mainWindow.h"

int main(int argc, char* argv[])
{
    // 192.168.1.44 local machine 
    // 192.168.1.48 server internal ip
    // 92.255.165.77 server global ip

    QApplication app(argc, argv);
    MainWindow* mainWindow = new MainWindow(nullptr);

    mainWindow->init();

    QTimer::singleShot(1000, [mainWindow]() {
        mainWindow->connectCallifornia("92.255.165.77", "8081");
    });

    return app.exec();
}