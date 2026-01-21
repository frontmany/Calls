#include "configManager.h"
#include "utilities/logger.h"
#include "utilities/configKeys.h"
#include "utilities/utilities.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

ConfigManager::ConfigManager(QString configPath)
    : m_configPath(configPath)
{
    setDefaultValues();
}

ConfigManager::~ConfigManager()
{
    saveConfig();
}

void ConfigManager::loadConfig() {
    try {
        if (!QFile::exists(m_configPath)) {
            LOG_INFO("Config file not found: " + m_configPath.toStdString());

            setDefaultValues();
            saveConfig();
        }
        else {
            m_version = getApplicationVersionFromConfig();
            m_updaterHost = getUpdaterHostFromConfig();
            m_isSpeakerMuted = isSpeakerMutedFromConfig();
            m_isCameraActive = isCameraActiveFromConfig();
            m_isMicrophoneMuted = isMicrophoneMutedFromConfig();
            m_outputVolume = getOutputVolumeFromConfig();
            m_inputVolume = getInputVolumeFromConfig();
            m_isMultiInstanceAllowed = isMultiInstanceAllowedFromConfig();
            m_port = getPortFromConfig();
            m_serverHost = getServerHostFromConfig();
            m_firstLaunch = isFirstLaunchFromConfig();
            m_appDirectory = getAppDirectoryFromConfig();
            m_logDirectory = getLogDirectoryFromConfig();
            m_crashDumpDirectory = getCrashDumpDirectoryFromConfig();
            m_temporaryUpdateDirectory = getTemporaryUpdateDirectoryFromConfig();
            m_deletionListFileName = getDeletionListFileNameFromConfig();
            m_ignoredFilesWhileCollectingForUpdate = getIgnoredFilesWhileCollectingForUpdateFromConfig();
            m_ignoredDirectoriesWhileCollectingForUpdate = getIgnoredDirectoriesWhileCollectingForUpdateFromConfig();
            m_operationSystemType = getOperationSystemTypeFromConfig();
        }

        m_isConfigLoaded = true;
    }
    catch (const std::exception& e) {
        LOG_ERROR(std::string("Failed to load config: ") + e.what());

        if (!m_isConfigLoaded) {
            setDefaultValues();
            saveConfig();
        }
    }
}

void ConfigManager::saveConfig() {
    try {
        QJsonObject configObject;

        configObject[ConfigKeys::VERSION] = m_version;
        configObject[ConfigKeys::UPDATER_HOST] = m_updaterHost;
        configObject[ConfigKeys::SERVER_HOST] = m_serverHost;
        configObject[ConfigKeys::PORT] = m_port;
        configObject[ConfigKeys::MULTI_INSTANCE] = m_isMultiInstanceAllowed ? "1" : "0";
        configObject[ConfigKeys::INPUT_VOLUME] = m_inputVolume;
        configObject[ConfigKeys::OUTPUT_VOLUME] = m_outputVolume;
        configObject[ConfigKeys::MICROPHONE_MUTED] = m_isMicrophoneMuted ? "1" : "0";
        configObject[ConfigKeys::SPEAKER_MUTED] = m_isSpeakerMuted ? "1" : "0";
        configObject[ConfigKeys::CAMERA_ENABLED] = m_isCameraActive ? "1" : "0";
        configObject[ConfigKeys::FIRST_LAUNCH] = m_firstLaunch ? "1" : "0";
        configObject[ConfigKeys::LOG_DIRECTORY] = m_logDirectory;
        configObject[ConfigKeys::CRASH_DUMP_DIRECTORY] = m_crashDumpDirectory;
        configObject[ConfigKeys::APP_DIRECTORY] = m_appDirectory;
        configObject[ConfigKeys::TEMPORARY_UPDATE_DIRECTORY] = m_temporaryUpdateDirectory;
        configObject[ConfigKeys::DELETION_LIST_FILE_NAME] = m_deletionListFileName;
        configObject[ConfigKeys::OPERATION_SYSTEM_TYPE] = static_cast<int>(m_operationSystemType);
        
        QJsonArray ignoredFilesArray;
        for (const std::string& file : m_ignoredFilesWhileCollectingForUpdate) {
            ignoredFilesArray.append(QString::fromStdString(file));
        }
        configObject[ConfigKeys::IGNORED_FILES_WHILE_COLLECTING_FOR_UPDATE] = ignoredFilesArray;
        
        QJsonArray ignoredDirsArray;
        for (const std::string& dir : m_ignoredDirectoriesWhileCollectingForUpdate) {
            ignoredDirsArray.append(QString::fromStdString(dir));
        }
        configObject[ConfigKeys::IGNORED_DIRECTORIES_WHILE_COLLECTING_FOR_UPDATE] = ignoredDirsArray;

        QJsonDocument configDoc(configObject);

        QFile configFile(m_configPath);
        if (!configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            throw std::runtime_error("Cannot open config file for writing: " + m_configPath.toStdString());
        }

        configFile.write(configDoc.toJson(QJsonDocument::Indented));
        configFile.close();

        LOG_INFO("Config saved successfully to: " + m_configPath.toStdString());
    }
    catch (const std::exception& e) {
        LOG_ERROR(std::string("Failed to save config: ") + e.what());
    }
}

