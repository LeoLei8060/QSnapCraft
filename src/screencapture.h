#ifndef SCREENCAPTURE_H
#define SCREENCAPTURE_H

#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QColor>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class ScreenCapture : public QWidget
{
    Q_OBJECT
public:
    explicit ScreenCapture(QWidget *parent = nullptr);
    void startCapture();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void initScreenshot();
    void saveScreenshot(const QPixmap &screenshot);
    QRect getTargetRect(const QPoint &pos);
    QColor getColorAt(const QPoint &pos);
    void drawMagnifier(QPainter &painter, const QPoint &pos);
    
#ifdef Q_OS_WIN
    HWND getWindowFromPoint(const QPoint &pos);
    QRect getWindowRect(HWND hwnd);
#endif
    
    QPixmap fullScreenPixmap;
    QRect selectedRect;
    QPoint currentPos;
    QColor currentColor;
    bool showMagnifier;
};

#endif // SCREENCAPTURE_H
