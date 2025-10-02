#pragma once

namespace calls {
    enum class State
    {
        UNAUTHORIZED,
        FREE,
        CALLING,
        BUSY,
        IN_GROUP_CALL
    };
}