bool ConfigManager::isConfigLoaded() {
    return m_isConfigLoaded;
}

const QString& ConfigManager::getConfigPath() {
    return m_configPath;
}

void ConfigManager::setDefaultValues() {
    m_version = "1.0.0";
    m_isSpeakerMuted = false;
    m_isMicrophoneMuted = false;
    m_isMultiInstanceAllowed = false;
    m_isCameraActive = false;
    m_outputVolume = 100;
    m_inputVolume = 100;
    m_port = "8081";
    m_serverHost = "92.255.165.77";
    m_updaterHost = "92.255.165.77";
    m_firstLaunch = true;
    
    QString applicationDirPath = QCoreApplication::applicationDirPath();

    updater::OperationSystemType operationSystemType = resolveOperationSystemType();
    QString basePath;
    
    if (operationSystemType == updater::OperationSystemType::WINDOWS) {
        basePath = "C:/Users/Public/Callifornia";
    }
    else if (operationSystemType == updater::OperationSystemType::LINUX) {
        basePath = "/opt/Callifornia";
    }
    else if (operationSystemType == updater::OperationSystemType::MAC) {
        basePath = "/Applications/Callifornia";
    }
    else {
        basePath = applicationDirPath;
    }
    
    QString validatedBasePath = validateAppDirectoryPath(basePath);
    m_appDirectory = validatedBasePath;
    m_logDirectory = QDir(validatedBasePath).filePath("logs");
    m_crashDumpDirectory = QDir(validatedBasePath).filePath("crashes");
    m_temporaryUpdateDirectory = QDir(validatedBasePath).filePath("update");
    
    m_deletionListFileName = "remove.json";
    m_ignoredFilesWhileCollectingForUpdate = std::unordered_set<std::string>();
    m_ignoredDirectoriesWhileCollectingForUpdate = std::unordered_set<std::string>{"logs", "crashes"};
    m_operationSystemType = operationSystemType;
}

const QString& ConfigManager::getUpdaterHost() const {
    return m_updaterHost;
}

bool ConfigManager::isSpeakerMuted() const {
    return m_isSpeakerMuted;
}

bool ConfigManager::isCameraActive() const {
    return m_isCameraActive;
}

bool ConfigManager::isMicrophoneMuted() const {
    return m_isMicrophoneMuted;
}

int ConfigManager::getOutputVolume() const {
    return m_outputVolume;
}

int ConfigManager::getInputVolume() const {
    return m_inputVolume;
}

bool ConfigManager::isMultiInstanceAllowed() const {
    return m_isMultiInstanceAllowed;
}

const QString& ConfigManager::getPort() const {
    return m_port;
}

const QString& ConfigManager::getVersion() const {
    return m_version;
}

const QString& ConfigManager::getServerHost() const {
    return m_serverHost;
}

void ConfigManager::setVersion(const QString& version) {
    if (m_version != version) {
        m_version = version;
        saveConfig();
    }
}

