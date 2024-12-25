#include "screencapture.h"
#include <QApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>

ScreenCapture::ScreenCapture(QWidget *parent)
    : QWidget(nullptr)
    , automation(new UIAutomation())
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_MouseTracking);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
}

void ScreenCapture::start()
{
    QScreen *screen = QApplication::primaryScreen();
    fullScreenPixmap = screen->grabWindow(0);

    showFullScreen();
}

void ScreenCapture::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(rect(), QColor(0, 0, 0, 128));

    if (!highlightRect.isNull()) {
        painter.drawPixmap(highlightRect, fullScreenPixmap, highlightRect);
        painter.setPen(QPen(Qt::red, 2));
        painter.drawRect(highlightRect);
    }
}

void ScreenCapture::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (!highlightRect.isNull()) {
            QPixmap capturedPixmap = fullScreenPixmap.copy(highlightRect);
            emit    captureCompleted(capturedPixmap);
        }
        close();
    } else if (event->button() == Qt::RightButton) {
        emit captureCanceled();
        close();
    }
}

void ScreenCapture::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    if (pos != lastPos) {
        qDebug() << __FUNCTION__;
        updateHighlightRect(pos);
        lastPos = pos;
    }
}

void ScreenCapture::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit captureCanceled();
        close();
    }
}

void ScreenCapture::updateHighlightRect(const QPoint &pos)
{
    QRect newRect = getHighlightWindow(pos);
    if (newRect != highlightRect) {
        highlightRect = newRect;
        update();
    }
}

QString GetWindowTitle(HWND hwnd)
{
    char title[256];
    GetWindowTextA(hwnd, title, sizeof(title));
    return QString(title);
}

QRect ScreenCapture::getHighlightWindow(const QPoint &pos)
{
    DWORD selfID = GetCurrentProcessId();
    HWND  hwnd = GetTopWindow(nullptr);
    while (hwnd != nullptr) {
        if (IsWindow(hwnd) && IsWindowVisible(hwnd)) {
            DWORD winID;
            GetWindowThreadProcessId(hwnd, &winID);
            if (winID != selfID) {
                RECT windowRect;
                GetWindowRect(hwnd, &windowRect);
                QRect windowQRc = QRect(windowRect.left,
                                        windowRect.top,
                                        windowRect.right - windowRect.left,
                                        windowRect.bottom - windowRect.top);
                if (windowQRc.contains(pos)) {
                    qDebug() << "window: " << GetWindowTitle(hwnd);
                    QRect result = getControlRect(hwnd, pos);
                    return result.isEmpty() ? windowQRc : result;
                }
            }
        }
        hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
    }
}

struct EnumWindowsData
{
    QPoint        pos;
    HWND          selfHwnd;
    QRect         resultRect;
    UIAutomation *automation;
    bool          found;
};

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    EnumWindowsData *data = reinterpret_cast<EnumWindowsData *>(lParam);

    if (hwnd == data->selfHwnd || !IsWindowVisible(hwnd)) {
        return TRUE;
    }

    RECT rect;
    GetWindowRect(hwnd, &rect);
    QRect windowRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);

    if (windowRect.contains(data->pos)) {
        wchar_t title[256];
        GetWindowTextW(hwnd, title, 256);
        qDebug() << "window:" << QString::fromWCharArray(title);

        WindowInfo info = data->automation->getWindowControls(hwnd, data->pos);

        if (!info.controls.empty()) {
            qDebug() << "control count:" << info.controls.size()
                     << "control:" << info.controls[info.controls.size() - 1].name;
            data->resultRect = info.controls[info.controls.size() - 1].bounds;
        } else {
            data->resultRect = windowRect;
        }

        data->found = true;
        return FALSE;
    }
    return TRUE;
}

QRect ScreenCapture::getControlRect(HWND hwnd, const QPoint &pos)
{
    WindowInfo info = automation->getWindowControls(hwnd, pos);
    if (!info.controls.empty()) {
        qDebug() << "control count:" << info.controls.size()
                 << "control:" << info.controls[info.controls.size() - 1].name;
        return info.controls[info.controls.size() - 1].bounds;
    } else {
        return QRect();
    }

    //    EnumWindowsData data;
    //    data.pos = pos;
    //    data.selfHwnd = (HWND) this->winId();
    //    data.automation = automation;
    //    data.found = false;

    //    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));

    //    return data.found ? data.resultRect : QRect();
}
