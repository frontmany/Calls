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
constexpr int SCREEN_CAPTURE_INTERVAL_MS = 33;             // Screen capture frame interval (~30 FPS)
constexpr int CAMERA_CAPTURE_INTERVAL_MS = 50;             // Camera capture frame interval (~20 FPS)

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
// H.264 encoding parameters
// ============================================================================

// H.264 Bitrate presets (for 1080p@30fps)
constexpr int H264_BITRATE_HIGH = 4000000;                   // 4 Mbps - high quality
constexpr int H264_BITRATE_MEDIUM = 2000000;                // 2 Mbps - medium quality  
constexpr int H264_BITRATE_LOW = 1000000;                   // 1 Mbps - low quality

// Special presets for camera and screen (optimized for performance)
constexpr int H264_BITRATE_CAMERA = 1200000;                // 1.2 Mbps - camera 720p optimized
constexpr int H264_BITRATE_SCREEN = 1500000;                 // 1.5 Mbps - screen high quality

// Additional presets for simultaneous streaming
constexpr int H264_BITRATE_CAMERA_SIMULTANEOUS = 1000000;   // 1.0 Mbps - camera when both active
constexpr int H264_BITRATE_SCREEN_SIMULTANEOUS = 1200000;   // 1.2 Mbps - screen when both active

// Adaptive resolution presets
constexpr int H264_CAMERA_WIDTH = 1280;                     // Camera width: 720p
constexpr int H264_CAMERA_HEIGHT = 720;                     // Camera height: 720p
constexpr int H264_SCREEN_WIDTH = 1920;                     // Screen width: 1080p (adaptive)
constexpr int H264_SCREEN_HEIGHT = 1080;                    // Screen height: 1080p (adaptive)

// H.264 encoding parameters
constexpr int H264_CAMERA_FPS = 30;                         // Camera FPS for performance
constexpr int H264_SCREEN_FPS = 30;                         // Screen FPS for better quality
constexpr int H264_KEYINT = 30;                            // Keyframe interval in frames

// H.264 Quality presets
constexpr int H264_QUALITY_MAX_QP_BEST = 30;                // Best quality (lower QP)
constexpr int H264_QUALITY_MAX_QP_BALANCED = 38;             // Balanced quality/speed
constexpr int H264_QUALITY_MAX_QP_FAST = 46;                // Fast encoding (higher QP)

constexpr int H264_QUALITY_MIN_QP_BEST = 18;                // Best quality min QP
constexpr int H264_QUALITY_MIN_QP_BALANCED = 20;             // Balanced min QP
constexpr int H264_QUALITY_MIN_QP_FAST = 24;                 // Fast min QP

// Special QP presets for camera and screen (optimized for performance)
constexpr int H264_QUALITY_MAX_QP_CAMERA = 28;               // Camera optimized (faster encoding)
constexpr int H264_QUALITY_MIN_QP_CAMERA = 22;               // Camera optimized min QP

constexpr int H264_QUALITY_MAX_QP_SCREEN = 30;               // Screen optimized (better quality)
constexpr int H264_QUALITY_MIN_QP_SCREEN = 20;               // Screen optimized min QP

// H.264 Performance modes
constexpr int H264_THREADS_BEST = 1;                        // Best quality (1 thread)
constexpr int H264_THREADS_BALANCED = 2;                     // Balanced (2 threads)
constexpr int H264_THREADS_FAST = 4;                        // Fast (4+ threads)

constexpr int H264_ENTROPY_BEST = 1;                         // CABAC (better compression)
constexpr int H264_ENTROPY_FAST = 0;                         // CAVLC (faster)