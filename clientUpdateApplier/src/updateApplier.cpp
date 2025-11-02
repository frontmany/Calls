#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <fstream>
#include <vector>

#include <QCoreApplication>
#include <QProcess>
#include <QThread>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

bool killProcess(int pid) {
    QProcess process;

#ifdef Q_OS_WIN
    process.start("taskkill", { "/PID", QString::number(pid), "/F" });
#else
    process.start("kill", { QString::number(pid) });
#endif

    if (!process.waitForFinished(5000)) {
        return false;
    }

    return process.exitCode() == 0;
}

void waitForProcessExit(int pid, int maxWaitMs = 5000) {
    for (int i = 0; i < maxWaitMs / 100; ++i) {
        QProcess process;

#ifdef Q_OS_WIN
        process.start("tasklist", { "/FI", QString("PID eq %1").arg(pid) });
#else
        process.start("ps", { "-p", QString::number(pid) });
#endif

        if (process.waitForFinished(1000)) {
            QString output = process.readAllStandardOutput();
            if (!output.contains(QString::number(pid))) {
                break;
            }
        }
        QThread::msleep(100);
    }
}

void copyDirectory(const QDir& source, const QDir& destination) {
    if (!source.exists()) {
        return;
    }

    if (!destination.exists()) {
        QDir().mkpath(destination.path());
    }

    for (const auto& file : source.entryList(QDir::Files)) {
        if (file == "remove.json") {
            continue;
        }

        QString sourceFile = source.filePath(file);
        QString destFile = destination.filePath(file);

        if (QFile::exists(destFile)) {
            QFile::remove(destFile);
        }

        QFile::copy(sourceFile, destFile);
    }

    for (const auto& dir : source.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QDir sourceSubDir(source.filePath(dir));
        QDir destSubDir(destination.filePath(dir));

        if (!destSubDir.exists()) {
            QDir().mkpath(destSubDir.path());
        }

        copyDirectory(sourceSubDir, destSubDir);
    }
}

void processRemoveJson(const QString& removeJsonPath) {
    QFile file(removeJsonPath);
    if (!file.exists()) {
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        std::cerr << "Failed to open remove.json" << std::endl;
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (doc.isNull() || !doc.isObject()) {
        std::cerr << "Invalid JSON in remove.json" << std::endl;
        return;
    }

    QJsonObject jsonObject = doc.object();
    if (jsonObject.contains("files") && jsonObject["files"].isArray()) {
        QJsonArray filesArray = jsonObject["files"].toArray();
        for (const auto& fileValue : filesArray) {
            QString filePath = fileValue.toString();
            QFile fileToRemove(filePath);
            if (fileToRemove.exists()) {
                if (!fileToRemove.remove()) {
                    std::cerr << "Failed to remove: " << filePath.toStdString() << std::endl;
                }
            }

            QDir dirToRemove(filePath);
            if (dirToRemove.exists()) {
                dirToRemove.removeRecursively();
            }
        }
    }
}

bool launchApplication(const QString& appPath) {
    return QProcess::startDetached(appPath);
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <PID> <APP_PATH>" << std::endl;
        return 1;
    }

    int pid = QString(argv[1]).toInt();
    QString appPath = argv[2];

    QDir tempDirectory("update_temp");
    QDir currentDirectory(".");
    QString removeJsonPath = tempDirectory.filePath("remove.json");

    try {
        std::cout << "Stopping process with PID: " << pid << std::endl;
        if (!killProcess(pid)) {
            std::cerr << "Failed to kill process, waiting for exit..." << std::endl;
        }

        waitForProcessExit(pid);

        if (tempDirectory.exists()) {
            std::cout << "Copying files from update_temp to current directory..." << std::endl;
            copyDirectory(tempDirectory, currentDirectory);
        }
        else {
            std::cerr << "Update directory not found: update_temp" << std::endl;
            return 1;
        }

        if (QFile::exists(removeJsonPath)) {
            std::cout << "Processing remove.json..." << std::endl;
            processRemoveJson(removeJsonPath);
        }

        std::cout << "Cleaning up temporary directory..." << std::endl;
        tempDirectory.removeRecursively();

        std::cout << "Launching application: " << appPath.toStdString() << std::endl;
        if (!launchApplication(appPath)) {
            std::cerr << "Failed to launch application: " << appPath.toStdString() << std::endl;
            return 1;
        }

        std::cout << "Update completed successfully!" << std::endl;
        return 0;

    }
    catch (const std::exception& e) {
        std::cerr << "Error during update: " << e.what() << std::endl;
        return 1;
    }
}