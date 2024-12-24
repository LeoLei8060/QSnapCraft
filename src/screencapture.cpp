#include "screencapture.h"
#include <QScreen>
#include <QPainter>
#include <QApplication>
#include <QMouseEvent>
#include <QFileDialog>
#include <QDateTime>
#include <QDebug>
#include <QGuiApplication>

#ifdef Q_OS_WIN
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")

struct EnumWindowsData {
    POINT pt;
    HWND hwnd;
    HWND selfHwnd;
    bool foundWindow;
    std::vector<std::pair<HWND, RECT>> windowList; // 存储所有符合条件的窗口
    QString selfClassName;  // 存储自身窗口的类名
};

// 检查窗口是否可用于检测
static bool isWindowValid(HWND hwnd, const QString& selfClassName)
{
    if (!hwnd || !IsWindow(hwnd)) {
        return false;
    }
    
    // 检查窗口是否可见
    if (!IsWindowVisible(hwnd)) {
        return false;
    }
    
    // 检查窗口是否最小化
    WINDOWPLACEMENT placement;
    placement.length = sizeof(WINDOWPLACEMENT);
    if (GetWindowPlacement(hwnd, &placement)) {
        if (placement.showCmd == SW_SHOWMINIMIZED) {
            return false;
        }
    }

    // 获取窗口类名
    wchar_t className[256];
    GetClassNameW(hwnd, className, sizeof(className)/sizeof(wchar_t));
    QString windowClassName = QString::fromWCharArray(className);

    // 只排除自身窗口和Qt临时窗口
    if (windowClassName == selfClassName || 
        windowClassName.contains("Qt")) {
        qDebug() << "Skipping self or Qt window:" << windowClassName;
        return false;
    }
    
    return true;
}

// 获取窗口标题和类名
static QString getWindowInfo(HWND hwnd)
{
    if (!hwnd) return QString();
    
    wchar_t title[256];
    GetWindowTextW(hwnd, title, sizeof(title)/sizeof(wchar_t));
    
    wchar_t className[256];
    GetClassNameW(hwnd, className, sizeof(className)/sizeof(wchar_t));
    
    return QString("Window Title: %1 (Class: %2)")
        .arg(QString::fromWCharArray(title))
        .arg(QString::fromWCharArray(className));
}

// 获取窗口的Z序（数值越小越靠前）
static int GetWindowZOrder(HWND hwnd)
{
    int z = 0;
    for (HWND h = hwnd; h != NULL; h = GetWindow(h, GW_HWNDPREV)) {
        z++;
    }
    return z;
}

// 回调函数，用于EnumWindows
static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    auto* data = reinterpret_cast<EnumWindowsData*>(lParam);
    
    // 跳过自身窗口
    if (hwnd == data->selfHwnd) {
        return TRUE;
    }
    
    // 获取窗口区域
    RECT rect;
    if (!GetWindowRect(hwnd, &rect)) {
        return TRUE;
    }
    
    // 检查点是否在窗口内
    if (PtInRect(&rect, data->pt)) {
        // 检查窗口是否有效
        if (isWindowValid(hwnd, data->selfClassName)) {
            // 将符合条件的窗口添加到列表中
            data->windowList.push_back({hwnd, rect});
            qDebug() << "Found window:" << getWindowInfo(hwnd) << "Z-order:" << GetWindowZOrder(hwnd);
        }
    }
    
    return TRUE;  // 继续枚举所有窗口
}

HWND ScreenCapture::getWindowFromPoint(const QPoint &pos)
{
    POINT pt;
    pt.x = pos.x();
    pt.y = pos.y();
    
    // 初始化数据结构
    EnumWindowsData data;
    data.pt = pt;
    data.hwnd = NULL;
    data.selfHwnd = (HWND)winId();
    data.foundWindow = false;
    data.windowList.clear();
    
    // 获取自身窗口的类名
    wchar_t className[256];
    GetClassNameW(data.selfHwnd, className, sizeof(className)/sizeof(wchar_t));
    data.selfClassName = QString::fromWCharArray(className);
    qDebug() << "\n=== Window Detection at" << pos << "===";
    qDebug() << "Self window class name:" << data.selfClassName;
    
    // 枚举所有顶层窗口
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));
    
    // 如果没有找到任何窗口
    if (data.windowList.empty()) {
        qDebug() << "No valid window found at position:" << pos;
        return NULL;
    }
    
    // 根据Z序排序窗口列表（从前到后）
    std::sort(data.windowList.begin(), data.windowList.end(), 
        [](const auto& a, const auto& b) {
            return GetWindowZOrder(a.first) < GetWindowZOrder(b.first);
        });
    
    // 返回Z序最靠前的窗口
    HWND topWindow = data.windowList.front().first;
    qDebug() << "Selected window:" << getWindowInfo(topWindow);
    return topWindow;
}