void ConfigManager::setUpdaterHost(const QString& host) {
    if (m_updaterHost != host) {
        m_updaterHost = host;
        saveConfig();
    }
}

void ConfigManager::setSpeakerMuted(bool muted) {
    if (m_isSpeakerMuted != muted) {
        m_isSpeakerMuted = muted;
        saveConfig();
    }
}

void ConfigManager::setMicrophoneMuted(bool muted) {
    if (m_isMicrophoneMuted != muted) {
        m_isMicrophoneMuted = muted;
        saveConfig();
    }
}

void ConfigManager::setCameraActive(bool active) {
    m_isCameraActive = active;
    saveConfig();
}

bool ConfigManager::isFirstLaunch() const {
    return m_firstLaunch;
}

void ConfigManager::setFirstLaunch(bool firstLaunch) {
    if (m_firstLaunch != firstLaunch) {
        m_firstLaunch = firstLaunch;
        saveConfig();
    }
}

bool ConfigManager::isFirstLaunchFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return true;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return true;
    }

    if (!doc.isObject()) {
        return true;
    }

    QJsonObject jsonObj = doc.object();

    if (!jsonObj.contains(ConfigKeys::FIRST_LAUNCH)) {
        return true;
    }

    QJsonValue firstLaunchValue = jsonObj[ConfigKeys::FIRST_LAUNCH];

    if (firstLaunchValue.isString()) {
        QString value = firstLaunchValue.toString().toLower().trimmed();
        bool result = (value == "1" || value == "true" || value == "yes" || value == "on");
        return result;
    }
    else if (firstLaunchValue.isBool()) {
        return firstLaunchValue.toBool();
    }
    else if (firstLaunchValue.isDouble()) {
        return (firstLaunchValue.toInt() == 1);
    }
    else {
        return true;
    }
}

void ConfigManager::setOutputVolume(int volume) {
    if (volume < 0) volume = 0;
    if (volume > 200) volume = 200;
    
    if (m_outputVolume != volume) {
        m_outputVolume = volume;
        saveConfig();
    }
}

void ConfigManager::setInputVolume(int volume) {
    if (volume < 0) volume = 0;
    if (volume > 200) volume = 200;
    
    if (m_inputVolume != volume) {
        m_inputVolume = volume;
        saveConfig();
    }
}

void ConfigManager::setMultiInstanceAllowed(bool allowed) {
    if (m_isMultiInstanceAllowed != allowed) {
        m_isMultiInstanceAllowed = allowed;
        saveConfig();
    }
}

void ConfigManager::setPort(const QString& port) {
    if (m_port != port) {
        m_port = port;
        saveConfig();
    }
}

void ConfigManager::setServerHost(const QString& host) {
    if (m_serverHost != host) {
        m_serverHost = host;
        saveConfig();
    }
}

QString ConfigManager::getApplicationVersionFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return m_version;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return m_version;
    }

    if (!doc.isObject()) {
        return m_version;
    }

    QJsonObject jsonObj = doc.object();
    QString version = jsonObj[ConfigKeys::VERSION].toString();

    if (version.isEmpty()) {
    }
    else {
    }

    return version;
}

QString ConfigManager::getUpdaterHostFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return m_updaterHost;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return m_updaterHost;
    }

    if (!doc.isObject()) {
        return m_updaterHost;
    }

    QJsonObject jsonObj = doc.object();
    QString host = jsonObj[ConfigKeys::UPDATER_HOST].toString();

    if (host.isEmpty()) {
    }
    else {
    }

    return host;
}

QString ConfigManager::getServerHostFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return m_serverHost;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return m_serverHost;
    }

    if (!doc.isObject()) {
        return m_serverHost;
    }

    QJsonObject jsonObj = doc.object();
    QString host = jsonObj[ConfigKeys::SERVER_HOST].toString();

    if (host.isEmpty()) {
    }
    else {
    }

    return host;
}

QString ConfigManager::getPortFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return m_port;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return m_port;
    }

    if (!doc.isObject()) {
        return m_port;
    }

    QJsonObject jsonObj = doc.object();
    QString port = jsonObj[ConfigKeys::PORT].toString();

    if (port.isEmpty()) {
    }
    else {
    }

    return port;
}

