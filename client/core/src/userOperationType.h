#pragma once

namespace core
{
    enum class UserOperationType
    {
        AUTHORIZE,
        LOGOUT,
        START_OUTGOING_CALL,
        STOP_OUTGOING_CALL,
        ACCEPT_CALL,
        DECLINE_CALL,
        END_CALL,
        START_SCREEN_SHARING,
        STOP_SCREEN_SHARING,
        START_CAMERA_SHARING,
        STOP_CAMERA_SHARING
    };
}
