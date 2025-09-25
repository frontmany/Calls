#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QString>
#include <QLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QScrollArea>
#include <QFrame>
#include <QPropertyAnimation>
#include <QGraphicsBlurEffect>
#include <QSpacerItem>

#include "calls.h"
#include "incomingCallWidget.h"
#include "settingsPanelWidget.h"

struct StyleMainMenuWidget {
    static const QColor m_primaryColor;
    static const QColor m_hoverColor;
    static const QColor m_backgroundColor;
    static const QColor m_textColor;
    static const QColor m_onlineColor;
    static const QColor m_offlineColor;
    static const QColor m_callingColor;
    static const QColor m_errorColor;

    static QString containerStyle();
    static QString titleStyle();
    static QString nicknameStyle();
    static QString buttonStyle();
    static QString settingsButtonStyle();
    static QString lineEditStyle();
    static QString avatarStyle(const QColor& color);
    static QString scrollAreaStyle();
    static QString incomingCallWidgetStyle();
    static QString settingsPanelStyle();
    static QString callingSectionStyle();
    static QString callingTextStyle();
    static QString errorLabelStyle();
};

class MainMenuWidget : public QWidget {
    Q_OBJECT

public:
    MainMenuWidget(QWidget* parent = nullptr);
    void setNickname(const QString& nickname);
    void setStatus(calls::ClientStatus status);
    void addIncomingCall(const QString& friendNickname);
    void removeIncomingCall(const QString& friendNickname);
    void clearIncomingCalls();
    void setCallingInfo(const QString& friendNickname);
    void clearCallingInfo();

    void setInputVolume(int volume);
    void setOutputVolume(int volume);
    void setMuted(bool muted);

signals:
    void callRequested(const QString& friendNickname);
    void callAccepted(const QString& friendNickname);
    void callDeclined(const QString& friendNickname);

private slots:
    void onCallButtonClicked();
    void onSettingsButtonClicked();
    void onIncomingCallAccepted(const QString& friendNickname);
    void onIncomingCallDeclined(const QString& friendNickname);
    void onCancelCallClicked();

private:
    void showIncomingCallsArea();
    void hideIncomingCallsArea();
    void setupUI();
    void setupAnimations();
    void paintEvent(QPaintEvent* event) override;
    QColor generateRandomColor(const QString& seed);
    void updateCallingState(bool calling);
    void setErrorMessage(const QString& errorText);
    void clearErrorMessage();

    // Main layouts
    QVBoxLayout* m_mainLayout;
    QWidget* m_mainContainer;
    QVBoxLayout* m_containerLayout;
    QPixmap m_backgroundTexture;

    // Header section
    QLabel* m_titleLabel;
    QWidget* m_userInfoWidget;
    QHBoxLayout* m_userInfoLayout;
    QLabel* m_avatarLabel;
    QVBoxLayout* m_userTextLayout;
    QLabel* m_nicknameLabel;
    QLabel* m_statusLabel;

    // Calling section (new)
    QWidget* m_callingSection;
    QHBoxLayout* m_callingLayout;
    QLabel* m_callingText;
    QPushButton* m_cancelCallButton;

    // Call section
    QLabel* m_errorLabel;
    QLineEdit* m_friendNicknameEdit;
    QPushButton* m_callButton;

    // Incoming calls section
    QScrollArea* m_incomingCallsScrollArea;
    QWidget* m_incomingCallsContainer;
    QVBoxLayout* m_incomingCallsLayout;

    // Settings section
    QPushButton* m_settingsButton;
    SettingsPanel* m_settingsPanel;

    // Animations
    QPropertyAnimation* m_settingsAnimation;
    QPropertyAnimation* m_incomingCallsAnimation;
    QPropertyAnimation* m_callingAnimation;

    QString m_currentNickname;
    QString m_callingFriend;
};