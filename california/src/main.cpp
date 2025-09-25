#include <QApplication>
#include "mainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    std::unique_ptr<MainWindow> mainWindow = std::make_unique<MainWindow>(nullptr, "192.168.1.40", "8081");
    mainWindow->showMaximized();
    app.exec();

    return 0;
}