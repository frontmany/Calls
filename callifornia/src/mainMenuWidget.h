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

#include "state.h"
#include "incomingCallWidget.h"
#include "settingsPanelWidget.h"

struct StyleMainMenuWidget {
    static const QColor m_primaryColor;
    static const QColor m_selectionColor;
    static const QColor m_hoverColor;
    static const QColor m_backgroundColor;
    static const QColor m_textColor;
    static const QColor m_onlineColor;
    static const QColor m_offlineColor;
    static const QColor m_callingColor;
    static const QColor m_errorColor;
    static const QColor m_disabledColor;
    static const QColor m_settingsButtonColor;
    static const QColor m_settingsButtonHoverColor;
    static const QColor m_lineEditBackgroundColor;
    static const QColor m_lineEditFocusBackgroundColor;
    static const QColor m_placeholderColor;
    static const QColor m_callingSectionBackgroundColor;
    static const QColor m_stopCallingButtonColor;
    static const QColor m_stopCallingButtonHoverColor;
    static const QColor m_incomingCallBackgroundColor;
    static const QColor m_settingsPanelBackgroundColor;
    static const QColor m_scrollBarColor;
    static const QColor m_scrollBarHoverColor;
    static const QColor m_scrollBarPressedColor;

    static QString containerStyle();
    static QString titleStyle();
    static QString nicknameStyle();
    static QString buttonStyle();
    static QString disabledButtonStyle();
    static QString settingsButtonStyle();
    static QString lineEditStyle();
    static QString disabledLineEditStyle();
    static QString avatarStyle(const QColor& color);
    static QString scrollAreaStyle();
    static QString incomingCallWidgetStyle();
    static QString settingsPanelStyle();
    static QString callingSectionStyle();
    static QString callingTextStyle();
    static QString errorLabelStyle();
    static QString incomingCallsHeaderStyle();
    static QString stopCallingButtonStyle();
    static QString stopCallingButtonHoverStyle();
    static QString notificationRedLabelStyle();
};

class MainMenuWidget : public QWidget {
    Q_OBJECT

public:
    MainMenuWidget(QWidget* parent = nullptr);
    void setNickname(const QString& nickname);
    void setState(calls::State state);
    void addIncomingCall(const QString& friendNickname, int remainingTime = 32);
    void removeIncomingCall(const QString& friendNickname);
    void clearIncomingCalls();
    void showCallingPanel(const QString& friendNickname);
    void showErrorNotification(const QString& text, int durationMs);
    void removeCallingPanel();
    void setErrorMessage(const QString& errorText);
    void clearErrorMessage();
    void setFocusToLineEdit();

    std::vector<std::pair<std::string, int>> getIncomingCalls() const;

    void setInputVolume(int volume);
    void setOutputVolume(int volume);
    void setMicrophoneMuted(bool muted);
    void setSpeakerMuted(bool muted);

signals:
    void startCallingButtonClicked(const QString& friendNickname);
    void declineCallButtonClicked(const QString& friendNickname);
    void acceptCallButtonClicked(const QString& friendNickname);
    void stopCallingButtonClicked();
    void refreshAudioDevicesButtonClicked();
    void inputVolumeChanged(int newVolume);
    void outputVolumeChanged(int newVolume);
    void muteMicrophoneClicked(bool mute);
    void muteSpeakerClicked(bool mute);

private slots:
    void onCallButtonClicked();
    void onSettingsButtonClicked();
    void onIncomingCallAccepted(const QString& friendNickname);
    void onIncomingCallDeclined(const QString& friendNickname);
    void onStopCallingButtonClicked();

private:
    void showIncomingCallsArea();
    void hideIncomingCallsArea();
    void setupUI();
    void setupAnimations();
    void paintEvent(QPaintEvent* event) override;
    QColor generateRandomColor(const QString& seed);
    void updateCallingState(bool calling);

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

    // Calling section
    QWidget* m_callingSection;
    QHBoxLayout* m_callingLayout;
    QLabel* m_callingText;
    QPushButton* m_stopCallingButton;

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


    QWidget* m_notificationWidget;
    QHBoxLayout* m_notificationLayout;
    QLabel* m_notificationLabel;
    QTimer* m_notificationTimer;

    // Animations
    QPropertyAnimation* m_settingsAnimation;
    QPropertyAnimation* m_incomingCallsAnimation;
    QPropertyAnimation* m_callingAnimation;

    QString m_currentNickname;
    QString m_callingFriend;
};