#ifndef SCREENCAPTURE_H
#define SCREENCAPTURE_H

#include "uiautomation.h"
#include <QPixmap>
#include <QScreen>
#include <QWidget>

class ScreenCapture : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenCapture(QWidget *parent = nullptr);
    void start();

signals:
    void captureCompleted(const QPixmap &pixmap);
    void captureCanceled();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QPixmap       fullScreenPixmap;
    QRect         highlightRect;
    UIAutomation *automation;
    QPoint        lastPos;

    void  updateHighlightRect(const QPoint &pos);
    QRect getHighlightWindow(const QPoint &pos);
    QRect getControlRect(HWND hwnd, const QPoint &pos);
};

#endif // SCREENCAPTURE_H
