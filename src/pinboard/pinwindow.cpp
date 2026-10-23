#include "pinwindow.h"

#include "output/outputservice.h"
#include "ui/glyphicon.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsDropShadowEffect>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QScreen>
#include <QSignalBlocker>
#include <QWheelEvent>

#include <windows.h>

PinWindow::PinWindow(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("pinWindow"));
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_imageLabel = new QLabel(this);
    m_imageLabel->setObjectName(QStringLiteral("pinImageContent"));
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setScaledContents(true);
    m_imageLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_imageLabel->setStyleSheet(QStringLiteral("background: transparent; border: 1px solid rgba(128, 128, 128, 180);"));

    m_shadowEffect = new QGraphicsDropShadowEffect(m_imageLabel);
    m_shadowEffect->setBlurRadius(18);
    m_shadowEffect->setColor(QColor(0, 0, 0, 150));
    m_shadowEffect->setOffset(0, 3);
    m_imageLabel->setGraphicsEffect(m_shadowEffect);

    createContextMenu();
}

PinWindow::~PinWindow()
{
    qDebug() << __FUNCTION__;
}

void PinWindow::pinImage(const QPixmap &pixmap, const QRect &rect)
{
    m_currentImage = pixmap;
    m_imageLabel->setPixmap(m_currentImage);
    setGeometry(rect.adjusted(-SHADOW_MARGIN, -SHADOW_MARGIN, SHADOW_MARGIN, SHADOW_MARGIN));
    updateImageLabelGeometry();
    show();
    raise();
}

void PinWindow::setClickThrough(bool enabled)
{
    if (m_clickThrough == enabled) {
        return;
    }

    m_clickThrough = enabled;
    setAttribute(Qt::WA_TransparentForMouseEvents, enabled);

    const HWND     window = reinterpret_cast<HWND>(winId());
    const LONG_PTR current_style = GetWindowLongPtrW(window, GWL_EXSTYLE);
    const LONG_PTR next_style = enabled ? current_style | WS_EX_TRANSPARENT | WS_EX_LAYERED
                                        : current_style & ~WS_EX_TRANSPARENT;
    SetWindowLongPtrW(window, GWL_EXSTYLE, next_style);
}

bool PinWindow::isClickThrough() const
{
    return m_clickThrough;
}

void PinWindow::setShadowEnabled(bool enabled)
{
    if (m_shadowEffect == nullptr) {
        return;
    }
    if (m_shadowAction != nullptr) {
        const QSignalBlocker blocker(m_shadowAction);
        m_shadowAction->setChecked(enabled);
    }
    m_shadowEffect->setEnabled(enabled);
    update();
}

void PinWindow::setStayOnTop(bool enabled)
{
    if (m_stayOnTop == enabled) {
        return;
    }

    const bool  was_visible = isVisible();
    const QRect old_geometry = geometry();
    m_stayOnTop = enabled;
    setWindowFlag(Qt::WindowStaysOnTopHint, enabled);
    setGeometry(old_geometry);
    if (was_visible) {
        show();
        raise();
    }
}

void PinWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        setFocus(Qt::MouseFocusReason);
        m_isDragging = true;
        m_dragStartPos = event->globalPosition().toPoint();
        m_startGeometry = geometry();
        m_currentHandle = hitTest(event->position().toPoint());
        event->accept();
    }
}

void PinWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_isDragging) {
        updateCursor(hitTest(event->position().toPoint()));
        return;
    }

    if (event->buttons().testFlag(Qt::LeftButton)) {
        adjustRect(event->globalPosition().toPoint());
        event->accept();
    }
}

void PinWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        m_currentHandle = ResizeHandle::None;
        updateCursor(hitTest(event->position().toPoint()));
    }
}

void PinWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        close();
    }
}

void PinWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) {
        scaleContentBy(1.1);
        return;
    }
    if (event->key() == Qt::Key_Minus) {
        scaleContentBy(1.0 / 1.1);
        return;
    }

    const int distance = event->modifiers().testFlag(Qt::ShiftModifier) ? 10 : 1;
    switch (event->key()) {
    case Qt::Key_Left:
        moveContentBy(QPoint(-distance, 0));
        return;
    case Qt::Key_Right:
        moveContentBy(QPoint(distance, 0));
        return;
    case Qt::Key_Up:
        moveContentBy(QPoint(0, -distance));
        return;
    case Qt::Key_Down:
        moveContentBy(QPoint(0, distance));
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void PinWindow::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() == 0) {
        QWidget::wheelEvent(event);
        return;
    }

    const int direction = event->angleDelta().y() > 0 ? 1 : -1;
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        adjustOpacity(direction);
    } else {
        scaleContentBy(direction > 0 ? 1.1 : 1.0 / 1.1);
    }
    event->accept();
}

