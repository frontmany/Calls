#pragma once

#include <string>
#include "userOperationType.h"

namespace core
{
    struct UserOperation
    {
        UserOperationType type;
        std::string nickname;

        UserOperation(UserOperationType operationType, const std::string& userNickname = "")
            : type(operationType), nickname(userNickname)
        {
        }

        bool operator==(const UserOperation& other) const
        {
            return type == other.type && nickname == other.nickname;
        }
    };
}
