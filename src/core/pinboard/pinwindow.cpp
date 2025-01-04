#include "pinwindow.h"
#include <QApplication>
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>

PinWindow::PinWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    qDebug() << __FUNCTION__;
}

PinWindow::~PinWindow()
{
    qDebug() << __FUNCTION__;
}

void PinWindow::pinImage(const QPixmap &pixmap, const QRect &rect)
{
    setGeometry(rect);
    m_currentImage = pixmap;
    show();
}

void PinWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 绘制半透明背景
    painter.fillRect(rect(), QColor(0, 0, 0, 1));

    painter.drawPixmap(rect(), m_currentImage);

    // 绘制边框
    painter.setPen(QPen(Qt::gray, 1, Qt::SolidLine));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

void PinWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_dragStartPos = event->globalPos();
        m_startGeometry = geometry();
        m_currentHandle = hitTest(event->pos());
        event->accept();
    }
}

void PinWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_isDragging) {
        updateCursor(hitTest(event->pos()));
        return;
    }

    if (event->buttons() & Qt::LeftButton) {
        adjustRect(event->globalPos());
        event->accept();
    }
}

void PinWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        m_currentHandle = ResizeHandle::None;
        updateCursor(hitTest(event->pos()));
    }
}

void PinWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        close();
    }
}

PinWindow::ResizeHandle PinWindow::hitTest(const QPoint &pos) const
{
    const int width = this->width();
    const int height = this->height();
    const int border = RESIZE_HANDLE_SIZE;

    if (pos.x() <= border) {
        if (pos.y() <= border)
            return ResizeHandle::TopLeft;
        else if (pos.y() >= height - border)
            return ResizeHandle::BottomLeft;
        else
            return ResizeHandle::Left;
    } else if (pos.x() >= width - border) {
        if (pos.y() <= border)
            return ResizeHandle::TopRight;
        else if (pos.y() >= height - border)
            return ResizeHandle::BottomRight;
        else
            return ResizeHandle::Right;
    } else if (pos.y() <= border) {
        return ResizeHandle::Top;
    } else if (pos.y() >= height - border) {
        return ResizeHandle::Bottom;
    }

    return ResizeHandle::Move;
}

void PinWindow::updateCursor(ResizeHandle handle)
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
    default:
        setCursor(Qt::ArrowCursor);
        break;
    }
}

void PinWindow::adjustRect(const QPoint &pos)
{
    QRect  newGeometry = m_startGeometry;
    QPoint delta = pos - m_dragStartPos;

    switch (m_currentHandle) {
    case ResizeHandle::TopLeft:
        newGeometry.setTopLeft(m_startGeometry.topLeft() + delta);
        break;
    case ResizeHandle::Top:
        newGeometry.setTop(m_startGeometry.top() + delta.y());
        break;
    case ResizeHandle::TopRight:
        newGeometry.setTopRight(m_startGeometry.topRight() + delta);
        break;
    case ResizeHandle::Right:
        newGeometry.setRight(m_startGeometry.right() + delta.x());
        break;
    case ResizeHandle::BottomRight:
        newGeometry.setBottomRight(m_startGeometry.bottomRight() + delta);
        break;
    case ResizeHandle::Bottom:
        newGeometry.setBottom(m_startGeometry.bottom() + delta.y());
        break;
    case ResizeHandle::BottomLeft:
        newGeometry.setBottomLeft(m_startGeometry.bottomLeft() + delta);
        break;
    case ResizeHandle::Left:
        newGeometry.setLeft(m_startGeometry.left() + delta.x());
        break;
    case ResizeHandle::Move:
        newGeometry.moveTopLeft(m_startGeometry.topLeft() + delta);
        break;
    default:
        return;
    }

    // 保持最小尺寸
    if (newGeometry.width() < 50)
        newGeometry.setWidth(50);
    if (newGeometry.height() < 50)
        newGeometry.setHeight(50);

    setGeometry(newGeometry);

    // 更新缩放后的图片
    m_scaledImage = m_currentImage.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    update();
}

QSize PinWindow::calculateFitSize(const QSize &imageSize, const QSize &maxSize) const
{
    if (imageSize.width() <= maxSize.width() && imageSize.height() <= maxSize.height())
        return imageSize;

    qreal widthRatio = qreal(maxSize.width()) / imageSize.width();
    qreal heightRatio = qreal(maxSize.height()) / imageSize.height();
    qreal ratio = qMin(widthRatio, heightRatio);

    return QSize(imageSize.width() * ratio, imageSize.height() * ratio);
}