void PinWindow::closeEvent(QCloseEvent *event)
{
    emit sigCloseRequested(this);
    QWidget::closeEvent(event);
}

void PinWindow::contextMenuEvent(QContextMenuEvent *event)
{
    m_contextMenu->popup(event->globalPos());
    event->accept();
}

void PinWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateImageLabelGeometry();
}

void PinWindow::createContextMenu()
{
    m_contextMenu = new QMenu(this);
    m_contextMenu->setObjectName(QStringLiteral("menu_PinWindow"));
    m_contextMenu->setToolTipsVisible(true);
    m_contextMenu->setStyleSheet(
        QStringLiteral("QMenu { background: #292929; color: #E8E8E8; border: 1px solid #555;"
                       " padding: 5px; }"
                       "QMenu::item { padding: 6px 28px 6px 10px; border-radius: 4px; }"
                       "QMenu::item:selected { background: #0A84FF; color: white; }"
                       "QMenu::separator { height: 1px; background: #444; margin: 4px 8px; }"));

    QAction *copy_action = m_contextMenu->addAction(createGlyphIcon(GlyphIcon::kCopy), QStringLiteral("复制图像"));
    copy_action->setObjectName(QStringLiteral("action_CopyImage"));
    connect(copy_action, &QAction::triggered, this, &PinWindow::copyImage);

    QAction *save_action = m_contextMenu->addAction(createGlyphIcon(GlyphIcon::kSave), QStringLiteral("图像另存为"));
    save_action->setObjectName(QStringLiteral("action_SaveImageAs"));
    connect(save_action, &QAction::triggered, this, &PinWindow::saveImageAs);

    m_shadowAction = m_contextMenu->addAction(QStringLiteral("窗口阴影"));
    m_shadowAction->setObjectName(QStringLiteral("action_WindowShadow"));
    m_shadowAction->setCheckable(true);
    m_shadowAction->setChecked(true);
    connect(m_shadowAction, &QAction::toggled, this, [this](bool enabled) {
        m_shadowEffect->setEnabled(enabled);
        update();
    });

    QMenu *processing_menu = m_contextMenu->addMenu(QStringLiteral("图像处理"));
    processing_menu->setObjectName(QStringLiteral("menu_ImageProcessing"));

    QAction *rotate_left = processing_menu->addAction(QStringLiteral("向左旋转"));
    rotate_left->setObjectName(QStringLiteral("action_RotateLeft"));
    connect(rotate_left, &QAction::triggered, this, [this]() { applyTransform(QTransform().rotate(-90)); });

    QAction *rotate_right = processing_menu->addAction(QStringLiteral("向右旋转"));
    rotate_right->setObjectName(QStringLiteral("action_RotateRight"));
    connect(rotate_right, &QAction::triggered, this, [this]() { applyTransform(QTransform().rotate(90)); });

    QAction *flip_horizontal = processing_menu->addAction(QStringLiteral("水平翻转"));
    flip_horizontal->setObjectName(QStringLiteral("action_FlipHorizontal"));
    connect(flip_horizontal, &QAction::triggered, this, [this]() { applyTransform(QTransform().scale(-1, 1)); });

    QAction *flip_vertical = processing_menu->addAction(QStringLiteral("垂直翻转"));
    flip_vertical->setObjectName(QStringLiteral("action_FlipVertical"));
    connect(flip_vertical, &QAction::triggered, this, [this]() { applyTransform(QTransform().scale(1, -1)); });

    m_contextMenu->addSeparator();

    QAction *close_action = m_contextMenu->addAction(createGlyphIcon(GlyphIcon::kClose), QStringLiteral("关闭"));
    close_action->setObjectName(QStringLiteral("action_ClosePin"));
    connect(close_action, &QAction::triggered, this, &QWidget::close);

    QAction *destroy_action = m_contextMenu->addAction(createGlyphIcon(GlyphIcon::kClear), QStringLiteral("销毁"));
    destroy_action->setObjectName(QStringLiteral("action_DestroyPin"));
    connect(destroy_action, &QAction::triggered, this, &QWidget::deleteLater);
}

