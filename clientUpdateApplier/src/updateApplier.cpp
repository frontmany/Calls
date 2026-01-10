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
    process.start("taskkill", { "/PID", QString::number(pid), "/F" });

    if (!process.waitForFinished(5000)) {
        return false;
    }

    return process.exitCode() == 0;
}

bool isProcessRunning(int pid) {
    QProcess process;
    process.start("tasklist", { "/FI", QString("PID eq %1").arg(pid) });

    if (!process.waitForFinished(1000)) {
        return true;
    }

    QString output = process.readAllStandardOutput();
    return output.contains(QString::number(pid));
}

void waitForProcessExit(int pid, int maxWaitMs = 15000) {
    for (int i = 0; i < maxWaitMs / 100; ++i) {
        if (!isProcessRunning(pid)) {
            break;
        }
        QThread::msleep(100);
    }
}

bool isAppImage(const QString& appPath) {
    return false;
}

QString getAppImageBasePath(const QString& appPath) {
    return QFileInfo(appPath).absolutePath();
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
        return false;
    }

    return QProcess::startDetached(appPath, QStringList(), appInfo.absolutePath());
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    if (argc < 3) {
        return 1;
    }

    int pid = QString(argv[1]).toInt();
    QString appPath = argv[2];

    QDir tempDirectory("update_temp");
    QDir currentDirectory(".");
    QString removeJsonPath = tempDirectory.filePath("remove.json");

    try {
        if (!killProcess(pid)) {
        }

        waitForProcessExit(pid, 15000);

        if (tempDirectory.exists()) {
            copyDirectory(tempDirectory, currentDirectory);
        }
        else {
            return 1;
        }

        if (QFile::exists(removeJsonPath)) {
            processRemoveJson(removeJsonPath);
        }

        tempDirectory.removeRecursively();

        if (!launchApplication(appPath)) {
            return 1;
        }

        return 0;

    }
    catch (const std::exception& e) {
        return 1;
    }
}