#pragma once

#include "utils/mousehook.h"
#include "utils/uiinspector.h"

#include <QImage>
#include <QPixmap>
#include <QWidget>

class ScreenshotWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenshotWindow(QWidget *parent = nullptr);
    ~ScreenshotWindow() override = default;

public slots:
    void start();
    void quit();

protected:
    void paintEvent(QPaintEvent *event) override;

    void captureFullScreens();

private slots:
    void onMouseMove(const POINT &pt);
    void onLButtonDown(const POINT &pt);
    void onLButtonUp(const POINT &pt);
    void onRButtonDown(const POINT &pt);
    void onRButtonUp(const POINT &pt);

private:
    QImage m_screenShot;
    QRect  m_highlightRect;

    MouseHook   m_mouseHook;
    UIInspector m_inspector;
};
