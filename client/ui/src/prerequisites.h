#pragma once

#include <QFile>
#include <QJsonDocument>
#include <QSharedMemory>
#include <QJsonObject>

#include "utilities/logger.h"


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
