#include <QCoreApplication>
#include <QProcess>
#include <QThread>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>

#include "utilities/logger.h"
#include "utilities/crashCatchInitializer.h"
#include "utilities/constant.h"

#ifdef Q_OS_WIN
#include <windows.h>
#elif defined(Q_OS_LINUX) || defined(Q_OS_MAC)
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#endif

using namespace updateApplier::utilities;
using namespace updateApplier::utilities::log;

bool killProcess(int pid) {
    LOG_INFO("Attempting to kill process with PID: {}", pid);

#ifdef Q_OS_WIN
    HANDLE processHandle = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
    if (processHandle == NULL) {
        DWORD error = GetLastError();
        if (error == ERROR_INVALID_PARAMETER) {
            LOG_WARN("Process with PID {} does not exist", pid);
        } else {
            LOG_ERROR("Failed to open process {}: error code {}", pid, error);
        }
        return false;
    }

    BOOL result = TerminateProcess(processHandle, 0);
    CloseHandle(processHandle);

    if (result) {
        LOG_INFO("Successfully terminated process {}", pid);
        return true;
    } else {
        LOG_ERROR("Failed to terminate process {}: error code {}", pid, GetLastError());
        return false;
    }
#elif defined(Q_OS_LINUX) || defined(Q_OS_MAC)
    if (kill(static_cast<pid_t>(pid), SIGTERM) == 0) {
        LOG_INFO("Successfully sent SIGTERM to process {}", pid);
        // Wait a bit for graceful shutdown
        QThread::msleep(SIGTERM_WAIT_TIME_MS);
        
        // Check if process still exists, if so, force kill
        if (kill(static_cast<pid_t>(pid), 0) == 0) {
            LOG_WARN("Process {} still running after SIGTERM, sending SIGKILL", pid);
            if (kill(static_cast<pid_t>(pid), SIGKILL) == 0) {
                LOG_INFO("Successfully sent SIGKILL to process {}", pid);
                return true;
            } else {
                LOG_ERROR("Failed to send SIGKILL to process {}", pid);
                return false;
            }
        }
        return true;
    } else {
        if (errno == ESRCH) {
            LOG_WARN("Process with PID {} does not exist", pid);
        } else {
            LOG_ERROR("Failed to kill process {}: error {}", pid, errno);
        }
        return false;
    }
#else
    LOG_ERROR("Unsupported platform for process termination");
    return false;
#endif
}

bool copyFiles(const QDir& source, const QDir& destination) {
    if (!source.exists()) {
        LOG_WARN("Source directory does not exist: {}", source.path().toStdString());
        return false;
    }

    if (!destination.exists()) {
        if (!QDir().mkpath(destination.path())) {
            LOG_ERROR("Failed to create destination directory: {}", destination.path().toStdString());
            return false;
        }
        LOG_DEBUG("Created destination directory: {}", destination.path().toStdString());
    }

    bool allSuccess = true;
    for (const auto& file : source.entryList(QDir::Files)) {
        QString sourceFile = source.filePath(file);
        QString destFile = destination.filePath(file);

        if (QFile::exists(destFile)) {
            if (!QFile::remove(destFile)) {
                LOG_ERROR("Failed to remove existing file: {}", destFile.toStdString());
                allSuccess = false;
                continue;
            }
        }

        if (QFile::copy(sourceFile, destFile)) {
            QFile::setPermissions(destFile,
                QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                QFile::ReadUser | QFile::WriteUser | QFile::ExeUser |
                QFile::ReadGroup | QFile::ExeGroup |
                QFile::ReadOther | QFile::ExeOther);
            LOG_DEBUG("Copied file: {} -> {}", sourceFile.toStdString(), destFile.toStdString());
        } else {
            LOG_ERROR("Failed to copy file: {} -> {}", sourceFile.toStdString(), destFile.toStdString());
            allSuccess = false;
        }
    }

    for (const auto& dir : source.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QDir sourceSubDir(source.filePath(dir));
        QDir destSubDir(destination.filePath(dir));
        if (!copyFiles(sourceSubDir, destSubDir)) {
            allSuccess = false;
        }
    }

    return allSuccess;
}