bool ConfigManager::isMultiInstanceAllowedFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return false;
    }

    if (!doc.isObject()) {
        return false;
    }

    QJsonObject jsonObj = doc.object();

    if (!jsonObj.contains(ConfigKeys::MULTI_INSTANCE)) {
        return false;
    }

    QJsonValue multiInstanceValue = jsonObj[ConfigKeys::MULTI_INSTANCE];

    if (multiInstanceValue.isString()) {
        QString value = multiInstanceValue.toString().toLower().trimmed();
        bool result = (value == "1" || value == "true" || value == "yes" || value == "on");
        return result;
    }
    else if (multiInstanceValue.isBool()) {
        bool result = multiInstanceValue.toBool();
        return result;
    }
    else if (multiInstanceValue.isDouble()) {
        bool result = (multiInstanceValue.toInt() == 1);
        return result;
    }
    else {
        return false;
    }
}

int ConfigManager::getInputVolumeFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return 0;
    }

    if (!doc.isObject()) {
        return 0;
    }

    QJsonObject jsonObj = doc.object();
    QJsonValue volumeValue = jsonObj[ConfigKeys::INPUT_VOLUME];
    
    int volume = 100;
    if (volumeValue.isDouble()) {
        volume = volumeValue.toInt();
    }
    else if (volumeValue.isString()) {
        bool ok;
        volume = volumeValue.toString().toInt(&ok);
        if (!ok) {
            volume = 100;
        }
    }
    else {
        volume = 100;
    }
    
    if (volume < 0) volume = 0;
    if (volume > 200) volume = 200;

    return volume;
}

int ConfigManager::getOutputVolumeFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return 0;
    }

    if (!doc.isObject()) {
        return 0;
    }

    QJsonObject jsonObj = doc.object();
    QJsonValue volumeValue = jsonObj[ConfigKeys::OUTPUT_VOLUME];
    
    int volume = 100;
    if (volumeValue.isDouble()) {
        volume = volumeValue.toInt();
    }
    else if (volumeValue.isString()) {
        bool ok;
        volume = volumeValue.toString().toInt(&ok);
        if (!ok) {
            volume = 100;
        }
    }
    else {
        volume = 100;
    }
    
    if (volume < 0) volume = 0;
    if (volume > 200) volume = 200;

    return volume;
}

bool ConfigManager::isMicrophoneMutedFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return false;
    }

    if (!doc.isObject()) {
        return false;
    }

    QJsonObject jsonObj = doc.object();

    if (!jsonObj.contains(ConfigKeys::MICROPHONE_MUTED)) {
        return false;
    }

    QJsonValue mutedValue = jsonObj[ConfigKeys::MICROPHONE_MUTED];

    if (mutedValue.isString()) {
        QString value = mutedValue.toString().toLower().trimmed();
        bool result = (value == "1" || value == "true" || value == "yes" || value == "on");
        return result;
    }
    else if (mutedValue.isBool()) {
        bool result = mutedValue.toBool();
        return result;
    }
    else if (mutedValue.isDouble()) {
        bool result = (mutedValue.toInt() == 1);
        return result;
    }
    else {
        return false;
    }
}

bool ConfigManager::isSpeakerMutedFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return false;
    }

    if (!doc.isObject()) {
        return false;
    }

    QJsonObject jsonObj = doc.object();

    if (!jsonObj.contains(ConfigKeys::SPEAKER_MUTED)) {
        return false;
    }

    QJsonValue mutedValue = jsonObj[ConfigKeys::SPEAKER_MUTED];

    if (mutedValue.isString()) {
        QString value = mutedValue.toString().toLower().trimmed();
        bool result = (value == "1" || value == "true" || value == "yes" || value == "on");
        return result;
    }
    else if (mutedValue.isBool()) {
        bool result = mutedValue.toBool();
        return result;
    }
    else if (mutedValue.isDouble()) {
        bool result = (mutedValue.toInt() == 1);
        return result;
    }
    else {
        return false;
    }
}

