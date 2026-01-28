#pragma once

#include <cerrno>
#include <string>
#include <system_error>
#include "asio.hpp"

namespace serverUpdater::utilities {

/** Returns a locale-independent string for logging std::error_code.
 *  Avoids ec.message() which uses system encoding (e.g. CP1251 on Russian Windows)
 *  and produces mojibake when logs expect UTF-8. */
inline std::string errorCodeForLog(const std::error_code& errorCode) {
    if (!errorCode)
        return "success";
    if (errorCode == asio::error::operation_aborted)
        return "The I/O operation has been aborted";
    int value = errorCode.value();
#if defined(_WIN32) || defined(_WIN64)
    switch (value) {
        case 995:   return "The I/O operation has been aborted";
        case 10054: return "Connection reset by peer";
        case 10053: return "Software caused connection abort";
        case 10061: return "Connection refused";
        case 10060: return "Connection timed out";
        case 10057: return "Socket is not connected";
        case 10050: return "Network is unreachable";
        case 10051: return "Network is down";
        case 10065: return "No route to host";
        default:    return "error " + std::to_string(value);
    }
#else
    switch (value) {
        case ECANCELED:    return "Operation canceled";
        case ECONNRESET:   return "Connection reset by peer";
        case ECONNABORTED: return "Software caused connection abort";
        case ECONNREFUSED: return "Connection refused";
        case ETIMEDOUT:    return "Connection timed out";
        case ENOTCONN:     return "Transport endpoint is not connected";
        case ENETUNREACH:  return "Network is unreachable";
        case ENETDOWN:     return "Network is down";
        case EHOSTUNREACH: return "No route to host";
        default:           return "error " + std::to_string(value);
    }
#endif
}

}