bool deleteFilesListedInJson(const QString& removeJsonPath) {
    try {
        std::string pathStr = removeJsonPath.toStdString();
        LOG_INFO("Processing deletion list from: {}", pathStr);
        
        QFile file(removeJsonPath);
        if (!file.open(QIODevice::ReadOnly)) {
            LOG_ERROR("Failed to open deletion list file: {}", pathStr);
            return false;
        }
        
        QByteArray jsonData = file.readAll();
        file.close();

        if (jsonData.isEmpty()) {
            LOG_WARN("Deletion list file is empty: {}", pathStr);
            return true; // Empty file is not an error, just nothing to delete
        }

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
        
        if (doc.isNull()) {
            LOG_ERROR("Invalid JSON format in deletion list file: {} (parse error: {})", 
                pathStr, parseError.errorString().toStdString());
            return false;
        }

        QJsonArray filesArray;
        
        // Check if it's a direct array
        if (doc.isArray()) {
            filesArray = doc.array();
        }
        // Check if it's an object with a key
        else if (doc.isObject()) {
            QJsonObject jsonObject = doc.object();
            
            // Try "filePaths" key first (old format)
            if (jsonObject.contains(JSON_KEY_FILE_PATHS) && jsonObject[JSON_KEY_FILE_PATHS].isArray()) {
                filesArray = jsonObject[JSON_KEY_FILE_PATHS].toArray();
            }
            // Try "files" key (alternative format)
            else if (jsonObject.contains("files") && jsonObject["files"].isArray()) {
                filesArray = jsonObject["files"].toArray();
            }
            else {
                LOG_WARN("Deletion list file does not contain expected array (tried '{}' or 'files' keys): {}", JSON_KEY_FILE_PATHS, pathStr);
                return false;
            }
        }
        else {
            LOG_ERROR("Invalid JSON format in deletion list file (not array or object): {}", pathStr);
            return false;
        }

        if (filesArray.isEmpty()) {
            return true;
        }

        bool allSuccess = true;
        for (const auto& fileValue : filesArray) {
            if (!fileValue.isString()) {
                LOG_WARN("Skipping non-string value in deletion list");
                continue;
            }
            
            QString filePath = fileValue.toString();
            if (filePath.isEmpty()) {
                LOG_WARN("Skipping empty file path in deletion list");
                continue;
            }

            std::string filePathStr = filePath.toStdString();
            QFile fileToRemove(filePath);
            if (fileToRemove.exists()) {
                if (fileToRemove.remove()) {
                    LOG_DEBUG("Deleted file: {}", filePathStr);
                } else {
                    LOG_ERROR("Failed to delete file: {}", filePathStr);
                    allSuccess = false;
                }
            }
        }

        LOG_INFO("Finished processing deletion list, {} files processed", filesArray.size());
        return allSuccess;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Exception in deleteFilesListedInJson: {}", e.what());
        return false;
    }
    catch (...) {
        LOG_ERROR("Unknown exception in deleteFilesListedInJson");
        return false;
    }
}

bool objectsDiffer(const QJsonObject& obj1, const QJsonObject& obj2, int depth = 0) {
    try {
        // Limit recursion depth to prevent stack overflow
        if (depth > 100) {
            LOG_WARN("objectsDiffer recursion depth limit reached ({}), assuming objects differ", depth);
            return true;
        }
        
        QSet<QString> keys1;
        QSet<QString> keys2;
        
        try {
            QStringList keysList1 = obj1.keys();
            QStringList keysList2 = obj2.keys();
            
            for (const QString& key : keysList1) {
                keys1.insert(key);
            }
            
            for (const QString& key : keysList2) {
                keys2.insert(key);
            }
        }
        catch (const std::exception& e) {
            LOG_ERROR("Exception while creating QSet from keys: {}", e.what());
            return true;
        }
        catch (...) {
            LOG_ERROR("Unknown exception while creating QSet from keys");
            return true;
        }

        if (keys1 != keys2) {
            return true;
        }

        for (const QString& key : keys1) {
            const QJsonValue& val1 = obj1.value(key);
            const QJsonValue& val2 = obj2.value(key);

            if (val1.type() != val2.type()) {
                return true;
            }

            if (val1.isObject() && val2.isObject()) {
                if (objectsDiffer(val1.toObject(), val2.toObject(), depth + 1)) {
                    return true;
                }
            }
            else if (val1.isArray() && val2.isArray()) {
                QJsonArray arr1 = val1.toArray();
                QJsonArray arr2 = val2.toArray();

                if (arr1.size() != arr2.size()) {
                    return true;
                }

                for (int i = 0; i < arr1.size(); ++i) {
                    if (arr1[i].isObject() && arr2[i].isObject()) {
                        if (objectsDiffer(arr1[i].toObject(), arr2[i].toObject(), depth + 1)) {
                            return true;
                        }
                    }
                    else if (arr1[i].isArray() && arr2[i].isArray()) {
                        QJsonObject tempObj1, tempObj2;
                        tempObj1["array"] = arr1[i];
                        tempObj2["array"] = arr2[i];

                        if (objectsDiffer(tempObj1, tempObj2, depth + 1)) {
                            return true;
                        }
                    }
                    else if (arr1[i] != arr2[i]) {
                        return true;
                    }
                }
            }
            else if (val1 != val2) {
                return true;
            }
        }

        return false;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Exception in objectsDiffer: {}", e.what());
        return true; // Return true on error to be safe
    }
    catch (...) {
        LOG_ERROR("Unknown exception in objectsDiffer");
        return true; // Return true on error to be safe
    }
}

