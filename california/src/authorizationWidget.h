#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QString>
#include <QLayout>
#include <QRegularExpression>
#include <QGraphicsBlurEffect>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QPixmap>
#include <QPainter>
#include <QGraphicsDropShadowEffect>

struct StyleAuthorizationWidget {
    // Color constants
    static const QColor m_primaryColor;
    static const QColor m_hoverColor;
    static const QColor m_errorColor;
    static const QColor m_successColor;
    static const QColor m_textColor;
    static const QColor m_backgroundColor;
    static const QColor m_glassColor;
    static const QColor m_glassBorderColor;
    static const QColor m_textDarkColor;
    static const QColor m_disabledColor;

    // Style methods
    static QString glassButtonStyle();
    static QString glassButtonHoverStyle();
    static QString glassButtonDisabledStyle();
    static QString glassLineEditStyle();
    static QString glassLabelStyle();
    static QString glassErrorLabelStyle();
    static QString glassTitleLabelStyle();
    static QString glassSubTitleLabelStyle();
};

class AuthorizationWidget : public QWidget {
    Q_OBJECT

public:
    AuthorizationWidget(QWidget* parent = nullptr);
    void setErrorMessage(const QString& errorText);
    void clearErrorMessage();
    void reset();

private slots:
    void onAuthorizeClicked();
    void onTextChanged(const QString& text);

private:
    void setupUI();
    void setupAnimations();
    void paintEvent(QPaintEvent* event) override;
    bool validateNickname(const QString& nickname);

    QWidget* m_container;
    QVBoxLayout* m_mainLayout;
    QVBoxLayout* m_glassLayout;
    QLabel* m_titleLabel;
    QLabel* m_subtitleLabel;
    QLabel* m_errorLabel;
    QLineEdit* m_nicknameEdit;
    QPushButton* m_authorizeButton;

    QGraphicsBlurEffect* m_backgroundBlurEffect;
    QPropertyAnimation* m_blurAnimation;
};