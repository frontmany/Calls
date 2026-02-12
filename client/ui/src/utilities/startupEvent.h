#pragma once

#include <QEvent>

class StartupEvent : public QEvent
{
public:
    StartupEvent()
        : QEvent(static_cast<QEvent::Type>(StartupEventType))
    {
    }

    static const QEvent::Type StartupEventType;
};