bool configsDiffer(const QString& appConfigPath, const QString& newConfigPath) {
    try {
        QFile appConfigFile(appConfigPath);
        QFile newConfigFile(newConfigPath);

        if (!appConfigFile.open(QIODevice::ReadOnly) || !newConfigFile.open(QIODevice::ReadOnly)) {
            LOG_WARN("Failed to open config files for comparison");
            return true;
        }

        QByteArray appConfigData = appConfigFile.readAll();
        QByteArray newConfigData = newConfigFile.readAll();

        appConfigFile.close();
        newConfigFile.close();

        QJsonDocument appDoc = QJsonDocument::fromJson(appConfigData);
        QJsonDocument newDoc = QJsonDocument::fromJson(newConfigData);

        if (appDoc.isNull() || !appDoc.isObject() || newDoc.isNull() || !newDoc.isObject()) {
            LOG_WARN("Invalid JSON in config files");
            return true;
        }

        QJsonObject appConfig = appDoc.object();
        QJsonObject newConfig = newDoc.object();

        if (appConfig.keys().size() != newConfig.keys().size()) {
            return true;
        }
        
        return objectsDiffer(appConfig, newConfig);
    }
    catch (const std::exception& e) {
        LOG_ERROR("Exception in configsDiffer: {}", e.what());
        return true; // Return true on error to be safe
    }
    catch (...) {
        LOG_ERROR("Unknown exception in configsDiffer");
        return true; // Return true on error to be safe
    }
}

