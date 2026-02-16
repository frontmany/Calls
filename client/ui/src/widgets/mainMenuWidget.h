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
#include "settingsPanelWidget.h"

struct StyleMainMenuWidget {
    static const QColor m_primaryColor;
    static const QColor m_selectionColor;
    static const QColor m_hoverColor;
    static const QColor m_backgroundColor;
    static const QColor m_textColor;
    static const QColor m_onlineColor;
    static const QColor m_offlineColor;
    static const QColor m_outgoingCallColor;
    static const QColor m_errorColor;
    static const QColor m_disabledColor;
    static const QColor m_settingsButtonColor;
    static const QColor m_settingsButtonHoverColor;
    static const QColor m_lineEditBackgroundColor;
    static const QColor m_lineEditFocusBackgroundColor;
    static const QColor m_placeholderColor;
    static const QColor m_outgoingCallSectionBackgroundColor;
    static const QColor m_stopOutgoingCallButtonColor;
    static const QColor m_stopOutgoingCallButtonHoverColor;
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
    static QString avatarStyle();
    static QString scrollAreaStyle();
    static QString incomingCallWidgetStyle();
    static QString settingsPanelStyle();
    static QString outgoingCallSectionStyle();
    static QString outgoingCallTextStyle();
    static QString errorLabelStyle();
    static QString incomingCallsHeaderStyle();
    static QString stopOutgoingCallButtonStyle();
    static QString stopOutgoingCallButtonHoverStyle();
    static QString disabledStopOutgoingCallButtonStyle();
    static QString notificationRedLabelStyle();
    static QString notificationBlueLabelStyle();
};

class MainMenuWidget : public QWidget {
    Q_OBJECT

public:
    MainMenuWidget(QWidget* parent = nullptr);
    void setNickname(const QString& nickname);
    void setStatusLabelOnline();
    void setStatusLabelOutgoingCall();
    void setStatusLabelBusy();
    void setStatusLabelOffline();
    void showOutgoingCallPanel(const QString& friendNickname);
    void removeOutgoingCallPanel();
    void setErrorMessage(const QString& errorText);
    void clearErrorMessage();
    void setFocusToLineEdit();

    void setInputVolume(int volume);
    void setOutputVolume(int volume);
    void setMicrophoneMuted(bool muted);
    void setSpeakerMuted(bool muted);
    void setCameraActive(bool active);
    void setCallButtonEnabled(bool enabled);
    void setStopOutgoingCallButtonEnabled(bool enabled);


signals:
    void startOutgoingCallButtonClicked(const QString& friendNickname);
    void stopOutgoingCallButtonClicked();
    void audioDevicePickerRequested();
    void inputVolumeChanged(int newVolume);
    void outputVolumeChanged(int newVolume);
    void muteMicrophoneClicked(bool mute);
    void muteSpeakerClicked(bool mute);
    void activateCameraClicked(bool activateD);

private slots:
    void onCallButtonClicked();
    void onSettingsButtonClicked();
    void onStopOutgoingCallButtonClicked();

private:
    void setupUI();
    void setupAnimations();
    void paintEvent(QPaintEvent* event) override;
    void updateOutgoingCallState(bool outgoingCall);

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

    // Outgoing call section
    QWidget* m_outgoingCallSection;
    QHBoxLayout* m_outgoingCallLayout;
    QLabel* m_outgoingCallText;
    QPushButton* m_stopOutgoingCallButton;

    // Call section
    QLabel* m_errorLabel;
    QLineEdit* m_friendNicknameEdit;
    QPushButton* m_callButton;

    // Settings section
    QPushButton* m_settingsButton;
    SettingsPanel* m_settingsPanel;

    // Animations
    QPropertyAnimation* m_settingsAnimation;
    QPropertyAnimation* m_outgoingCallAnimation;

    QString m_currentNickname;
    QString m_outgoingCallFriend;
};
