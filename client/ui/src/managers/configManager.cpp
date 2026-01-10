#include "configManager.h"
#include "utilities/logger.h"
#include "utilities/configKeys.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

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
        m_logDirectoryName = getLogDirectoryNameFromConfig();
        m_temporaryUpdateDirectoryName = getTemporaryUpdateDirectoryNameFromConfig();
        m_deletionListFileName = getDeletionListFileNameFromConfig();
        m_ignoredFilesWhileCollectingForUpdate = getIgnoredFilesWhileCollectingForUpdateFromConfig();
        m_ignoredDirectoriesWhileCollectingForUpdate = getIgnoredDirectoriesWhileCollectingForUpdateFromConfig();

        m_isConfigLoaded = true;
    }
    catch (const std::exception& e) {
        LOG_ERROR(std::string("Failed to load config: ") + e.what());

        if (!m_isConfigLoaded)
            setDefaultValues();
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
        configObject[ConfigKeys::LOG_DIRECTORY_NAME] = m_logDirectoryName;
        configObject[ConfigKeys::TEMPORARY_UPDATE_DIRECTORY_NAME] = m_temporaryUpdateDirectoryName;
        configObject[ConfigKeys::DELETION_LIST_FILE_NAME] = m_deletionListFileName;
        
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

        qDebug() << "Config saved successfully to:" << m_configPath;
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
    m_logDirectoryName = "logs";
    m_temporaryUpdateDirectoryName = "updateTemp";
    m_deletionListFileName = "remove.json";
    m_ignoredFilesWhileCollectingForUpdate = std::unordered_set<std::string>();
    m_ignoredDirectoriesWhileCollectingForUpdate = std::unordered_set<std::string>{"logs"};
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
        LOG_WARN("Failed to open config file: {}", m_configPath.toStdString());
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
    QString version = jsonObj[ConfigKeys::VERSION].toString();

    if (version.isEmpty()) {
        LOG_WARN("version not found or empty in config file");
    }
    else {
    }

    return version;
}

QString ConfigManager::getUpdaterHostFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARN("Failed to open config file: {}", m_configPath.toStdString());
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
    QString host = jsonObj[ConfigKeys::UPDATER_HOST].toString();

    if (host.isEmpty()) {
        LOG_WARN("updaterHost not found or empty in config file");
    }
    else {
    }

    return host;
}

QString ConfigManager::getServerHostFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARN("Failed to open config file: {}", m_configPath.toStdString());
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
    QString host = jsonObj[ConfigKeys::SERVER_HOST].toString();

    if (host.isEmpty()) {
        LOG_WARN("serverHost not found or empty in config file");
    }
    else {
    }

    return host;
}

QString ConfigManager::getPortFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARN("Failed to open config file: {}", m_configPath.toStdString());
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
    QString port = jsonObj[ConfigKeys::PORT].toString();

    if (port.isEmpty()) {
        LOG_WARN("port not found or empty in config file");
    }
    else {
    }

    return port;
}

bool ConfigManager::isMultiInstanceAllowedFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARN("Failed to open config file: {}", m_configPath.toStdString());
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
        LOG_WARN("multiInstance has unsupported type, defaulting to false");
        return false;
    }
}

int ConfigManager::getInputVolumeFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARN("Failed to open config file: {}", m_configPath.toStdString());
        return 0;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_WARN("JSON parse error: {}", parseError.errorString().toStdString());
        return 0;
    }

    if (!doc.isObject()) {
        LOG_WARN("Config file does not contain a JSON object");
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
            LOG_WARN("inputVolume is not a valid number in config file");
            volume = 100;
        }
    }
    else {
        LOG_WARN("inputVolume not found or invalid type in config file");
        volume = 100;
    }
    
    if (volume < 0) volume = 0;
    if (volume > 200) volume = 200;

    return volume;
}

int ConfigManager::getOutputVolumeFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARN("Failed to open config file: {}", m_configPath.toStdString());
        return 0;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_WARN("JSON parse error: {}", parseError.errorString().toStdString());
        return 0;
    }

    if (!doc.isObject()) {
        LOG_WARN("Config file does not contain a JSON object");
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
            LOG_WARN("outputVolume is not a valid number in config file");
            volume = 100;
        }
    }
    else {
        LOG_WARN("outputVolume not found or invalid type in config file");
        volume = 100;
    }
    
    if (volume < 0) volume = 0;
    if (volume > 200) volume = 200;

    return volume;
}

bool ConfigManager::isMicrophoneMutedFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARN("Failed to open config file: {}", m_configPath.toStdString());
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
        LOG_WARN("microphoneMuted has unsupported type, defaulting to false");
        return false;
    }
}

bool ConfigManager::isSpeakerMutedFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARN("Failed to open config file: {}", m_configPath.toStdString());
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
        LOG_WARN("speakerMuted has unsupported type, defaulting to false");
        return false;
    }
}

bool ConfigManager::isCameraActiveFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARN("Failed to open config file: {}", m_configPath.toStdString());
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
        LOG_WARN("cameraEnabled has unsupported type, defaulting to false");
        return false;
    }
}

const QString& ConfigManager::getLogDirectoryName() const {
    return m_logDirectoryName;
}

const QString& ConfigManager::getTemporaryUpdateDirectoryName() const {
    return m_temporaryUpdateDirectoryName;
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

QString ConfigManager::getLogDirectoryNameFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARN("Failed to open config file: {}", m_configPath.toStdString());
        return "logs";
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_WARN("JSON parse error: {}", parseError.errorString().toStdString());
        return "logs";
    }

    if (!doc.isObject()) {
        LOG_WARN("Config file does not contain a JSON object");
        return "logs";
    }

    QJsonObject jsonObj = doc.object();
    QString logDirName = jsonObj[ConfigKeys::LOG_DIRECTORY_NAME].toString();

    if (logDirName.isEmpty()) {
        return "logs";
    }

    return logDirName;
}

QString ConfigManager::getTemporaryUpdateDirectoryNameFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARN("Failed to open config file: {}", m_configPath.toStdString());
        return "updateTemp";
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_WARN("JSON parse error: {}", parseError.errorString().toStdString());
        return "updateTemp";
    }

    if (!doc.isObject()) {
        LOG_WARN("Config file does not contain a JSON object");
        return "updateTemp";
    }

    QJsonObject jsonObj = doc.object();
    QString tempDirName = jsonObj[ConfigKeys::TEMPORARY_UPDATE_DIRECTORY_NAME].toString();

    if (tempDirName.isEmpty()) {
        return "updateTemp";
    }

    return tempDirName;
}

QString ConfigManager::getDeletionListFileNameFromConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARN("Failed to open config file: {}", m_configPath.toStdString());
        return "remove.json";
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_WARN("JSON parse error: {}", parseError.errorString().toStdString());
        return "remove.json";
    }

    if (!doc.isObject()) {
        LOG_WARN("Config file does not contain a JSON object");
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
        LOG_WARN("Failed to open config file: {}", m_configPath.toStdString());
        return std::unordered_set<std::string>();
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_WARN("JSON parse error: {}", parseError.errorString().toStdString());
        return std::unordered_set<std::string>();
    }

    if (!doc.isObject()) {
        LOG_WARN("Config file does not contain a JSON object");
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
        LOG_WARN("Failed to open config file: {}", m_configPath.toStdString());
        return std::unordered_set<std::string>{"logs"};
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        LOG_WARN("JSON parse error: {}", parseError.errorString().toStdString());
        return std::unordered_set<std::string>{"logs"};
    }

    if (!doc.isObject()) {
        LOG_WARN("Config file does not contain a JSON object");
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