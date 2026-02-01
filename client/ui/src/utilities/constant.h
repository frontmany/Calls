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
constexpr int SCREEN_CAPTURE_INTERVAL_MS = 50;             // Screen capture frame interval (~14 FPS)

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
constexpr const char* DEFAULT_MAIN_SERVER_TCP_PORT = "8081";
constexpr const char* DEFAULT_MAIN_SERVER_UDP_PORT = "8081";
constexpr const char* DEFAULT_UPDATER_SERVER_TCP_PORT = "8082";
constexpr const char* DEFAULT_SERVER_HOST = "92.255.165.77";  // Default server host
constexpr const char* DEFAULT_UPDATER_HOST = "92.255.165.77"; // Default updater host
constexpr const char* DELETION_LIST_FILE_NAME = "remove.json"; // Update deletion list filename

// Update ignored directories
constexpr const char* IGNORED_DIRECTORY_LOGS = "logs";     // Logs directory name
constexpr const char* IGNORED_DIRECTORY_CRASHES = "crashes"; // Crashes directory name

// ============================================================================
// AV1 Encoding Constants
// ============================================================================

// AV1 encoding parameters
constexpr int AV1_PRESET = 13;                           // SVT-AV1 preset (7-13 for real-time, higher = faster)
constexpr int AV1_CRF = 32;                               // Quality (0-63, lower = better quality)
constexpr int AV1_KEYINT = 60;                            // Keyframe interval in frames (~3 seconds at 10 FPS)
constexpr int AV1_TARGET_WIDTH = 1920;                    // Target width for encoding
constexpr int AV1_TARGET_HEIGHT = 1080;                   // Target height for encoding
constexpr double AV1_FPS = 20.0;                          // Frames per second (50ms interval)
 