QJsonObject mergeConfigs(const QString& applicationConfigFilePath, const QString& loadedConfigFilePath) {
    LOG_INFO("Merging configs: {} -> {}", applicationConfigFilePath.toStdString(), loadedConfigFilePath.toStdString());

    QFileInfo loadedConfigInfo(loadedConfigFilePath);
    QDir tempDirectory = loadedConfigInfo.absoluteDir();
    QString mergeRulesPath = tempDirectory.filePath(CONFIG_MERGE_RULES_FILE_NAME);

    QFile appConfigFile(applicationConfigFilePath);
    if (!appConfigFile.open(QIODevice::ReadOnly)) {
        LOG_ERROR("Failed to open application config file: {}", applicationConfigFilePath.toStdString());
        return QJsonObject();
    }

    QByteArray appConfigData = appConfigFile.readAll();
    appConfigFile.close();

    QJsonDocument appConfigDoc = QJsonDocument::fromJson(appConfigData);
    if (appConfigDoc.isNull()) {
        LOG_ERROR("Failed to parse application config JSON: {}", applicationConfigFilePath.toStdString());
        return QJsonObject();
    }

    QJsonObject oldConfig = appConfigDoc.object();
    LOG_DEBUG("Loaded old config with {} keys", oldConfig.keys().size());

    QFile loadedConfigFile(loadedConfigFilePath);
    if (!loadedConfigFile.open(QIODevice::ReadOnly)) {
        LOG_ERROR("Failed to open loaded config file: {}", loadedConfigFilePath.toStdString());
        return QJsonObject();
    }

    QByteArray loadedConfigData = loadedConfigFile.readAll();
    loadedConfigFile.close();

    QJsonDocument loadedConfigDoc = QJsonDocument::fromJson(loadedConfigData);
    if (loadedConfigDoc.isNull()) {
        LOG_ERROR("Failed to parse loaded config JSON: {}", loadedConfigFilePath.toStdString());
        return QJsonObject();
    }

    QJsonObject loadedConfig = loadedConfigDoc.object();
    LOG_DEBUG("Loaded new config with {} keys", loadedConfig.keys().size());

    QFile mergeRulesFile(mergeRulesPath);
    if (mergeRulesFile.exists() && mergeRulesFile.open(QIODevice::ReadOnly)) {
        LOG_INFO("Found merge rules file: {}", mergeRulesPath.toStdString());
        QByteArray mergeRulesData = mergeRulesFile.readAll();
        mergeRulesFile.close();

        QJsonDocument mergeRulesDoc = QJsonDocument::fromJson(mergeRulesData);
        if (!mergeRulesDoc.isNull() && mergeRulesDoc.isArray()) {
            QJsonArray rulesArray = mergeRulesDoc.array();
            LOG_DEBUG("Processing {} merge rules", rulesArray.size());

            int mergedCount = 0;
            for (const QJsonValue& ruleValue : rulesArray) {
                if (!ruleValue.isObject()) {
                    LOG_WARN("Invalid rule format (not an object), skipping");
                    continue;
                }

                QJsonObject rule = ruleValue.toObject();

                if (!rule.contains(JSON_KEY_KEYS) || !rule[JSON_KEY_KEYS].isArray()) {
                    LOG_WARN("Rule missing '{}' array, skipping", JSON_KEY_KEYS);
                    continue;
                }

                QJsonArray keysArray = rule[JSON_KEY_KEYS].toArray();

                if (!rule.contains(JSON_KEY_NEW_KEY) || !rule[JSON_KEY_NEW_KEY].isString()) {
                    LOG_WARN("Rule missing '{}' string, skipping", JSON_KEY_NEW_KEY);
                    continue;
                }

                QString newKey = rule[JSON_KEY_NEW_KEY].toString();

                QJsonValue valueToInsert;
                bool valueFound = false;
                QString foundKey;

                for (const QJsonValue& keyValue : keysArray) {
                    if (!keyValue.isString()) {
                        continue;
                    }

                    QString key = keyValue.toString();
                    if (oldConfig.contains(key)) {
                        valueToInsert = oldConfig[key];
                        valueFound = true;
                        foundKey = key;
                        break;
                    }
                }

                if (valueFound) {
                    loadedConfig[newKey] = valueToInsert;
                    mergedCount++;
                    LOG_DEBUG("Merged value from key '{}' to new key '{}'", foundKey.toStdString(), newKey.toStdString());
                } else {
                    LOG_WARN("No value found for any of the keys in rule for '{}'. Using default value from new config (if any).", newKey.toStdString());
                }
            }
            LOG_INFO("Successfully merged {} values from old config", mergedCount);
        } else {
            LOG_WARN("Merge rules file is not a valid JSON array: {}", mergeRulesPath.toStdString());
        }
    } else {
        LOG_INFO("No merge rules file found at: {}", mergeRulesPath.toStdString());
    }

    return loadedConfig;
}

bool saveConfigToFile(const QString& configFilePath, const QJsonObject& config) {
    LOG_INFO("Saving config to: {}", configFilePath.toStdString());

    QFile configFile(configFilePath);
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        LOG_ERROR("Failed to open config file for writing: {}", configFilePath.toStdString());
        return false;
    }

    QJsonDocument configDoc(config);
    qint64 bytesWritten = configFile.write(configDoc.toJson(QJsonDocument::Indented));
    configFile.close();

    if (bytesWritten > 0) {
        LOG_INFO("Successfully saved config file ({} bytes)", bytesWritten);
        return true;
    } else {
        LOG_ERROR("Failed to write config file: {}", configFilePath.toStdString());
        return false;
    }
}

