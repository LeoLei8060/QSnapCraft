#include "editor_window.h"
#include <windows.h>
#include <QApplication>
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>

EditorWindow::EditorWindow(QWidget *parent)
    : QWidget(parent)
    , m_toolbar(this)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    connect(&m_toolbar, &Toolbar::toolSelected, this, &EditorWindow::onToolSelected);
    m_toolbar.show();

    // 初始化屏幕几何信息
    const auto screens = QGuiApplication::screens();
    for (const auto* screen : screens) {
        m_screenGeometries[const_cast<QScreen*>(screen)] = screen->geometry();
        m_totalGeometry = m_totalGeometry.united(screen->geometry());
    }
    setGeometry(m_totalGeometry);

    // 监听屏幕变化
    for (auto* screen : screens) {
        connect(screen, &QScreen::geometryChanged, this, [this, screen](const QRect &geometry) {
            m_screenGeometries[screen] = geometry;
            m_totalGeometry = QRect();
            for (const auto &rect : m_screenGeometries) {
                m_totalGeometry = m_totalGeometry.united(rect);
            }
            setGeometry(m_totalGeometry);
        });
    }

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
    updateToolbarPosition();
    update();
}

void EditorWindow::updateToolbarPosition()
{
    // 计算工具栏位置，默认在捕获区域右边右对齐
    const int toolbarWidth = m_toolbar.width();
    const int toolbarHeight = m_toolbar.height();
    const int spacing = 8;  // 与捕获区域的间距

    // 默认位置：右对齐，在捕获区域下方
    int x = m_captureRect.right() - toolbarWidth;
    int y = m_captureRect.bottom() + spacing;

    // 如果工具栏超出了总屏幕的右边界，左移工具栏
    if (x + toolbarWidth > m_totalGeometry.right()) {
        x = m_totalGeometry.right() - toolbarWidth;
    }
    // 如果工具栏超出了总屏幕的左边界，右移工具栏
    if (x < m_totalGeometry.left()) {
        x = m_totalGeometry.left();
    }

    // 如果工具栏超出了总屏幕的下边界，将工具栏移到捕获区域上方
    if (y + toolbarHeight > m_totalGeometry.bottom()) {
        y = m_captureRect.top() - toolbarHeight - spacing;
    }

    m_toolbar.move(x, y);
    m_toolbar.raise();  // 确保工具栏总是在最上层
}

