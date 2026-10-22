#include "desktopcapture.h"

#include <QCursor>
#include <QGuiApplication>
#include <QPainter>
#include <QPixmap>
#include <QScreen>

#include <windows.h>

namespace {
class IconBitmap
{
public:
    explicit IconBitmap(HBITMAP bitmap)
        : m_bitmap(bitmap)
    {}

    ~IconBitmap()
    {
        if (m_bitmap != nullptr) {
            DeleteObject(m_bitmap);
        }
    }

    HBITMAP get() const { return m_bitmap; }

private:
    HBITMAP m_bitmap{nullptr};
};

bool currentCursorImage(QImage *image, QPoint *hotspot)
{
    CURSORINFO cursorInfo{};
    cursorInfo.cbSize = sizeof(cursorInfo);
    if (!GetCursorInfo(&cursorInfo) || (cursorInfo.flags & CURSOR_SHOWING) == 0) {
        return false;
    }

    ICONINFO iconInfo{};
    if (!GetIconInfo(cursorInfo.hCursor, &iconInfo)) {
        return false;
    }
    IconBitmap colorBitmap(iconInfo.hbmColor);
    IconBitmap maskBitmap(iconInfo.hbmMask);
    if (iconInfo.hbmColor == nullptr) {
        return false;
    }

    BITMAP bitmap{};
    if (!GetObject(iconInfo.hbmColor, sizeof(bitmap), &bitmap)) {
        return false;
    }

    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = bitmap.bmWidth;
    bitmapInfo.bmiHeader.biHeight = -bitmap.bmHeight;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    QImage cursorImage(bitmap.bmWidth, bitmap.bmHeight, QImage::Format_ARGB32_Premultiplied);
    HDC    screenDc = GetDC(nullptr);
    if (screenDc == nullptr) {
        return false;
    }
    const int copied
        = GetDIBits(screenDc, iconInfo.hbmColor, 0, bitmap.bmHeight, cursorImage.bits(), &bitmapInfo, DIB_RGB_COLORS);
    ReleaseDC(nullptr, screenDc);
    if (copied == 0) {
        return false;
    }

    *image = std::move(cursorImage);
    *hotspot = QPoint(static_cast<int>(iconInfo.xHotspot), static_cast<int>(iconInfo.yHotspot));
    return true;
}
} // namespace

CaptureFrame captureVirtualDesktop(bool includeCursor)
{
    CaptureFrame frame;

    const auto screens = QGuiApplication::screens();
    if (screens.isEmpty()) {
        frame.error = QStringLiteral("没有可用的显示器。");
        return frame;
    }

    for (QScreen *screen : screens) {
        if (screen == nullptr) {
            continue;
        }

        const QPixmap pixmap = screen->grabWindow(0);
        if (pixmap.isNull()) {
            frame.error = QStringLiteral("抓取显示器 %1 失败。").arg(screen->name());
            frame.displays.clear();
            frame.logicalComposite = {};
            return frame;
        }

        QImage nativeImage = pixmap.toImage();
        nativeImage.setDevicePixelRatio(1.0);
        if (nativeImage.isNull()) {
            frame.error = QStringLiteral("读取显示器 %1 像素失败。").arg(screen->name());
            frame.displays.clear();
            return frame;
        }

        const QRect logicalGeometry = screen->geometry();
        frame.logicalBounds = frame.logicalBounds.isNull() ? logicalGeometry
                                                           : frame.logicalBounds.united(logicalGeometry);
        frame.displays.append(DisplaySurface{
            screen->name(),
            logicalGeometry,
            std::move(nativeImage),
            screen->devicePixelRatio(),
        });
    }

    if (!frame.logicalBounds.isValid() || frame.displays.isEmpty()) {
        frame.error = QStringLiteral("无法确定虚拟桌面。");
        return frame;
    }

    if (includeCursor) {
        QImage cursorImage;
        QPoint hotspot;
        if (currentCursorImage(&cursorImage, &hotspot)) {
            const QPoint cursorPosition = QCursor::pos();
            for (DisplaySurface &surface : frame.displays) {
                if (!surface.logicalGeometry.contains(cursorPosition)) {
                    continue;
                }
                const qreal  scaleX = qreal(surface.nativeImage.width()) / surface.logicalGeometry.width();
                const qreal  scaleY = qreal(surface.nativeImage.height()) / surface.logicalGeometry.height();
                const QPoint point(qRound((cursorPosition.x() - surface.logicalGeometry.left()) * scaleX) - hotspot.x(),
                                   qRound((cursorPosition.y() - surface.logicalGeometry.top()) * scaleY) - hotspot.y());
                QPainter     cursorPainter(&surface.nativeImage);
                cursorPainter.drawImage(point, cursorImage);
                break;
            }
        }
    }

    frame.logicalComposite = QImage(frame.logicalBounds.size(), QImage::Format_ARGB32_Premultiplied);
    frame.logicalComposite.fill(Qt::black);

    QPainter painter(&frame.logicalComposite);
    for (const DisplaySurface &surface : frame.displays) {
        const QRect target(surface.logicalGeometry.topLeft() - frame.logicalBounds.topLeft(),
                           surface.logicalGeometry.size());
        painter.drawImage(target, surface.nativeImage, surface.nativeImage.rect());
    }
    painter.end();
    return frame;
}