bool removeMergeRulesTemporaryDirectory(const QString& loadedConfigFilePath) {
    QFileInfo loadedConfigInfo(loadedConfigFilePath);
    QDir tempDirectory = loadedConfigInfo.absoluteDir();
    QString mergeRulesPath = tempDirectory.filePath(CONFIG_MERGE_RULES_FILE_NAME);

    if (QFile::exists(mergeRulesPath)) {
        if (QFile::remove(mergeRulesPath)) {
            LOG_DEBUG("Removed merge rules file: {}", mergeRulesPath.toStdString());
            return true;
        } else {
            LOG_ERROR("Failed to remove merge rules file: {}", mergeRulesPath.toStdString());
            return false;
        }
    }
    return true;
}

bool removeConfigFromTemporaryDirectory(const QString& loadedConfigFilePath) {
    if (QFile::exists(loadedConfigFilePath)) {
        if (QFile::remove(loadedConfigFilePath)) {
            LOG_DEBUG("Removed config from temporary directory: {}", loadedConfigFilePath.toStdString());
            return true;
        } else {
            LOG_ERROR("Failed to remove config from temporary directory: {}", loadedConfigFilePath.toStdString());
            return false;
        }
    }
    return true;
}

bool removeDeletionListFromTemporaryDirectory(const QString& deletionListFilePath) {
    try {
        if (QFile::exists(deletionListFilePath)) {
            if (QFile::remove(deletionListFilePath)) {
                return true;
            } else {
                LOG_ERROR("Failed to remove deletion list file from temporary directory: {}", deletionListFilePath.toStdString());
                return false;
            }
        }
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Exception in removeDeletionListFromTemporaryDirectory: {}", e.what());
        return false;
    }
    catch (...) {
        LOG_ERROR("Unknown exception in removeDeletionListFromTemporaryDirectory");
        return false;
    }
}

bool launchApplication(const QString& appPath) {
    LOG_INFO("Launching application: {}", appPath.toStdString());

    QFileInfo appInfo(appPath);
    if (!appInfo.exists()) {
        LOG_ERROR("Application file does not exist: {}", appPath.toStdString());
        return false;
    }

    if (!QProcess::startDetached(appPath, QStringList(), appInfo.absolutePath())) {
        LOG_ERROR("Failed to launch application: {}", appPath.toStdString());
        return false;
    }

    LOG_INFO("Successfully launched application: {}", appPath.toStdString());
    return true;
}

