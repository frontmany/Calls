#pragma once

#include <QSharedMemory>
#include <QApplication>

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
    static QSharedMemory sharedMemory("callifornia");

    if (sharedMemory.attach()) {
        return false;
    }

    if (!sharedMemory.create(1)) {
        return false;
    }

    return true;
}
