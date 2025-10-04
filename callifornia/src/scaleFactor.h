#pragma once 

#include <cstdint>
#include <QScreen>
#include <QApplication>

static qreal getDeviceScaleFactor() {
    QScreen* screen = QApplication::primaryScreen();
    return screen->devicePixelRatio();
}

static int scale(int baseSize) {
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