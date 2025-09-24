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
    static const QColor m_containerColor;
    static const QColor m_onlineColor;
    static const QColor m_offlineColor;
    static const QColor m_settingsButtonColor;

    static QString nicknameStyle();
    static QString scrollAreaStyle();
    static QString settingsButtonStyle();
    static QString containerStyle();
    static QString titleStyle();
    static QString buttonStyle();
    static QString buttonHoverStyle();
    static QString volumeValueStyle();
    static QString volumeLabelStyle();
    static QString lineEditStyle();
    static QString avatarStyle(const QColor& color);
    static QString incomingCallWidgetStyle();
    static QString settingsPanelStyle();
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

signals:
    void callRequested(const QString& friendNickname);
    void callAccepted(const QString& friendNickname);
    void callDeclined(const QString& friendNickname);
    void logoutRequested();
    void inputVolumeChanged(int volume);
    void outputVolumeChanged(int volume);

private slots:
    void onRefreshAudioDevicesClicked();
    void onCallButtonClicked();
    void onSettingsButtonClicked();
    void onIncomingCallAccepted(const QString& friendNickname);
    void onIncomingCallDeclined(const QString& friendNickname);
    //void onCallWidgetAnimationFinished(); // Новый слот

private:
    void setupAudioControlsUI();
    void showIncomingCallsArea();
    void hideIncomingCallsArea();
    void setupUI();
    void setupAnimations();
    void setupContainerShadow();
    void paintEvent(QPaintEvent* event) override;
    QColor generateRandomColor(const QString& seed);

    // Main layouts
    QVBoxLayout* m_mainLayout;
    QWidget* m_mainContainer;
    QVBoxLayout* m_containerLayout;

    // Header section
    QLabel* m_titleLabel;
    QWidget* m_userInfoWidget;
    QHBoxLayout* m_userInfoLayout;
    QLabel* m_avatarLabel;
    QVBoxLayout* m_userTextLayout;
    QLabel* m_nicknameLabel;
    QLabel* m_statusLabel;

    // Call section
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
    QGraphicsBlurEffect* m_blurEffect;

    QString m_currentNickname;
};