bool ConfigManager::isCameraActiveFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return false;
    }

    if (!doc.isObject()) {
        return false;
    }

    QJsonObject jsonObj = doc.object();

    if (!jsonObj.contains(ConfigKeys::CAMERA_ENABLED)) {
        return false;
    }

    QJsonValue enabledValue = jsonObj[ConfigKeys::CAMERA_ENABLED];

    if (enabledValue.isString()) {
        QString value = enabledValue.toString().toLower().trimmed();
        bool result = (value == "1" || value == "true" || value == "yes" || value == "on");
        return result;
    }
    else if (enabledValue.isBool()) {
        bool result = enabledValue.toBool();
        return result;
    }
    else if (enabledValue.isDouble()) {
        bool result = (enabledValue.toInt() == 1);
        return result;
    }
    else {
        return false;
    }
}

const QString& ConfigManager::getLogDirectoryPath() const {
    return m_logDirectory;
}

const QString& ConfigManager::getCrashDumpDirectoryPath() const {
    return m_crashDumpDirectory;
}

const QString& ConfigManager::getAppDirectoryPath() const {
    return m_appDirectory;
}

const QString& ConfigManager::getTemporaryUpdateDirectoryPath() const {
    return m_temporaryUpdateDirectory;
}

const QString& ConfigManager::getDeletionListFileName() const {
    return m_deletionListFileName;
}

const std::unordered_set<std::string>& ConfigManager::getIgnoredFilesWhileCollectingForUpdate() const {
    return m_ignoredFilesWhileCollectingForUpdate;
}

const std::unordered_set<std::string>& ConfigManager::getIgnoredDirectoriesWhileCollectingForUpdate() const {
    return m_ignoredDirectoriesWhileCollectingForUpdate;
}

QString ConfigManager::getLogDirectoryFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return m_logDirectory;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return m_logDirectory;
    }

    if (!doc.isObject()) {
        return m_logDirectory;
    }

    QJsonObject jsonObj = doc.object();
    QString logDirPath = jsonObj[ConfigKeys::LOG_DIRECTORY].toString();
    
    if (!logDirPath.isEmpty()) {
        return logDirPath;
    }

    return m_logDirectory;
}

QString ConfigManager::getCrashDumpDirectoryFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return m_crashDumpDirectory;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return m_crashDumpDirectory;
    }

    if (!doc.isObject()) {
        return m_crashDumpDirectory;
    }

    QJsonObject jsonObj = doc.object();
    QString crashDirPath = jsonObj[ConfigKeys::CRASH_DUMP_DIRECTORY].toString();
    
    if (!crashDirPath.isEmpty()) {
        return crashDirPath;
    }

    return m_crashDumpDirectory;
}

QString ConfigManager::getAppDirectoryFromConfig() {
    QString applicationDirPath = QCoreApplication::applicationDirPath();
    
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return applicationDirPath;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return applicationDirPath;
    }

    if (!doc.isObject()) {
        return applicationDirPath;
    }

    QJsonObject jsonObj = doc.object();
    QString appDir = jsonObj[ConfigKeys::APP_DIRECTORY].toString();

    if (!appDir.isEmpty()) {
        return validateAppDirectoryPath(appDir);
    }

    return applicationDirPath;
}

QString ConfigManager::validateAppDirectoryPath(const QString& path) {
    QString applicationDirPath = QCoreApplication::applicationDirPath();
    
    QDir configDir(path);
    QDir actualDir(applicationDirPath);
    QString normalizedConfigPath = QDir::cleanPath(configDir.absolutePath());
    QString normalizedAppPath = QDir::cleanPath(actualDir.absolutePath());
    
    if (normalizedConfigPath.compare(normalizedAppPath, Qt::CaseInsensitive) == 0) {
        return path;
    } else {
        return applicationDirPath;
    }
}

QString ConfigManager::getTemporaryUpdateDirectoryFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return m_temporaryUpdateDirectory;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return m_temporaryUpdateDirectory;
    }

    if (!doc.isObject()) {
        return m_temporaryUpdateDirectory;
    }

    QJsonObject jsonObj = doc.object();
    QString tempDirPath = jsonObj[ConfigKeys::TEMPORARY_UPDATE_DIRECTORY].toString();
    
    if (!tempDirPath.isEmpty()) {
        return tempDirPath;
    }

    return m_temporaryUpdateDirectory;
}

