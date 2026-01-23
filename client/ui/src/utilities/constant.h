#pragma once

// Application executable names
constexpr const char* APPLICATION_EXECUTABLE_NAME_WINDOWS = "callifornia.exe";
constexpr const char* APPLICATION_EXECUTABLE_NAME_LINUX_AND_MAC = "callifornia";

// Update applier executable names
constexpr const char* UPDATE_APPLIER_EXECUTABLE_NAME_WINDOWS = "updateApplier.exe";
constexpr const char* UPDATE_APPLIER_EXECUTABLE_NAME_LINUX_AND_MAC = "updateApplier";
constexpr const char* UPDATE_APPLIER_EXECUTABLE_NAME_WINDOWS_NEW = "updateApplierNEW.exe";
constexpr const char* UPDATE_APPLIER_EXECUTABLE_NAME_LINUX_AND_MAC_NEW = "updateApplierNEW";

// Shared memory name
constexpr const char* SHARED_MEMORY_NAME = "callifornia";

// ============================================================================
// UI Logic Constants
// ============================================================================

// Time constants (milliseconds)
constexpr int TIMER_INTERVAL_MS = 1000;                    // Timer update interval (1 second)
constexpr int ERROR_MESSAGE_DURATION_MS = 2500;            // Default error message display duration
constexpr int UPDATE_APPLIER_DELAY_MS = 600;               // Delay before launching update applier
constexpr int BLUR_ANIMATION_DURATION_MS = 1200;           // Blur animation duration
constexpr int UI_ANIMATION_DURATION_MS = 200;              // Standard UI animation duration
constexpr int SCREEN_CAPTURE_INTERVAL_MS = 72;             // Screen capture frame interval (~14 FPS)

// Incoming call constants
constexpr int DEFAULT_INCOMING_CALL_SECONDS = 32;          // Default incoming call timeout

// Volume constants
constexpr int DEFAULT_VOLUME = 100;                        // Default volume level (0-200)
constexpr int MIN_VOLUME = 0;                              // Minimum volume level
constexpr int MAX_VOLUME = 200;                            // Maximum volume level

// Nickname validation constants
constexpr int MIN_NICKNAME_LENGTH = 3;                     // Minimum nickname length
constexpr int MAX_NICKNAME_LENGTH = 15;                    // Maximum nickname length

// Blur animation constants
constexpr int BLUR_ANIMATION_END_VALUE = 10;               // Final blur radius value

// Configuration default values
constexpr const char* DEFAULT_VERSION = "1.0.0";           // Default application version
constexpr const char* DEFAULT_PORT = "8081";               // Default server port
constexpr const char* DEFAULT_SERVER_HOST = "92.255.165.77";  // Default server host
constexpr const char* DEFAULT_UPDATER_HOST = "92.255.165.77"; // Default updater host
constexpr const char* DELETION_LIST_FILE_NAME = "remove.json"; // Update deletion list filename

// Update ignored directories
constexpr const char* IGNORED_DIRECTORY_LOGS = "logs";     // Logs directory name
constexpr const char* IGNORED_DIRECTORY_CRASHES = "crashes"; // Crashes directory name
