#pragma once

#include <QObject>
#include <QMediaDevices>
#include <memory>

#include "core.h"

class AudioDevicesWatcher : public QObject
{
    Q_OBJECT

public:
    explicit AudioDevicesWatcher(std::shared_ptr<core::Core> client, QObject* parent = nullptr);

signals:
    void devicesChanged();

private slots:
    void refreshDevices();

private:
    std::shared_ptr<core::Core> m_coreClient;
    QMediaDevices m_mediaDevices;
};
