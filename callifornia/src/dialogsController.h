#pragma once
#include <QLabel>
#include <QWidget>

class OverlayWidget;

class DialogsController : public QObject
{
	Q_OBJECT

public:
	explicit DialogsController(QWidget* parent);
	~DialogsController();

    void showUpdatingDialog();
    void hideUpdatingDialog();
	
	void showConnectionErrorDialog();
	void hideUpdatingErrorDialog();

    void updateLoadingProgress(double progress);
	void swapUpdatingToRestarting();
	void swapUpdatingToUpToDate();

signals:
	void exitButtonClicked();

private:
    QWidget* createUpdatingDialog(OverlayWidget* overlay);
    QWidget* createConnectionErrorDialog(OverlayWidget* overlay);

private:
    QWidget* m_parent;

    OverlayWidget* m_updatingOverlay;
    QWidget* m_updatingDialog;
	QLabel* m_updatingProgressLabel;
	QLabel* m_updatingLabel;
	QLabel* m_updatingGifLabel;

    OverlayWidget* m_connectionErrorOverlay;
    QWidget* m_connectionErrorDialog;
};

