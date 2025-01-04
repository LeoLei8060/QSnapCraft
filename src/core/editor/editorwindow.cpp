#include "editorwindow.h"
#include <windows.h>
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QStandardPaths>

EditorWindow::EditorWindow(QWidget *parent)
    : QWidget(parent)
    , m_toolbar(this)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Widget);
    setAttribute(Qt::WA_TranslucentBackground);

    // 连接信号
    connect(&m_toolbar, &Toolbar::toolSelected, this, &EditorWindow::onToolSelected);
    m_toolbar.hide();

    const auto screens = QGuiApplication::screens();
    for (const auto *screen : screens) {
        m_screenGeometries[const_cast<QScreen *>(screen)] = screen->geometry();
        m_totalGeometry = m_totalGeometry.united(screen->geometry());
    }
    setGeometry(m_totalGeometry);

    for (auto *screen : screens) {
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

void EditorWindow::start(const QPixmap &image, const QRect &captureRect)
{
    m_screenshotPixmap = image;
    m_captureRect = captureRect;
    m_toolbar.show();
    updateToolbarPosition();
    show();
}

void EditorWindow::hideWindow()
{
    m_toolbar.hide();
    hide();
}

void EditorWindow::updateToolbarPosition()
{
    const int toolbarWidth = m_toolbar.width();
    const int toolbarHeight = m_toolbar.height();
    const int spacing = 8;

    int x = m_captureRect.right() - toolbarWidth;
    int y = m_captureRect.bottom() + spacing;

    if (x + toolbarWidth > m_totalGeometry.right()) {
        x = m_totalGeometry.right() - toolbarWidth;
    }
    if (x < m_totalGeometry.left()) {
        x = m_totalGeometry.left();
    }

    if (y + toolbarHeight > m_totalGeometry.bottom()) {
        y = m_captureRect.top() - toolbarHeight - spacing;
    }

    m_toolbar.move(x, y);
    m_toolbar.raise();
}

void EditorWindow::onToolSelected(Toolbar::Tool tool)
{
    switch (tool) {
    case Toolbar::Tool::Copy:
        // TODO: 实现复制功能
        break;
    case Toolbar::Tool::Save:
        saveImage();
        break;
    case Toolbar::Tool::Pin:
        // TODO: 实现贴到屏幕功能
        break;
    case Toolbar::Tool::Exit:
        close(); // 关闭窗口
        break;
    default:
        // 其他工具的处理...
        break;
    }
}

QString EditorWindow::getSaveFilePath()
{
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QString defaultName = QString("screenshot_%1.png")
                              .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

    return QFileDialog::getSaveFileName(this,
                                        tr("Save Screenshot"),
                                        defaultPath + "/" + defaultName,
                                        tr("Images (*.png *.jpg *.bmp)"));
}

void EditorWindow::saveImage()
{
    QString filePath = getSaveFilePath();
    if (filePath.isEmpty()) {
        return; // 用户取消了保存操作
    }

    // 创建要保存的图像（仅包含捕获区域）
    QPixmap savePixmap = m_screenshotPixmap.copy(m_captureRect);

    // 保存图像
    if (!savePixmap.save(filePath)) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to save the image to %1").arg(filePath));
        return;
    }

    // 发送信号-截屏结束
    emit sigEditorFinished();
}

void EditorWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);

    if (m_captureRect.isValid()) {
        painter.drawPixmap(0, 0, m_screenshotPixmap);

        QPainterPath windowPath;
        windowPath.addRect(rect());

        QPainterPath highlightPath;
        highlightPath.addRect(m_captureRect);

        QPainterPath overlayPath = windowPath.subtracted(highlightPath);

        painter.fillPath(overlayPath, QColor(0, 0, 0, 128));

        painter.setPen(QPen(Qt::white, 2));
        painter.drawRect(m_captureRect);

        const int handleSize = 8;
        painter.setBrush(Qt::white);
        painter.setPen(Qt::black);

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

    // TODO: 暂时先取消编辑窗口放大镜功能
    //    m_magnifier.paint(painter, m_currentImage, QCursor::pos());
}

