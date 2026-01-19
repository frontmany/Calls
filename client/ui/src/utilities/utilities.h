#pragma once

#include <QSharedMemory>
#include <QApplication>
#include <QScreen>
#include "updater.h"
#include <QDebug>
#include "constant.h"

static qreal getDeviceScaleFactor() {
    QScreen* screen = QApplication::primaryScreen();
    return screen->devicePixelRatio();
}

static inline int scale(int baseSize) {
    qreal rawFactor = getDeviceScaleFactor();

    if (rawFactor > 1) {
        qreal gap = rawFactor - 1;
        qreal halfGap = gap / 4;
        qreal factor = halfGap + 1;
        return  baseSize / factor;
    }
    else {
        return baseSize / rawFactor;
    }
}

static int extraScale(int size, int count) {
    int result = size;

    for (int i = 0; i < count; i++) {
        result = scale(result);
    }

    return result;
}

inline static bool isFirstInstance() {
    static QSharedMemory sharedMemory(SHARED_MEMORY_NAME);

    if (sharedMemory.attach()) {
        return false;
    }

    if (!sharedMemory.create(1)) {
        return false;
    }

    return true;
}

inline updater::OperationSystemType resolveOperationSystemType() {
#if defined(Q_OS_WINDOWS)
    return updater::OperationSystemType::WINDOWS;
#elif defined(Q_OS_LINUX)
    return updater::OperationSystemType::LINUX;
#elif defined(Q_OS_MACOS)
    return updater::OperationSystemType::MAC;
#else
#if defined(_WIN32)
    return updater::OperationSystemType::WINDOWS;
#elif defined(__linux__)
    return updater::OperationSystemType::LINUX;
#elif defined(__APPLE__)
    return updater::OperationSystemType::MAC;
#else
    qWarning() << "Unknown operating system";
    return updater::OperationSystemType::WINDOWS;
#endif
#endif
}
