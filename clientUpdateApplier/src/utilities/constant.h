#pragma once

// JSON keys
constexpr const char* JSON_KEY_FILE_PATHS = "filePaths";
constexpr const char* JSON_KEY_KEYS = "keys";
constexpr const char* JSON_KEY_NEW_KEY = "newkey";

// File names
constexpr const char* CONFIG_MERGE_RULES_FILE_NAME = "configMergeRules.json";

// Directory names
constexpr const char* CRASHES_DIRECTORY_NAME = "crashes";

// Application names
constexpr const char* UPDATE_APPLIER_CRASH_DUMP_NAME = "calliforniaUpdateApplier";
constexpr const char* UPDATE_APPLIER_VERSION = "___";

// Argument validation
constexpr int EXPECTED_ARGUMENT_COUNT = 6;
constexpr int EXPECTED_USER_ARGUMENT_COUNT = 5;

// Process termination
constexpr int SIGTERM_WAIT_TIME_MS = 1000;
