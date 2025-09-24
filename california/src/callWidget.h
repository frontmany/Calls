#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QTime>

struct StyleCallWidget {
    static const QColor m_primaryColor;
    static const QColor m_hoverColor;
    static const QColor m_backgroundColor;
    static const QColor m_textColor;
    static const QColor m_containerColor;

    static QString containerStyle();
    static QString titleStyle();
    static QString timerStyle();
    static QString controlButtonStyle();
    static QString controlButtonHoverStyle();
    static QString hangupButtonStyle();
    static QString hangupButtonHoverStyle();
    static QString panelStyle();
};

class CallWidget : public QWidget {
    Q_OBJECT

public:
    CallWidget(QWidget* parent = nullptr);
    ~CallWidget() = default;

    void setCallInfo(const QString& friendNickname);
    void updateTimer();

signals:
    void muteMicClicked();
    void muteSpeakerClicked();
    void hangupClicked();

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onMuteMicClicked();
    void onMuteSpeakerClicked();
    void onHangupClicked();
    void updateCallTimer();

private:
    void setupUI();
    QColor generateRandomColor(const QString& seed);

    // Main layouts
    QVBoxLayout* m_mainLayout;
    QWidget* m_mainContainer;
    QVBoxLayout* m_containerLayout;

    // Call info section
    QLabel* m_titleLabel;
    QLabel* m_timerLabel;
    QLabel* m_friendNicknameLabel;

    // Control panel
    QWidget* m_controlPanel;
    QHBoxLayout* m_controlLayout;
    QPushButton* m_muteMicButton;
    QPushButton* m_muteSpeakerButton;
    QPushButton* m_hangupButton;

    // Timer
    QTimer* m_callTimer;
    QTime m_callDuration;
    QString m_friendNickname;
};
