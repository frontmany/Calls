#pragma once
#include <QDialog>
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

signals:
	void exitButtonClicked();

private:
	QDialog* createUpdatingDialog(OverlayWidget* overlay);
	QDialog* createConnectionErrorDialog(OverlayWidget* overlay);

private:
	QWidget* m_parent;
	
	OverlayWidget* m_updatingOverlay;
	QDialog* m_updatingDialog;
	QLabel* m_updatingProgressLabel;
	QLabel* m_updatingLabel;
	QLabel* m_updatingGifLabel;
	
	OverlayWidget* m_connectionErrorOverlay;
	QDialog* m_connectionErrorDialog;
};

