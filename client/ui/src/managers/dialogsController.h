#pragma once
#include <QLabel>

class OverlayWidget;
class QGridLayout;
class QPushButton;
class QScreen;
class ScreenPreviewWidget;

class DialogsController : public QObject
{
	Q_OBJECT

public:
	explicit DialogsController(QWidget* parent);
	~DialogsController();

    void showUpdatingDialog();
    void hideUpdatingDialog();
    void setUpdateLoadingProgress(double progress);
    void setUpdateDialogStatus(const QString& statusText, bool hideProgress = true);
    void swapUpdatingToRestarting();
    void swapUpdatingToUpToDate();

    void showScreenShareDialog(const QList<QScreen*>& screens);
    void hideScreenShareDialog();
	
	void showConnectionErrorDialog();
	void hideUpdatingErrorDialog();

    void showAlreadyRunningDialog();
    void hideAlreadyRunningDialog();

    void showFirstLaunchDialog(const QString& imagePath = ":/resources/welcome.jpg", const QString& descriptionText = "");
    void hideFirstLaunchDialog();

signals:
	void closeRequested();
    void screenSelected(int screenIndex);
    void screenShareDialogCancelled();

private slots:
    void onScreenShareButtonClicked();

private:
    QWidget* createAlreadyRunningDialog(OverlayWidget* overlay);
    QWidget* createUpdatingDialog(OverlayWidget* overlay);
    QWidget* createConnectionErrorDialog(OverlayWidget* overlay);
    QWidget* createScreenShareDialog(OverlayWidget* overlay);
    QWidget* createFirstLaunchDialog(OverlayWidget* overlay, const QString& imagePath = "", const QString& descriptionText = "");
    void refreshScreenSharePreviews();
    void handleScreenPreviewClick(int screenIndex, bool currentlySelected);
    void updateScreenShareSelectionState();

private:
    QWidget* m_parent;

    OverlayWidget* m_updatingOverlay;
    QWidget* m_updatingDialog;
	QLabel* m_updatingProgressLabel;
	QLabel* m_updatingLabel;
	QLabel* m_updatingGifLabel;

    OverlayWidget* m_connectionErrorOverlay;
    QWidget* m_connectionErrorDialog;

    OverlayWidget* m_screenShareOverlay;
    QWidget* m_screenShareDialog;
    QWidget* m_screenShareScreensContainer;
    QGridLayout* m_screenShareScreensLayout;
    QPushButton* m_screenShareButton;
    QLabel* m_screenShareStatusLabel;
    QList<ScreenPreviewWidget*> m_screenSharePreviewWidgets;
    QList<QScreen*> m_screenShareScreens;
    int m_screenShareSelectedIndex;

    OverlayWidget* m_alreadyRunningOverlay;
    QWidget* m_alreadyRunningDialog;
    QLabel* m_alreadyRunningImageLabel;
    QLabel* m_alreadyRunningTitleLabel;
    QLabel* m_alreadyRunningMessageLabel;

    OverlayWidget* m_firstLaunchOverlay;
    QWidget* m_firstLaunchDialog;
    QLabel* m_firstLaunchImageLabel;
    QLabel* m_firstLaunchDescriptionLabel;
    QPushButton* m_firstLaunchOkButton;
};

