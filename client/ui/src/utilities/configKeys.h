#pragma once

#include <QString>

namespace ConfigKeys
{
    const QString VERSION = "version";
    const QString UPDATER_HOST = "updaterHost";
    const QString SERVER_HOST = "serverHost";
    const QString PORT = "port";
    const QString MULTI_INSTANCE = "multiInstance";
    const QString INPUT_VOLUME = "inputVolume";
    const QString OUTPUT_VOLUME = "outputVolume";
    const QString MICROPHONE_MUTED = "microphoneMuted";
    const QString SPEAKER_MUTED = "speakerMuted";
    const QString CAMERA_ENABLED = "cameraEnabled";
    const QString FIRST_LAUNCH = "firstLaunch";
    const QString LOG_DIRECTORY = "logDirectory";
    const QString CRASH_DUMP_DIRECTORY = "crashDumpDirectory";
    const QString APP_DIRECTORY = "appDirectory";
    const QString TEMPORARY_UPDATE_DIRECTORY = "temporaryUpdateDirectory";
    const QString DELETION_LIST_FILE_NAME = "deletionListFileNameName";
    const QString IGNORED_FILES_WHILE_COLLECTING_FOR_UPDATE = "ignoredFilesWhileCollectingForUpdate";
    const QString IGNORED_DIRECTORIES_WHILE_COLLECTING_FOR_UPDATE = "ignoredDirectoriesWhileCollectingForUpdate";
}
