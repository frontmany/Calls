#include <QApplication>
#include "mainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    std::unique_ptr<MainWindow> mainWindow = std::make_unique<MainWindow>();
    mainWindow->resize(800, 600);
    mainWindow->show();         
    app.exec();

    return 0;
}