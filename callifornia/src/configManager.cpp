#include "configManager.h"
#include "logger.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

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

        configObject["version"] = m_version;
        configObject["updaterHost"] = m_updaterHost;
        configObject["serverHost"] = m_serverHost;
        configObject["port"] = m_port;
        configObject["multiInstance"] = m_isMultiInstanceAllowed ? "1" : "0";
        configObject["inputVolume"] = m_inputVolume;
        configObject["outputVolume"] = m_outputVolume;
        configObject["microphoneMuted"] = m_isMicrophoneMuted ? "1" : "0";
        configObject["speakerMuted"] = m_isSpeakerMuted ? "1" : "0";
        configObject["cameraEnabled"] = m_isCameraActive ? "1" : "0";

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
}

QString ConfigManager::getUpdaterHost() const {
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

QString ConfigManager::getPort() const {
    return m_port;
}

QString ConfigManager::getServerHost() const {
    return m_serverHost;
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
    QString version = jsonObj["version"].toString();

    if (version.isEmpty()) {
        LOG_WARN("version not found or empty in config file");
    }
    else {
        LOG_DEBUG("Retrieved version: {}", version.toStdString());
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
    QString host = jsonObj["updaterHost"].toString();

    if (host.isEmpty()) {
        LOG_WARN("updaterHost not found or empty in config file");
    }
    else {
        LOG_DEBUG("Retrieved updater host: {}", host.toStdString());
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
    QString host = jsonObj["serverHost"].toString();

    if (host.isEmpty()) {
        LOG_WARN("serverHost not found or empty in config file");
    }
    else {
        LOG_DEBUG("Retrieved server host: {}", host.toStdString());
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
    QString port = jsonObj["port"].toString();

    if (port.isEmpty()) {
        LOG_WARN("port not found or empty in config file");
    }
    else {
        LOG_DEBUG("Retrieved port: {}", port.toStdString());
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
    QJsonValue volumeValue = jsonObj["inputVolume"];
    
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
    QJsonValue volumeValue = jsonObj["outputVolume"];
    
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

    if (!jsonObj.contains("microphoneMuted")) {
        LOG_DEBUG("microphoneMuted key not found in config, defaulting to false");
        return false;
    }

    QJsonValue mutedValue = jsonObj["microphoneMuted"];

    if (mutedValue.isString()) {
        QString value = mutedValue.toString().toLower().trimmed();
        bool result = (value == "1" || value == "true" || value == "yes" || value == "on");
        LOG_DEBUG("microphoneMuted string value: '{}' -> {}", value.toStdString(), result);
        return result;
    }
    else if (mutedValue.isBool()) {
        bool result = mutedValue.toBool();
        LOG_DEBUG("microphoneMuted bool value: {}", result);
        return result;
    }
    else if (mutedValue.isDouble()) {
        bool result = (mutedValue.toInt() == 1);
        LOG_DEBUG("microphoneMuted numeric value: {} -> {}", mutedValue.toInt(), result);
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

    if (!jsonObj.contains("speakerMuted")) {
        LOG_DEBUG("speakerMuted key not found in config, defaulting to false");
        return false;
    }

    QJsonValue mutedValue = jsonObj["speakerMuted"];

    if (mutedValue.isString()) {
        QString value = mutedValue.toString().toLower().trimmed();
        bool result = (value == "1" || value == "true" || value == "yes" || value == "on");
        LOG_DEBUG("speakerMuted string value: '{}' -> {}", value.toStdString(), result);
        return result;
    }
    else if (mutedValue.isBool()) {
        bool result = mutedValue.toBool();
        LOG_DEBUG("speakerMuted bool value: {}", result);
        return result;
    }
    else if (mutedValue.isDouble()) {
        bool result = (mutedValue.toInt() == 1);
        LOG_DEBUG("speakerMuted numeric value: {} -> {}", mutedValue.toInt(), result);
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

    if (!jsonObj.contains("cameraEnabled")) {
        LOG_DEBUG("cameraEnabled key not found in config, defaulting to false");
        return false;
    }

    QJsonValue enabledValue = jsonObj["cameraEnabled"];

    if (enabledValue.isString()) {
        QString value = enabledValue.toString().toLower().trimmed();
        bool result = (value == "1" || value == "true" || value == "yes" || value == "on");
        LOG_DEBUG("cameraEnabled string value: '{}' -> {}", value.toStdString(), result);
        return result;
    }
    else if (enabledValue.isBool()) {
        bool result = enabledValue.toBool();
        LOG_DEBUG("cameraEnabled bool value: {}", result);
        return result;
    }
    else if (enabledValue.isDouble()) {
        bool result = (enabledValue.toInt() == 1);
        LOG_DEBUG("cameraEnabled numeric value: {} -> {}", enabledValue.toInt(), result);
        return result;
    }
    else {
        LOG_WARN("cameraEnabled has unsupported type, defaulting to false");
        return false;
    }
}