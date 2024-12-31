#include "editor_window.h"
#include <windows.h>
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

EditorWindow::EditorWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Widget | Qt::WindowStaysOnTopHint);

    int screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int top = GetSystemMetrics(SM_YVIRTUALSCREEN);

    setGeometry(left, top, screenWidth, screenHeight);

    setMouseTracking(true);
}

void EditorWindow::setImage(const QImage &image)
{
    m_currentImage = image;
}

void EditorWindow::setData(const QImage &image, const QRect &captureRect)
{
    m_currentImage = image;
    m_captureRect = captureRect;
    qDebug() << m_captureRect;
    update();
}

void EditorWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);

    // 绘制背景截图
    if (m_captureRect.isValid()) {
        painter.drawImage(0, 0, m_currentImage);

        // 添加半透明遮罩
        QPainterPath windowPath;
        windowPath.addRect(rect());

        QPainterPath highlightPath;
        highlightPath.addRect(m_captureRect);

        QPainterPath overlayPath = windowPath.subtracted(highlightPath);

        painter.fillPath(overlayPath, QColor(0, 0, 0, 128));

        // 绘制选区边框
        painter.setPen(QPen(Qt::white, 2));
        painter.drawRect(m_captureRect);

        // 绘制调整手柄
        const int handleSize = 8;
        painter.setBrush(Qt::white);
        painter.setPen(Qt::black);

        // 绘制四个角落的手柄
        QVector<QPoint> corners = {m_captureRect.topLeft(),
                                   m_captureRect.topRight(),
                                   m_captureRect.bottomRight(),
                                   m_captureRect.bottomLeft()};

        for (const QPoint &corner : corners) {
            painter.drawRect(QRect(corner.x() - handleSize / 2,
                                   corner.y() - handleSize / 2,
                                   handleSize,
                                   handleSize));
        }

        // 绘制四边的手柄
        QPoint midPoints[] = {QPoint(m_captureRect.left(), m_captureRect.center().y()),
                              QPoint(m_captureRect.right(), m_captureRect.center().y()),
                              QPoint(m_captureRect.center().x(), m_captureRect.top()),
                              QPoint(m_captureRect.center().x(), m_captureRect.bottom())};

        for (const QPoint &point : midPoints) {
            painter.drawRect(QRect(point.x() - handleSize / 2,
                                   point.y() - handleSize / 2,
                                   handleSize,
                                   handleSize));
        }
    }

    // 绘制放大镜
    m_magnifier.paint(painter, m_currentImage, QCursor::pos());
}

EditorWindow::ResizeHandle EditorWindow::hitTest(const QPoint &pos) const
{
    if (!m_captureRect.isValid())
        return ResizeHandle::None;

    const int handleSize = 8;
    QRect     rect = m_captureRect;

    // 检查四个角落
    if (QRect(rect.topLeft() - QPoint(handleSize / 2, handleSize / 2), QSize(handleSize, handleSize))
            .contains(pos))
        return ResizeHandle::TopLeft;
    if (QRect(rect.topRight() - QPoint(-handleSize / 2, handleSize / 2),
              QSize(handleSize, handleSize))
            .contains(pos))
        return ResizeHandle::TopRight;
    if (QRect(rect.bottomRight() - QPoint(-handleSize / 2, -handleSize / 2),
              QSize(handleSize, handleSize))
            .contains(pos))
        return ResizeHandle::BottomRight;
    if (QRect(rect.bottomLeft() - QPoint(handleSize / 2, -handleSize / 2),
              QSize(handleSize, handleSize))
            .contains(pos))
        return ResizeHandle::BottomLeft;

    // 检查四条边
    if (QRect(rect.left() - handleSize / 2,
              rect.top() + handleSize,
              handleSize,
              rect.height() - 2 * handleSize)
            .contains(pos))
        return ResizeHandle::Left;
    if (QRect(rect.right() - handleSize / 2,
              rect.top() + handleSize,
              handleSize,
              rect.height() - 2 * handleSize)
            .contains(pos))
        return ResizeHandle::Right;
    if (QRect(rect.left() + handleSize,
              rect.top() - handleSize / 2,
              rect.width() - 2 * handleSize,
              handleSize)
            .contains(pos))
        return ResizeHandle::Top;
    if (QRect(rect.left() + handleSize,
              rect.bottom() - handleSize / 2,
              rect.width() - 2 * handleSize,
              handleSize)
            .contains(pos))
        return ResizeHandle::Bottom;

    // 检查是否在区域内部
    if (rect.contains(pos))
        return ResizeHandle::Move;

    return ResizeHandle::None;
}

void EditorWindow::updateCursor(ResizeHandle handle)
{
    switch (handle) {
    case ResizeHandle::TopLeft:
    case ResizeHandle::BottomRight:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case ResizeHandle::TopRight:
    case ResizeHandle::BottomLeft:
        setCursor(Qt::SizeBDiagCursor);
        break;
    case ResizeHandle::Left:
    case ResizeHandle::Right:
        setCursor(Qt::SizeHorCursor);
        break;
    case ResizeHandle::Top:
    case ResizeHandle::Bottom:
        setCursor(Qt::SizeVerCursor);
        break;
    case ResizeHandle::Move:
        setCursor(Qt::SizeAllCursor);
        break;
    default:
        setCursor(Qt::ArrowCursor);
        break;
    }
}

void EditorWindow::adjustRect(const QPoint &pos)
{
    if (!m_isResizing)
        return;

    QRect  newRect = m_captureRect;
    QPoint delta = pos - m_dragStartPos;

    switch (m_currentHandle) {
    case ResizeHandle::TopLeft:
        newRect.setTopLeft(newRect.topLeft() + delta);
        break;
    case ResizeHandle::TopRight:
        newRect.setTopRight(newRect.topRight() + delta);
        break;
    case ResizeHandle::BottomRight:
        newRect.setBottomRight(newRect.bottomRight() + delta);
        break;
    case ResizeHandle::BottomLeft:
        newRect.setBottomLeft(newRect.bottomLeft() + delta);
        break;
    case ResizeHandle::Left:
        newRect.setLeft(newRect.left() + delta.x());
        break;
    case ResizeHandle::Right:
        newRect.setRight(newRect.right() + delta.x());
        break;
    case ResizeHandle::Top:
        newRect.setTop(newRect.top() + delta.y());
        break;
    case ResizeHandle::Bottom:
        newRect.setBottom(newRect.bottom() + delta.y());
        break;
    case ResizeHandle::Move:
        newRect.translate(delta);
        break;
    default:
        break;
    }

    // 确保选区不会超出屏幕
    QRect screenRect = rect();
    if (newRect.left() < screenRect.left())
        newRect.moveLeft(screenRect.left());
    if (newRect.top() < screenRect.top())
        newRect.moveTop(screenRect.top());
    if (newRect.right() > screenRect.right())
        newRect.moveRight(screenRect.right());
    if (newRect.bottom() > screenRect.bottom())
        newRect.moveBottom(screenRect.bottom());

    m_captureRect = newRect.normalized();
    m_dragStartPos = pos;
}

void EditorWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->pos();
        m_currentHandle = hitTest(m_dragStartPos);
        m_isResizing = (m_currentHandle != ResizeHandle::None);
    }
}

void EditorWindow::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();

    if (m_isResizing) {
        adjustRect(pos);
    } else {
        ResizeHandle handle = hitTest(pos);
        updateCursor(handle);
    }

    update();
}

void EditorWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isResizing = false;
        m_currentHandle = ResizeHandle::None;
        updateCursor(ResizeHandle::None);
    }
}