QString ConfigManager::getDeletionListFileNameFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return "remove.json";
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return "remove.json";
    }

    if (!doc.isObject()) {
        return "remove.json";
    }

    QJsonObject jsonObj = doc.object();
    QString fileName = jsonObj[ConfigKeys::DELETION_LIST_FILE_NAME].toString();

    if (fileName.isEmpty()) {
        return "remove.json";
    }

    return fileName;
}

std::unordered_set<std::string> ConfigManager::getIgnoredFilesWhileCollectingForUpdateFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return std::unordered_set<std::string>();
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return std::unordered_set<std::string>();
    }

    if (!doc.isObject()) {
        return std::unordered_set<std::string>();
    }

    QJsonObject jsonObj = doc.object();
    QJsonValue ignoredFilesValue = jsonObj[ConfigKeys::IGNORED_FILES_WHILE_COLLECTING_FOR_UPDATE];

    if (!ignoredFilesValue.isArray()) {
        return std::unordered_set<std::string>();
    }

    std::unordered_set<std::string> ignoredFiles;
    QJsonArray array = ignoredFilesValue.toArray();
    for (const QJsonValue& value : array) {
        if (value.isString()) {
            ignoredFiles.insert(value.toString().toStdString());
        }
    }

    return ignoredFiles;
}

std::unordered_set<std::string> ConfigManager::getIgnoredDirectoriesWhileCollectingForUpdateFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return std::unordered_set<std::string>{"logs"};
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return std::unordered_set<std::string>{"logs"};
    }

    if (!doc.isObject()) {
        return std::unordered_set<std::string>{"logs"};
    }

    QJsonObject jsonObj = doc.object();
    QJsonValue ignoredDirsValue = jsonObj[ConfigKeys::IGNORED_DIRECTORIES_WHILE_COLLECTING_FOR_UPDATE];

    if (!ignoredDirsValue.isArray()) {
        return std::unordered_set<std::string>{"logs"};
    }

    std::unordered_set<std::string> ignoredDirs;
    QJsonArray array = ignoredDirsValue.toArray();
    for (const QJsonValue& value : array) {
        if (value.isString()) {
            ignoredDirs.insert(value.toString().toStdString());
        }
    }

    return ignoredDirs;
}

updater::OperationSystemType ConfigManager::getOperationSystemType() const {
    return m_operationSystemType;
}

void ConfigManager::setOperationSystemType(updater::OperationSystemType operationSystemType) {
    if (m_operationSystemType != operationSystemType) {
        m_operationSystemType = operationSystemType;
        saveConfig();
    }
}

updater::OperationSystemType ConfigManager::getOperationSystemTypeFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return resolveOperationSystemType();
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return resolveOperationSystemType();
    }

    if (!doc.isObject()) {
        return resolveOperationSystemType();
    }

    QJsonObject jsonObj = doc.object();

    if (!jsonObj.contains(ConfigKeys::OPERATION_SYSTEM_TYPE)) {
        return resolveOperationSystemType();
    }

    QJsonValue operationSystemTypeValue = jsonObj[ConfigKeys::OPERATION_SYSTEM_TYPE];
    
    if (operationSystemTypeValue.isDouble()) {
        int operationSystemTypeInt = operationSystemTypeValue.toInt();
        if (operationSystemTypeInt >= 0 && operationSystemTypeInt <= 2) {
            return static_cast<updater::OperationSystemType>(operationSystemTypeInt);
        }
    }
    else if (operationSystemTypeValue.isString()) {
        QString operationSystemTypeStr = operationSystemTypeValue.toString().toUpper().trimmed();
        if (operationSystemTypeStr == "WINDOWS") {
            return updater::OperationSystemType::WINDOWS;
        }
        else if (operationSystemTypeStr == "LINUX") {
            return updater::OperationSystemType::LINUX;
        }
        else if (operationSystemTypeStr == "MAC") {
            return updater::OperationSystemType::MAC;
        }
    }

    return resolveOperationSystemType();
}
