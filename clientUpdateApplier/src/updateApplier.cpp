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
    if (process.waitForFinished(2000)) {
        return process.exitCode() == 0;
    }

    process.start("kill", { "-9", QString::number(pid) });
#endif

    if (!process.waitForFinished(5000)) {
        qWarning() << "Failed to wait for kill process";
        return false;
    }

    return process.exitCode() == 0;
}

bool isProcessRunning(int pid) {
    QProcess process;

#ifdef Q_OS_WIN
    process.start("tasklist", { "/FI", QString("PID eq %1").arg(pid) });
#else
    process.start("ps", { "-p", QString::number(pid) });
#endif

    if (!process.waitForFinished(1000)) {
        return true;
    }

    QString output = process.readAllStandardOutput();
    return output.contains(QString::number(pid));
}

void waitForProcessExit(int pid, int maxWaitMs = 15000) {
    for (int i = 0; i < maxWaitMs / 100; ++i) {
        if (!isProcessRunning(pid)) {
            qDebug() << "Process" << pid << "terminated successfully";
            break;
        }
        QThread::msleep(100);
    }
}

bool isAppImage(const QString& appPath) {
    return appPath.endsWith(".AppImage") || appPath.contains("/tmp/.mount_");
}

QString getAppImageBasePath(const QString& appPath) {
    QFileInfo appInfo(appPath);
    return appInfo.absolutePath();
}

void copyDirectory(const QDir& source, const QDir& destination) {
    if (!source.exists()) {
        qWarning() << "Source directory does not exist:" << source.path();
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

        if (QFile::copy(sourceFile, destFile)) {
            QFile::setPermissions(destFile,
                QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                QFile::ReadUser | QFile::WriteUser | QFile::ExeUser |
                QFile::ReadGroup | QFile::ExeGroup |
                QFile::ReadOther | QFile::ExeOther);
        }
    }

    for (const auto& dir : source.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QDir sourceSubDir(source.filePath(dir));
        QDir destSubDir(destination.filePath(dir));
        copyDirectory(sourceSubDir, destSubDir);
    }
}

void processRemoveJson(const QString& removeJsonPath) {
    QFile file(removeJsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (doc.isNull() || !doc.isObject()) {
        return;
    }

    QJsonObject jsonObject = doc.object();
    if (jsonObject.contains("files") && jsonObject["files"].isArray()) {
        QJsonArray filesArray = jsonObject["files"].toArray();
        for (const auto& fileValue : filesArray) {
            QString filePath = fileValue.toString();
            QFile fileToRemove(filePath);
            if (fileToRemove.exists()) {
                fileToRemove.remove();
            }
        }
    }
}

bool launchApplication(const QString& appPath) {
    QFileInfo appInfo(appPath);

    if (!appInfo.exists()) {
        qWarning() << "Application not found:" << appPath;
        return false;
    }

    if (isAppImage(appPath)) {
        QFile::setPermissions(appPath,
            QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
            QFile::ReadUser | QFile::WriteUser | QFile::ExeUser |
            QFile::ReadGroup | QFile::ExeGroup |
            QFile::ReadOther | QFile::ExeOther);
    }

    return QProcess::startDetached(appPath, QStringList(), appInfo.absolutePath());
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    if (argc < 3) {
        qCritical() << "Usage: update_applier <PID> <APP_PATH>";
        return 1;
    }

    int pid = QString(argv[1]).toInt();
    QString appPath = argv[2];

    qDebug() << "Updating application, PID:" << pid << "Path:" << appPath;

    QDir tempDirectory("update_temp");
    QDir currentDirectory(".");
    QString removeJsonPath = tempDirectory.filePath("remove.json");

    try {
        if (isAppImage(appPath)) {
            qDebug() << "Target is AppImage, using extended timeout";
        }

        qInfo() << "Stopping application...";
        if (!killProcess(pid)) {
            qWarning() << "Failed to kill process gracefully";
        }

        waitForProcessExit(pid, isAppImage(appPath) ? 20000 : 15000);

        if (tempDirectory.exists()) {
            qInfo() << "Applying update...";

            if (isAppImage(appPath)) {
                QString appImagePath = appPath;
                if (appPath.contains("/tmp/.mount_")) {
                    QFileInfo originalAppImage(qgetenv("APPIMAGE"));
                    if (originalAppImage.exists()) {
                        appImagePath = originalAppImage.absoluteFilePath();
                    }
                }

                QFileInfoList appImages = tempDirectory.entryInfoList({ "*.AppImage" }, QDir::Files);
                if (!appImages.empty()) {
                    QString newAppImage = appImages.first().absoluteFilePath();
                    QString destAppImage = appImagePath;

                    qDebug() << "Replacing AppImage:" << destAppImage;

                    if (QFile::exists(destAppImage)) {
                        QFile::remove(destAppImage);
                    }

                    if (QFile::copy(newAppImage, destAppImage)) {
                        QFile::setPermissions(destAppImage,
                            QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                            QFile::ReadUser | QFile::WriteUser | QFile::ExeUser);
                        appPath = destAppImage;
                    }
                }
            }

            copyDirectory(tempDirectory, currentDirectory);
        }
        else {
            qCritical() << "Update directory not found";
            return 1;
        }

        if (QFile::exists(removeJsonPath)) {
            processRemoveJson(removeJsonPath);
        }

        tempDirectory.removeRecursively();

        qInfo() << "Launching application...";
        if (!launchApplication(appPath)) {
            qCritical() << "Failed to launch application";
            return 1;
        }

        qInfo() << "Update completed successfully";
        return 0;

    }
    catch (const std::exception& e) {
        qCritical() << "Update error:" << e.what();
        return 1;
    }
}