bool validateArguments(int argc, char* argv[]) {
    if (argc < EXPECTED_ARGUMENT_COUNT) {
        LOG_CRITICAL("Invalid number of arguments. Expected {} arguments, got {}", EXPECTED_USER_ARGUMENT_COUNT, argc - 1);
        LOG_CRITICAL("Usage: {} <pid> <applicationPath> <temporaryUpdateDirectoryPath> <deletionListFileName> <configFileName>", argv[0]);
        return false;
    }

    // Validate PID
    bool ok;
    int pid = QString(argv[1]).toInt(&ok);
    if (!ok || pid <= 0) {
        LOG_CRITICAL("Invalid PID: {}", argv[1]);
        return false;
    }

    // Validate application path
    QString applicationPath = argv[2];
    if (applicationPath.isEmpty()) {
        LOG_CRITICAL("Application path is empty");
        return false;
    }

    // Validate temporary update directory path
    QString temporaryUpdateDirectoryPath = argv[3];
    if (temporaryUpdateDirectoryPath.isEmpty()) {
        LOG_CRITICAL("Temporary update directory path is empty");
        return false;
    }

    QDir tempDir(temporaryUpdateDirectoryPath);
    if (!tempDir.exists()) {
        LOG_CRITICAL("Temporary update directory does not exist: {}", temporaryUpdateDirectoryPath.toStdString());
        return false;
    }

    // Validate deletion list file name
    QString deletionListFileName = argv[4];
    if (deletionListFileName.isEmpty()) {
        LOG_CRITICAL("Deletion list file name is empty");
        return false;
    }

    // Validate config file name
    QString configFileName = argv[5];
    if (configFileName.isEmpty()) {
        LOG_CRITICAL("Config file name is empty");
        return false;
    }

    LOG_INFO("Arguments validated successfully");
    LOG_DEBUG("PID: {}, AppPath: {}, TempDir: {}, DeletionList: {}, ConfigFile: {}", 
        pid, applicationPath.toStdString(), temporaryUpdateDirectoryPath.toStdString(),
        deletionListFileName.toStdString(), configFileName.toStdString());

    return true;
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    // Initialize logger first
    log::initialize();
    LOG_INFO("Update Applier started");

    // Initialize crash catch
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataPath.isEmpty()) {
        appDataPath = QDir::currentPath();
    }
    std::string crashDumpPath = QDir(appDataPath).filePath(CRASHES_DIRECTORY_NAME).toStdString();
    initializeCrashCatch(crashDumpPath + "/" + UPDATE_APPLIER_CRASH_DUMP_NAME, UPDATE_APPLIER_VERSION);

    // Validate arguments
    if (!validateArguments(argc, argv)) {
        LOG_CRITICAL("Argument validation failed, exiting");
        return 1;
    }

    int pid = QString(argv[1]).toInt();
    QString applicationPath = argv[2];
    QString temporaryUpdateDirectoryPath = argv[3];
    QString deletionListFileName = argv[4];
    QString configFileName = argv[5];

    QDir temporaryUpdateDirectory(temporaryUpdateDirectoryPath);
    QDir applicationDirectory(QFileInfo(applicationPath).absolutePath());
    QString removeJsonPath = temporaryUpdateDirectory.filePath(deletionListFileName);

    QString applicationConfigFilePath = applicationDirectory.filePath(configFileName);
    QString loadedConfigFilePath = temporaryUpdateDirectory.filePath(configFileName);

    try {
        // Kill the main application process
        if (!killProcess(pid)) {
            LOG_WARN("Failed to kill process {}, continuing anyway", pid);
        }

        // Wait a bit for files to be released after process termination
        QThread::msleep(100);

        // Delete files listed in JSON
        if (QFile::exists(removeJsonPath)) {
            if (!deleteFilesListedInJson(removeJsonPath)) {
                LOG_WARN("Some files from deletion list could not be deleted");
            }
            
            if (!removeDeletionListFromTemporaryDirectory(removeJsonPath)) {
                LOG_WARN("Failed to remove deletion list file from temporary directory");
            }
        }

        // Merge configs if they differ
        if (configsDiffer(applicationConfigFilePath, loadedConfigFilePath)) {
            LOG_INFO("Configs differ, performing merge");
            QJsonObject mergedConfig = mergeConfigs(applicationConfigFilePath, loadedConfigFilePath);
            
            if (mergedConfig.isEmpty()) {
                LOG_ERROR("Failed to merge configs, aborting");
                return 1;
            }

            if (!saveConfigToFile(applicationConfigFilePath, mergedConfig)) {
                LOG_ERROR("Failed to save merged config, aborting");
                return 1;
            }

            if (!removeMergeRulesTemporaryDirectory(loadedConfigFilePath)) {
                LOG_WARN("Failed to remove merge rules from temporary directory");
            }

            if (!removeConfigFromTemporaryDirectory(loadedConfigFilePath)) {
                LOG_WARN("Failed to remove config from temporary directory");
            }
        } else {
            LOG_INFO("Configs are identical, skipping merge");
        }

        // Copy all files from temporary directory
        LOG_INFO("Copying files from temporary directory to application directory");
        if (!copyFiles(temporaryUpdateDirectory, applicationDirectory)) {
            LOG_ERROR("Some files failed to copy, but continuing");
        }

        // Remove temporary directory
        LOG_INFO("Removing temporary update directory");
        if (!temporaryUpdateDirectory.removeRecursively()) {
            LOG_ERROR("Failed to remove temporary update directory: {}", temporaryUpdateDirectoryPath.toStdString());
        }

        // Launch the application
        if (!launchApplication(applicationPath)) {
            LOG_CRITICAL("Failed to launch application, update may be incomplete");
            return 1;
        }

        LOG_INFO("Update completed successfully");
        log::flush();
        return 0;
    }
    catch (const std::exception& e) {
        LOG_CRITICAL("Exception occurred during update: {}", e.what());
        log::flush();
        return 1;
    }
    catch (...) {
        LOG_CRITICAL("Unknown exception occurred during update");
        log::flush();
        return 1;
    }
}
