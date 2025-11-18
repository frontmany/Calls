#pragma once
#include <QLabel>
#include <QList>
#include <QPointer>
#include <QWidget>

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

    void showScreenShareDialog(const QList<QScreen*>& screens);
    void hideScreenShareDialog();
	
	void showConnectionErrorDialog();
	void hideUpdatingErrorDialog();

    void updateLoadingProgress(double progress);
	void swapUpdatingToRestarting();
	void swapUpdatingToUpToDate();

signals:
	void exitButtonClicked();
    void screenSelected(int screenIndex);

private slots:
    void onScreenShareButtonClicked();

private:
    QWidget* createUpdatingDialog(OverlayWidget* overlay);
    QWidget* createConnectionErrorDialog(OverlayWidget* overlay);
    QWidget* createScreenShareDialog(OverlayWidget* overlay);
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
};