void PinWindow::updateImageLabelGeometry()
{
    if (m_imageLabel == nullptr) {
        return;
    }
    m_imageLabel->setGeometry(rect().adjusted(SHADOW_MARGIN, SHADOW_MARGIN, -SHADOW_MARGIN, -SHADOW_MARGIN));
}

void PinWindow::copyImage() const
{
    if (!m_currentImage.isNull()) {
        QApplication::clipboard()->setPixmap(m_currentImage);
    }
}

void PinWindow::saveImageAs()
{
    if (m_currentImage.isNull()) {
        return;
    }

    QString suggested_path = QStringLiteral("QSnapCraft.png");
    while (true) {
        QString selected_filter;
        QString path = QFileDialog::getSaveFileName(this,
                                                    QStringLiteral("图像另存为"),
                                                    suggested_path,
                                                    QStringLiteral("PNG 图像 (*.png);;"
                                                                   "JPEG 图像 (*.jpg *.jpeg);;"
                                                                   "BMP 图像 (*.bmp);;WebP 图像 (*.webp)"),
                                                    &selected_filter);
        if (path.isEmpty()) {
            return;
        }
        path = OutputService::ensurePathExtension(path, selected_filter);

        QString error;
        if (OutputService::saveImage(m_currentImage.toImage(), path, QFileInfo(path).suffix(), 95, &error)) {
            return;
        }

        const auto answer = QMessageBox::warning(this,
                                                 QStringLiteral("保存失败"),
                                                 error,
                                                 QMessageBox::Retry | QMessageBox::Cancel,
                                                 QMessageBox::Retry);
        if (answer != QMessageBox::Retry) {
            return;
        }
        suggested_path = path;
    }
}

void PinWindow::applyTransform(const QTransform &transform)
{
    if (m_currentImage.isNull()) {
        return;
    }

    const QPoint  content_top_left = m_imageLabel->mapToGlobal(QPoint());
    const QPixmap transformed = m_currentImage.transformed(transform, Qt::FastTransformation);
    if (transformed.isNull()) {
        return;
    }

    m_currentImage = transformed;
    m_imageLabel->setPixmap(m_currentImage);
    const QRect content_rect(content_top_left, m_currentImage.deviceIndependentSize().toSize());
    setGeometry(content_rect.adjusted(-SHADOW_MARGIN, -SHADOW_MARGIN, SHADOW_MARGIN, SHADOW_MARGIN));
    updateImageLabelGeometry();
}

void PinWindow::scaleContentBy(qreal factor)
{
    const QPoint center = m_imageLabel->mapToGlobal(m_imageLabel->rect().center());
    const QSize  next_size(qBound(MIN_CONTENT_SIZE, qRound(m_imageLabel->width() * factor), MAX_CONTENT_SIZE),
                          qBound(MIN_CONTENT_SIZE, qRound(m_imageLabel->height() * factor), MAX_CONTENT_SIZE));
    QRect        content_rect(QPoint(), next_size);
    content_rect.moveCenter(center);
    setGeometry(content_rect.adjusted(-SHADOW_MARGIN, -SHADOW_MARGIN, SHADOW_MARGIN, SHADOW_MARGIN));
    updateImageLabelGeometry();
}

void PinWindow::moveContentBy(const QPoint &delta)
{
    QRect next_geometry = geometry().translated(delta);
    if (QScreen *screen = QGuiApplication::screenAt(frameGeometry().center())) {
        const QRect available = screen->availableGeometry();
        next_geometry.moveLeft(qBound(available.left() - SHADOW_MARGIN,
                                      next_geometry.left(),
                                      available.right() - next_geometry.width() + SHADOW_MARGIN + 1));
        next_geometry.moveTop(qBound(available.top() - SHADOW_MARGIN,
                                     next_geometry.top(),
                                     available.bottom() - next_geometry.height() + SHADOW_MARGIN + 1));
    }
    move(next_geometry.topLeft());
}

void PinWindow::adjustOpacity(int direction)
{
    const qreal next_opacity = qBound(0.2, windowOpacity() + direction * 0.05, 1.0);
    setWindowOpacity(next_opacity);
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

    const int minimum_size = MIN_CONTENT_SIZE + SHADOW_MARGIN * 2;
    if (newGeometry.width() < minimum_size)
        newGeometry.setWidth(minimum_size);
    if (newGeometry.height() < minimum_size)
        newGeometry.setHeight(minimum_size);

    setGeometry(newGeometry);
    update();
}