QRect ScreenCapture::getWindowRect(HWND hwnd)
{
    if (!hwnd) {
        return QRect();
    }

    // 获取窗口类名
    wchar_t className[256];
    GetClassNameW(hwnd, className, sizeof(className)/sizeof(wchar_t));
    QString windowClassName = QString::fromWCharArray(className);
    qDebug() << "Getting rect for window class:" << windowClassName;

    RECT rect;
    if (!GetWindowRect(hwnd, &rect)) {
        return QRect();
    }

    // 获取窗口样式
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    
    // 如果是子窗口，转换坐标到屏幕坐标
    if (style & WS_CHILD) {
        POINT pt = {rect.left, rect.top};
        POINT pt2 = {rect.right, rect.bottom};
        HWND parent = GetParent(hwnd);
        if (parent) {
            // 获取父窗口的客户区域偏移
            POINT parentOffset = {0, 0};
            ClientToScreen(parent, &parentOffset);
            
            // 将子窗口坐标转换为屏幕坐标
            MapWindowPoints(hwnd, parent, (LPPOINT)&rect, 2);
            rect.left += parentOffset.x;
            rect.top += parentOffset.y;
            rect.right += parentOffset.x;
            rect.bottom += parentOffset.y;
        }
    }
    // 如果是顶层窗口，尝试获取DWM边框
    else {
        RECT frame = rect;
        if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(RECT)))) {
            rect = frame;
        }
    }

    QRect result(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
    qDebug() << "Window rect:" << result;
    return result;
}

#endif

ScreenCapture::ScreenCapture(QWidget *parent)
    : QWidget(parent)
    , showMagnifier(true)
{
    setWindowFlags(Qt::FramelessWindowHint | 
                  Qt::WindowStaysOnTopHint | 
                  Qt::X11BypassWindowManagerHint |
                  Qt::Tool);
    
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::CrossCursor);
    setWindowTitle("QSnapCraft");
}

QRect ScreenCapture::getTargetRect(const QPoint &pos)
{
#ifdef Q_OS_WIN
    qDebug() << "\n--- Window Detection at" << pos << "---";
    HWND hwnd = getWindowFromPoint(pos);
    if (hwnd) {
        return getWindowRect(hwnd);
    }
#endif
    return QRect();
}

void ScreenCapture::initScreenshot()
{
    // 获取主屏幕
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return;
    
    // 获取所有屏幕
    QList<QScreen *> screens = QGuiApplication::screens();
    
    // 计算所有屏幕的总区域
    QRect totalGeometry;
    for (QScreen *screen : screens) {
        totalGeometry = totalGeometry.united(screen->geometry());
    }
    
    // 设置窗口大小为所有屏幕的总大小
    setGeometry(totalGeometry);
    
    // 捕获所有屏幕的内容
    fullScreenPixmap = QPixmap(totalGeometry.size());
    fullScreenPixmap.fill(Qt::transparent);
    
    QPainter painter(&fullScreenPixmap);
    for (QScreen *screen : screens) {
        QPixmap screenPixmap = screen->grabWindow(0);
        painter.drawPixmap(screen->geometry().topLeft(), screenPixmap);
    }
    
    // 确保窗口在最顶层
    raise();
    activateWindow();
}

void ScreenCapture::startCapture()
{
    // 隐藏主窗口
    if (parentWidget()) {
        parentWidget()->hide();
    }
    
    // 初始化截图
    initScreenshot();
    
    // 显示截图窗口
    showFullScreen();
    activateWindow();
    setFocus();
}

QColor ScreenCapture::getColorAt(const QPoint &pos)
{
    if (fullScreenPixmap.rect().contains(pos)) {
        return fullScreenPixmap.toImage().pixelColor(pos);
    }
    return QColor();
}

