#pragma once

#include "utils/magnifier.h"
#include "utils/mousehook.h"
#include "utils/uiinspector.h"

#include <windows.h>
#include <QCursor>
#include <QImage>
#include <QPixmap>
#include <QWidget>

class ScreenshotWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenshotWindow(QWidget *parent = nullptr);
    ~ScreenshotWindow() override;

    void start(const QPixmap &pixmap,
               const QImage  &img,
               const int     &left,
               const int     &top,
               const int     &width,
               const int     &height); // 开始截图
    void quit();                   // 退出截图

    void showWindow();

    QPixmap getCaptureImage() const { return m_screenshotPixmap; }
    QRect   getCaptureRect() const { return m_shotRect; }

signals:
    void sigCompleteScreenshot();
    void sigCancelScreenshot();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    void   drawMagnifier(QPainter &painter, const QPoint &pos);
    void   drawInfoText(QPainter &painter, const QPoint &pos);
    QColor getColorAtPos(const QPoint &pos);

private slots:
    void onMouseMove(const POINT &pt);
    void onLButtonDown(const POINT &pt);
    void onLButtonUp(const POINT &pt);
    void onRButtonDown(const POINT &pt);
    void onRButtonUp(const POINT &pt);

private:
    void drawPositionText(QPainter *painter);
    void activateScreenCapture();
    void activateScreenEdit();
    void setSystemCursor();
    void restoreSystemCursor();

    QPixmap m_screenshotPixmap;
    QImage  m_screenshotImg;
    QRect   m_highlightRect;
    QRect   m_shotRect;
    QCursor m_originalCursor;
    bool    m_isActive{false}; // 截图工具是否激活
    bool    m_blockSetCursor;
    QPoint  m_dragStartPos;        // 拖拽起始位置
    bool    m_isDragging{false};   // 是否正在拖拽选择
    bool    m_smartInspect{false}; // 是否进行智能检测

    MouseHook   m_mouseHook;
    UIInspector m_inspector;
    Magnifier   m_magnifier; // 放大镜组件

    HCURSOR            m_crossCursor; // 截图窗口的鼠标样式
    static const DWORD CURSOR_IDS[];
    static const int   CURSOR_COUNT;

    static const int MAGNIFIER_SIZE;
    static const int MAGNIFIER_SCALE;
};