void EditorWindow::onToolSelected(Toolbar::Tool tool)
{
    switch (tool) {
    case Toolbar::Tool::Exit:
        close();
        break;
    case Toolbar::Tool::Copy:
        // TODO: 实现复制功能
        break;
    case Toolbar::Tool::Save:
        // TODO: 实现保存功能
        break;
    case Toolbar::Tool::Pin:
        // TODO: 实现贴到屏幕功能
        break;
    default:
        // TODO: 处理其他工具
        break;
    }
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
    const int handleSize = 8;

    // 检查是否在调整手柄区域内
    QRect handleRect;

    // 左上角
    handleRect = QRect(m_captureRect.topLeft().x() - handleSize / 2,
                      m_captureRect.topLeft().y() - handleSize / 2,
                      handleSize,
                      handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::TopLeft;

    // 上边
    handleRect = QRect(m_captureRect.center().x() - handleSize / 2,
                      m_captureRect.top() - handleSize / 2,
                      handleSize,
                      handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::Top;

    // 右上角
    handleRect = QRect(m_captureRect.topRight().x() - handleSize / 2,
                      m_captureRect.topRight().y() - handleSize / 2,
                      handleSize,
                      handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::TopRight;

    // 右边
    handleRect = QRect(m_captureRect.right() - handleSize / 2,
                      m_captureRect.center().y() - handleSize / 2,
                      handleSize,
                      handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::Right;

    // 右下角
    handleRect = QRect(m_captureRect.bottomRight().x() - handleSize / 2,
                      m_captureRect.bottomRight().y() - handleSize / 2,
                      handleSize,
                      handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::BottomRight;

    // 下边
    handleRect = QRect(m_captureRect.center().x() - handleSize / 2,
                      m_captureRect.bottom() - handleSize / 2,
                      handleSize,
                      handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::Bottom;

    // 左下角
    handleRect = QRect(m_captureRect.bottomLeft().x() - handleSize / 2,
                      m_captureRect.bottomLeft().y() - handleSize / 2,
                      handleSize,
                      handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::BottomLeft;

    // 左边
    handleRect = QRect(m_captureRect.left() - handleSize / 2,
                      m_captureRect.center().y() - handleSize / 2,
                      handleSize,
                      handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::Left;

    // 检查是否在选区内部
    if (m_captureRect.contains(pos))
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

void EditorWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = QCursor::pos();
        m_currentHandle = hitTest(m_dragStartPos);
        m_isResizing = (m_currentHandle != ResizeHandle::None);
    }
}

void EditorWindow::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = QCursor::pos();

    if (m_isResizing) {
        adjustRect(pos);
    } else {
        ResizeHandle handle = hitTest(pos);
        updateCursor(handle);
    }
}

void EditorWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isResizing = false;
        m_currentHandle = ResizeHandle::None;
        setCursor(Qt::ArrowCursor);
    }
}

void EditorWindow::adjustRect(const QPoint &pos)
{
    if (!m_isResizing)
        return;

    QRect newRect = m_captureRect;
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

    // 获取鼠标所在屏幕的几何信息
    QScreen* screen = QGuiApplication::screenAt(pos);
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }

    if (m_currentHandle == ResizeHandle::Move) {
        // 移动操作：确保在所有屏幕的总范围内
        if (newRect.left() < m_totalGeometry.left())
            newRect.moveLeft(m_totalGeometry.left());
        if (newRect.top() < m_totalGeometry.top())
            newRect.moveTop(m_totalGeometry.top());
        if (newRect.right() > m_totalGeometry.right())
            newRect.moveRight(m_totalGeometry.right());
        if (newRect.bottom() > m_totalGeometry.bottom())
            newRect.moveBottom(m_totalGeometry.bottom());
    } else {
        // 调整大小操作：确保在当前屏幕内
        const QRect& screenRect = m_screenGeometries[screen];
        
        // 根据拖动手柄的位置，限制矩形的调整范围
        switch (m_currentHandle) {
        case ResizeHandle::TopLeft:
        case ResizeHandle::Top:
        case ResizeHandle::TopRight:
            if (newRect.top() < screenRect.top())
                newRect.setTop(screenRect.top());
            break;
        case ResizeHandle::BottomLeft:
        case ResizeHandle::Bottom:
        case ResizeHandle::BottomRight:
            if (newRect.bottom() > screenRect.bottom())
                newRect.setBottom(screenRect.bottom());
            break;
        default:
            break;
        }

        switch (m_currentHandle) {
        case ResizeHandle::TopLeft:
        case ResizeHandle::Left:
        case ResizeHandle::BottomLeft:
            if (newRect.left() < screenRect.left())
                newRect.setLeft(screenRect.left());
            break;
        case ResizeHandle::TopRight:
        case ResizeHandle::Right:
        case ResizeHandle::BottomRight:
            if (newRect.right() > screenRect.right())
                newRect.setRight(screenRect.right());
            break;
        default:
            break;
        }
    }

    // 确保矩形大小不会太小
    static const int MIN_SIZE = 10;
    if (newRect.width() < MIN_SIZE) {
        if (m_currentHandle == ResizeHandle::Left || m_currentHandle == ResizeHandle::TopLeft || m_currentHandle == ResizeHandle::BottomLeft)
            newRect.setLeft(newRect.right() - MIN_SIZE);
        else
            newRect.setRight(newRect.left() + MIN_SIZE);
    }
    if (newRect.height() < MIN_SIZE) {
        if (m_currentHandle == ResizeHandle::Top || m_currentHandle == ResizeHandle::TopLeft || m_currentHandle == ResizeHandle::TopRight)
            newRect.setTop(newRect.bottom() - MIN_SIZE);
        else
            newRect.setBottom(newRect.top() + MIN_SIZE);
    }

    // 更新捕获区域
    if (newRect != m_captureRect) {
        m_captureRect = newRect.normalized();
        m_dragStartPos = pos;
        updateToolbarPosition();
        update();
    }
}
