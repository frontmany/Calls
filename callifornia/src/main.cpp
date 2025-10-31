#include <QApplication>
#include "mainWindow.h"

int main(int argc, char* argv[])
{
    // 192.168.1.44 local machine 
    // 192.168.1.48 server internal ip
    // 92.255.165.77 server global ip

    QApplication app(argc, argv);

    app.setStyleSheet(
        "QMainWindow {"
        "    background-color: #34495e;"
        "}"
        "QMainWindow::title {"
        "    background-color: #2c3e50;"
        "    color: white;"
        "    text-align: center;"
        "    padding: 5px;"
        "}"
    );

    MainWindow* mainWindow = new MainWindow(nullptr);
    mainWindow->initializeCallifornia("192.168.1.44", "8081");
    return app.exec();
}