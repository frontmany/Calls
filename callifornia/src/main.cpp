#include <QApplication>

#include "mainWindow.h"
#include "prerequisites.h"

int main(int argc, char* argv[]) 
{
    QApplication app(argc, argv);
    MainWindow* mainWindow = new MainWindow();
    mainWindow->executePrerequisites();
    mainWindow->init();

    return app.exec();
}