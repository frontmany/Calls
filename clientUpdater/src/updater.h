#pragma once 

#include "clientUpdater.h"

namespace updater
{

inline void init(std::unique_ptr<CallbacksInterface>&& callbacksHandler)
{
    ClientUpdater::get().init(std::move(callbacksHandler));
}

inline bool connect(const std::string& host, const std::string& port)
{
    return ClientUpdater::get().connect(host, port);
}

inline void disconnect()
{
    ClientUpdater::get().disconnect();
}

inline void checkUpdates(const std::string& currentVersionNumber)
{
    ClientUpdater::get().checkUpdates(currentVersionNumber);
}

inline bool startUpdate(OperationSystemType type)
{
    return ClientUpdater::get().startUpdate(type);
}

inline bool isConnected()
{
    return ClientUpdater::get().isConnected();
}

inline bool isAwaitingServerResponse()
{
    return ClientUpdater::get().isAwaitingServerResponse();
}

inline bool isAwaitingCheckUpdatesFunctionCall()
{
    return ClientUpdater::get().isAwaitingCheckUpdatesFunctionCall();
}

inline bool isAwaitingStartUpdateFunctionCall()
{
    return ClientUpdater::get().isAwaitingStartUpdateFunctionCall();
}

inline const std::string& getServerHost()
{
    return ClientUpdater::get().getServerHost();
}

inline const std::string& getServerPort()
{
    return ClientUpdater::get().getServerPort();
}

}