EditorWindow::ResizeHandle EditorWindow::hitTest(const QPoint &pos) const
{
    const int handleSize = 8;

    QRect handleRect;

    handleRect = QRect(m_captureRect.topLeft().x() - handleSize / 2,
                       m_captureRect.topLeft().y() - handleSize / 2,
                       handleSize,
                       handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::TopLeft;

    handleRect = QRect(m_captureRect.center().x() - handleSize / 2,
                       m_captureRect.top() - handleSize / 2,
                       handleSize,
                       handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::Top;

    handleRect = QRect(m_captureRect.topRight().x() - handleSize / 2,
                       m_captureRect.topRight().y() - handleSize / 2,
                       handleSize,
                       handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::TopRight;

    handleRect = QRect(m_captureRect.right() - handleSize / 2,
                       m_captureRect.center().y() - handleSize / 2,
                       handleSize,
                       handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::Right;

    handleRect = QRect(m_captureRect.bottomRight().x() - handleSize / 2,
                       m_captureRect.bottomRight().y() - handleSize / 2,
                       handleSize,
                       handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::BottomRight;

    handleRect = QRect(m_captureRect.center().x() - handleSize / 2,
                       m_captureRect.bottom() - handleSize / 2,
                       handleSize,
                       handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::Bottom;

    handleRect = QRect(m_captureRect.bottomLeft().x() - handleSize / 2,
                       m_captureRect.bottomLeft().y() - handleSize / 2,
                       handleSize,
                       handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::BottomLeft;

    handleRect = QRect(m_captureRect.left() - handleSize / 2,
                       m_captureRect.center().y() - handleSize / 2,
                       handleSize,
                       handleSize);
    if (handleRect.contains(pos))
        return ResizeHandle::Left;

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
        m_isDragging = (m_currentHandle != ResizeHandle::None);
    } else if (event->button() == Qt::RightButton) {
        m_toolbar.hide();
        emit sigCancelEditor();
    }
}

void EditorWindow::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = QCursor::pos();

    if (m_isDragging) {
        adjustRect(pos);
    } else {
        ResizeHandle handle = hitTest(pos);
        updateCursor(handle);
    }
}

void EditorWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        m_currentHandle = ResizeHandle::None;
        updateToolbarPosition();
    }
}

void EditorWindow::adjustRect(const QPoint &pos)
{
    if (!m_isDragging)
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

    QScreen *screen = QGuiApplication::screenAt(pos);
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }

    if (m_currentHandle == ResizeHandle::Move) {
        if (newRect.left() < m_totalGeometry.left())
            newRect.moveLeft(m_totalGeometry.left());
        if (newRect.top() < m_totalGeometry.top())
            newRect.moveTop(m_totalGeometry.top());
        if (newRect.right() > m_totalGeometry.right())
            newRect.moveRight(m_totalGeometry.right());
        if (newRect.bottom() > m_totalGeometry.bottom())
            newRect.moveBottom(m_totalGeometry.bottom());
    } else {
        const QRect &screenRect = m_screenGeometries[screen];

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

    static const int MIN_SIZE = 10;
    if (newRect.width() < MIN_SIZE) {
        if (m_currentHandle == ResizeHandle::Left || m_currentHandle == ResizeHandle::TopLeft
            || m_currentHandle == ResizeHandle::BottomLeft)
            newRect.setLeft(newRect.right() - MIN_SIZE);
        else
            newRect.setRight(newRect.left() + MIN_SIZE);
    }
    if (newRect.height() < MIN_SIZE) {
        if (m_currentHandle == ResizeHandle::Top || m_currentHandle == ResizeHandle::TopLeft
            || m_currentHandle == ResizeHandle::TopRight)
            newRect.setTop(newRect.bottom() - MIN_SIZE);
        else
            newRect.setBottom(newRect.top() + MIN_SIZE);
    }

    if (newRect != m_captureRect) {
        m_captureRect = newRect.normalized();
        m_dragStartPos = pos;
        updateToolbarPosition();
        update();
    }
}
