#pragma once

#include <QWidget>
#include <QList>
#include <QLabel>
#include <QGridLayout>
#include <QPushButton>
#include <QScreen>

class QScrollArea;
class QResizeEvent;

class ScreenPreviewWidget;

struct StyleScreenShareDialog
{
    static QString mainWidgetStyle(int radius, int border);
    static QString titleStyle(int fontSize, int padding);
    static QString instructionStyle(int fontSize, int padding);
    static QString statusStyle(int fontSize, int padding, int radius);
    static QString shareButtonStyle(int radius, int paddingH, int paddingV, int fontSize);
    static QString closeButtonStyle(int radius, int paddingH, int paddingV, int fontSize);
    static QString scrollAreaStyle(int barWidth, int handleRadius, int handleMinHeight);
};

class ScreenShareDialog : public QWidget
{
    Q_OBJECT
public:
    explicit ScreenShareDialog(QWidget* parent = nullptr);
    void setScreens(const QList<QScreen*>& screens);

signals:
    void screenSelected(int index);
    void cancelled();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void refreshScreenSharePreviews();
    void handleScreenPreviewClick(int screenIndex, bool currentlySelected);
    void updateScreenShareSelectionState();

private:
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_screensContainer = nullptr;
    QGridLayout* m_screensLayout = nullptr;
    QPushButton* m_shareButton = nullptr;
    QLabel* m_statusLabel = nullptr;
    QList<ScreenPreviewWidget*> m_previewWidgets;
    QList<QScreen*> m_screens;
    int m_selectedIndex = -1;
};