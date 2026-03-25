#pragma once

#include <QWidget>
#include <QLabel>

class QGraphicsDropShadowEffect;

class CallParticipantWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CallParticipantWidget(QWidget* parent = nullptr);

    void setNickname(const QString& nickname);
    void setTimerText(const QString& timerText);
    void setTimerLongFormat(bool longFormat);
    void setSpeaking(bool speaking);
    void setMuted(bool muted);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void applyShadowForSpeaking(bool speaking);

    QLabel* m_timerLabel = nullptr;
    QLabel* m_nicknameLabel = nullptr;
    QLabel* m_mutedLabel = nullptr;
    QGraphicsDropShadowEffect* m_shadowEffect = nullptr;
    bool m_speaking = false;
    bool m_muted = false;
};
