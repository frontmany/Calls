#pragma once

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QGraphicsDropShadowEffect>
#include <QPixmap>

#include "widgets/components/screen.h"

class MeetingParticipantWidget : public QWidget
{
    Q_OBJECT

public:
    enum class DisplayMode {
        DisplayName,
        DisplayCamera
    };

    explicit MeetingParticipantWidget(const QString& nickname, QWidget* parent = nullptr);
    ~MeetingParticipantWidget() = default;

    QString getNickname() const { return m_nickname; }

    DisplayMode getDisplayMode() const { return m_displayMode; }
    void setDisplayMode(DisplayMode mode);

    void updateVideoFrame(const QPixmap& frame);
    void clearVideoFrame();
    void setCameraEnabled(bool enabled);
    void setCompactSize(bool compact);

    void setMuted(bool muted) { m_muted = muted; }
    void setSpeaking(bool speaking) { m_speaking = speaking; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void setupUI();
    void updateAppearance();

    QString m_nickname;
    bool m_muted = false;
    bool m_speaking = false;
    bool m_cameraEnabled = false;
    bool m_compactMode = false;

    DisplayMode m_displayMode = DisplayMode::DisplayName;

    Screen* m_videoScreen = nullptr;
    QLabel* m_nameLabel = nullptr;
    QGridLayout* m_mainLayout = nullptr;
};