void ScreenCapture::drawMagnifier(QPainter &painter, const QPoint &pos)
{
    const int radius = 50;
    const int zoom = 2;
    
    // 创建放大镜区域
    QRect sourceRect(pos.x() - radius/zoom, pos.y() - radius/zoom, radius/zoom, radius/zoom);
    QRect targetRect(pos.x() + 20, pos.y() + 20, radius, radius);
    
    // 确保放大镜在屏幕内
    if (targetRect.right() > width()) {
        targetRect.moveLeft(pos.x() - targetRect.width() - 20);
    }
    if (targetRect.bottom() > height()) {
        targetRect.moveTop(pos.y() - targetRect.height() - 20);
    }
    
    // 绘制放大区域
    painter.save();
    painter.setBrush(Qt::white);
    painter.setPen(QPen(Qt::black, 2));
    painter.drawEllipse(targetRect);
    painter.setClipRegion(QRegion(targetRect, QRegion::Ellipse));
    
    // 绘制放大的图像
    painter.drawPixmap(targetRect, fullScreenPixmap, sourceRect);
    
    // 绘制十字线
    painter.setPen(QPen(Qt::red, 1));
    painter.drawLine(targetRect.center().x(), targetRect.top(), targetRect.center().x(), targetRect.bottom());
    painter.drawLine(targetRect.left(), targetRect.center().y(), targetRect.right(), targetRect.center().y());
    
    painter.restore();
}

void ScreenCapture::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.drawPixmap(0, 0, fullScreenPixmap);
    
    // 添加半透明蒙层
    QColor maskColor(0, 0, 0, 160);
    painter.fillRect(rect(), maskColor);
    
    if (!selectedRect.isEmpty()) {
        // 绘制选中的区域
        painter.drawPixmap(selectedRect, fullScreenPixmap, selectedRect);
        
        // 绘制边框
        QPen pen(Qt::red, 2);
        painter.setPen(pen);
        painter.drawRect(selectedRect);
        
        // 绘制尺寸信息
        QString sizeText = QString("%1 x %2").arg(selectedRect.width()).arg(selectedRect.height());
        if (currentColor.isValid()) {
            sizeText += QString(" | RGB(%1, %2, %3)").arg(currentColor.red())
                                                    .arg(currentColor.green())
                                                    .arg(currentColor.blue());
        }
        
        QFont font = painter.font();
        font.setBold(true);
        painter.setFont(font);
        
        QRect textRect = painter.boundingRect(selectedRect, Qt::AlignCenter, sizeText);
        QRect bgRect = textRect.adjusted(-10, -5, 10, 5);
        bgRect.moveTop(selectedRect.top() - bgRect.height() - 5);
        
        // 绘制文字背景
        painter.fillRect(bgRect, QColor(0, 0, 0, 200));
        painter.setPen(Qt::white);
        painter.drawText(bgRect, Qt::AlignCenter, sizeText);
    }
    
    // 绘制放大镜
    if (showMagnifier && !selectedRect.isEmpty()) {
        drawMagnifier(painter, currentPos);
    }
}

void ScreenCapture::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !selectedRect.isEmpty()) {
        QPixmap screenshot = fullScreenPixmap.copy(selectedRect);
        saveScreenshot(screenshot);
        hide();
    }
}

void ScreenCapture::mouseMoveEvent(QMouseEvent *event)
{
    currentPos = event->pos();
    selectedRect = getTargetRect(event->globalPos());
    currentColor = getColorAt(currentPos);
    update();
}

void ScreenCapture::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        // 关闭截图窗口
        close();
        
        // 显示主窗口
        if (parentWidget()) {
            parentWidget()->show();
            parentWidget()->raise();
            parentWidget()->activateWindow();
        }
    }
}

void ScreenCapture::saveScreenshot(const QPixmap &screenshot)
{
    if (screenshot.isNull())
        return;
        
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString filename = QFileDialog::getSaveFileName(
        nullptr,
        tr("Save Screenshot"),
        QDir::homePath() + "/screenshot_" + timestamp + ".png",
        tr("Images (*.png *.jpg)"));
        
    if (!filename.isEmpty()) {
        screenshot.save(filename);
    }
}
