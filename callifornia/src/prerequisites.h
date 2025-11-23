#pragma once

#include <QFile>
#include <QJsonDocument>
#include <QSharedMemory>
#include <QJsonObject>

#include "logger.h"

static void makeRemainingReplacements() {
    try {
        if (!QFile::exists("replace.json")) {
            LOG_INFO("replace.json not found in current directory");
            return;
        }

        QFile file("replace.json");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            LOG_ERROR("Failed to open replace.json for reading");
            return;
        }

        QByteArray jsonData = file.readAll();
        file.close();

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            LOG_ERROR("Failed to parse replace.json: {}", parseError.errorString().toStdString());
            return;
        }

        if (!doc.isObject()) {
            LOG_ERROR("replace.json does not contain a JSON object");
            return;
        }

        QJsonObject replacements = doc.object();
        LOG_INFO("Found replace.json with {} file replacements", replacements.size());

        for (auto it = replacements.begin(); it != replacements.end(); ++it) {
            QString oldFilename = it.key();
            QString newFilename = it.value().toString();

            LOG_DEBUG("Processing replacement: '{}' -> '{}'",
                newFilename.toStdString(), oldFilename.toStdString());

            if (!QFile::exists(newFilename)) {
                LOG_WARN("New file '{}' not found, skipping", newFilename.toStdString());
                continue;
            }

            QFile oldFile(oldFilename);
            if (oldFile.exists()) {
                if (!oldFile.remove()) {
                    LOG_ERROR("Failed to remove old file '{}'", oldFilename.toStdString());
                    continue;
                }
                LOG_INFO("Removed old file: {}", oldFilename.toStdString());
            }

            QFile newFile(newFilename);
            if (!newFile.rename(oldFilename)) {
                LOG_ERROR("Failed to rename '{}' to '{}': {}",
                    newFilename.toStdString(), oldFilename.toStdString(),
                    newFile.errorString().toStdString());
                return;
            }

            LOG_INFO("Renamed '{}' to '{}'", newFilename.toStdString(), oldFilename.toStdString());
        }

        LOG_INFO("All file replacements completed successfully");
        return;

    }
    catch (const std::exception& ex) {
        LOG_ERROR("Error applying file replacements: {}", ex.what());
        return;
    }
}

static QString getUpdaterHost(const QString& configPath = "config.json") {
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARN("Failed to open config file: {}", configPath.toStdString());
        return QString();
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_WARN("JSON parse error: {}", parseError.errorString().toStdString());
        return QString();
    }

    if (!doc.isObject()) {
        LOG_WARN("Config file does not contain a JSON object");
        return QString();
    }

    QJsonObject jsonObj = doc.object();
    QString host = jsonObj["updaterHost"].toString();

    if (host.isEmpty()) {
        LOG_WARN("updaterHost not found or empty in config file");
    }
    else {
        LOG_DEBUG("Retrieved updater host: {}", host.toStdString());
    }

    return host;
}

static QString getServerHost(const QString& configPath = "config.json") {
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARN("Failed to open config file: {}", configPath.toStdString());
        return QString();
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_WARN("JSON parse error: {}", parseError.errorString().toStdString());
        return QString();
    }

    if (!doc.isObject()) {
        LOG_WARN("Config file does not contain a JSON object");
        return QString();
    }

    QJsonObject jsonObj = doc.object();
    QString host = jsonObj["serverHost"].toString();

    if (host.isEmpty()) {
        LOG_WARN("serverHost not found or empty in config file");
    }
    else {
        LOG_DEBUG("Retrieved server host: {}", host.toStdString());
    }

    return host;
}

static QString getPort(const QString& configPath = "config.json") {
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARN("Failed to open config file: {}", configPath.toStdString());
        return QString();
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_WARN("JSON parse error: {}", parseError.errorString().toStdString());
        return QString();
    }

    if (!doc.isObject()) {
        LOG_WARN("Config file does not contain a JSON object");
        return QString();
    }

    QJsonObject jsonObj = doc.object();
    QString port = jsonObj["port"].toString();

    if (port.isEmpty()) {
        LOG_WARN("port not found or empty in config file");
    }
    else {
        LOG_DEBUG("Retrieved port: {}", port.toStdString());
    }

    return port;
}

static bool isMultiInstanceAllowed(const QString& configPath = "config.json") {
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARN("Failed to open config file: {}", configPath.toStdString());
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_WARN("JSON parse error in config file: {}", parseError.errorString().toStdString());
        return false;
    }

    if (!doc.isObject()) {
        LOG_WARN("Config file does not contain a JSON object");
        return false;
    }

    QJsonObject jsonObj = doc.object();

    if (!jsonObj.contains("multiInstance")) {
        LOG_DEBUG("multiInstance key not found in config, defaulting to false");
        return false;
    }

    QJsonValue multiInstanceValue = jsonObj["multiInstance"];

    if (multiInstanceValue.isString()) {
        QString value = multiInstanceValue.toString().toLower().trimmed();
        bool result = (value == "1" || value == "true" || value == "yes" || value == "on");
        LOG_DEBUG("multiInstance string value: '{}' -> {}", value.toStdString(), result);
        return result;
    }
    else if (multiInstanceValue.isBool()) {
        bool result = multiInstanceValue.toBool();
        LOG_DEBUG("multiInstance bool value: {}", result);
        return result;
    }
    else if (multiInstanceValue.isDouble()) {
        bool result = (multiInstanceValue.toInt() == 1);
        LOG_DEBUG("multiInstance numeric value: {} -> {}", multiInstanceValue.toInt(), result);
        return result;
    }
    else {
        LOG_WARN("multiInstance has unsupported type, defaulting to false");
        return false;
    }
}

inline static bool isFirstInstance() {
    static QSharedMemory sharedMemory("callifornia");

    if (sharedMemory.attach()) {
        return false;
    }

    if (!sharedMemory.create(1)) {
        return false;
    }

    return true;
}