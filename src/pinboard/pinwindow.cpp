#include "pinwindow.h"

#include "output/outputservice.h"
#include "pinboard/pinimagelimits.h"
#include "pinboard/pinsnapresolver.h"
#include "ui/glyphicon.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QDebug>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsDropShadowEffect>
#include <QImageReader>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QScreen>
#include <QSignalBlocker>
#include <QUrl>
#include <QWheelEvent>

#include <windows.h>

#include <algorithm>

PinWindow::PinWindow(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("pinWindow"));
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);

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
    m_displayedSize = {};
    setContentGeometry(rect);
    notifyContentGeometryChanged();
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
    applyClickThroughStyle();
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
    if (m_clickThrough) {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        applyClickThroughStyle();
    }
}

void PinWindow::setSnapDistancePx(int distancePx)
{
    m_snapDistancePx = qBound(0, distancePx, 32);
}

void PinWindow::setSnapTargets(const QVector<QRect> &targets)
{
    m_snapTargets = targets;
}

void PinWindow::setSmoothScaling(bool enabled)
{
    if (m_smoothScaling == enabled) {
        return;
    }
    m_smoothScaling = enabled;
    m_displayedSize = {};
    refreshDisplayedPixmap();
}

void PinWindow::setZoomAtCursor(bool enabled)
{
    m_zoomAtCursor = enabled;
}

QRect PinWindow::contentRect() const
{
    return QRect(m_imageLabel->mapToGlobal(QPoint()), m_imageLabel->size());
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
        emit sigSnapGuidesChanged(this, {});
        notifyContentGeometryChanged();
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
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        const int key = event->key();
        if (key >= Qt::Key_0 && key <= Qt::Key_9) {
            setWindowOpacity(key == Qt::Key_0 ? 1.0 : qreal(key - Qt::Key_0) / 10.0);
            return;
        }
    }

    if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) {
        scaleContentBy(1.1, contentRect().center());
        return;
    }
    if (event->key() == Qt::Key_Minus) {
        scaleContentBy(1.0 / 1.1, contentRect().center());
        return;
    }
    if (event->key() == Qt::Key_1) {
        applyTransform(QTransform().rotate(-90));
        return;
    }
    if (event->key() == Qt::Key_2) {
        applyTransform(QTransform().rotate(90));
        return;
    }
    if (event->key() == Qt::Key_3) {
        applyTransform(QTransform().scale(-1, 1));
        return;
    }
    if (event->key() == Qt::Key_4) {
        applyTransform(QTransform().scale(1, -1));
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
        scaleContentBy(direction > 0 ? 1.1 : 1.0 / 1.1, event->globalPosition().toPoint());
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
    refreshDisplayedPixmap();
    notifyContentGeometryChanged();
}

void PinWindow::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    const bool       hasImage = mimeData->hasImage() || !mimeData->imageData().isNull();
    const bool       hasFile = std::any_of(mimeData->urls().cbegin(), mimeData->urls().cend(), [](const QUrl &url) {
        return url.isLocalFile();
    });
    if (hasImage || hasFile) {
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void PinWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    QImage           image;
    if (mimeData->hasImage()) {
        image = qvariant_cast<QImage>(mimeData->imageData());
    }
    if (image.isNull()) {
        for (const QUrl &url : mimeData->urls()) {
            if (!url.isLocalFile()) {
                continue;
            }
            QImageReader reader(url.toLocalFile());
            reader.setAutoTransform(true);
            const QSize sourceSize = reader.size();
            if (sourceSize.isValid() && !canCreatePinImage(sourceSize)) {
                reader.setScaledSize(boundedPinImageSize(sourceSize));
            }
            image = reader.read();
            image = limitPinImageSize(image);
            if (!image.isNull()) {
                break;
            }
        }
    }
    if (image.isNull()) {
        event->ignore();
        return;
    }

    replaceImage(image);
    event->acceptProposedAction();
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
    refreshDisplayedPixmap();
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

    const QRect   oldContent = contentRect();
    const QPixmap transformed = m_currentImage.transformed(transform, Qt::FastTransformation);
    if (transformed.isNull()) {
        return;
    }

    m_currentImage = transformed;
    m_displayedSize = {};
    const QRectF transformedBounds = transform.mapRect(QRectF(QPointF(0, 0), QSizeF(oldContent.size())));
    const QSize  targetSize(qMax(1, qRound(qAbs(transformedBounds.width()))),
                           qMax(1, qRound(qAbs(transformedBounds.height()))));
    QRect        targetContentRect(QPoint(), targetSize);
    targetContentRect.moveCenter(oldContent.center());
    setContentGeometry(targetContentRect);
    notifyContentGeometryChanged();
}

void PinWindow::scaleContentBy(qreal factor, const QPoint &anchorGlobal)
{
    const QRect  oldContent = contentRect();
    const QPoint anchor = m_zoomAtCursor ? anchorGlobal : oldContent.center();
    const qreal  xRatio = qreal(anchor.x() - oldContent.left()) / qMax(1, oldContent.width());
    const qreal  yRatio = qreal(anchor.y() - oldContent.top()) / qMax(1, oldContent.height());
    const QSize  nextSize(qBound(MIN_CONTENT_SIZE, qRound(m_imageLabel->width() * factor), MAX_CONTENT_SIZE),
                         qBound(MIN_CONTENT_SIZE, qRound(m_imageLabel->height() * factor), MAX_CONTENT_SIZE));
    QRect        targetContentRect(QPoint(), nextSize);
    targetContentRect.moveTopLeft(
        QPoint(qRound(anchor.x() - nextSize.width() * xRatio), qRound(anchor.y() - nextSize.height() * yRatio)));
    setContentGeometry(targetContentRect);
    notifyContentGeometryChanged();
}

void PinWindow::moveContentBy(const QPoint &delta)
{
    QRect nextGeometry = geometry().translated(delta);
    if (QScreen *screen = QGuiApplication::screenAt(frameGeometry().center())) {
        const QRect available = screen->availableGeometry();
        nextGeometry.moveLeft(qBound(available.left() - SHADOW_MARGIN,
                                     nextGeometry.left(),
                                     available.right() - nextGeometry.width() + SHADOW_MARGIN + 1));
        nextGeometry.moveTop(qBound(available.top() - SHADOW_MARGIN,
                                    nextGeometry.top(),
                                    available.bottom() - nextGeometry.height() + SHADOW_MARGIN + 1));
    }
    move(nextGeometry.topLeft());
    notifyContentGeometryChanged();
}

void PinWindow::adjustOpacity(int direction)
{
    const qreal next_opacity = qBound(0.2, windowOpacity() + direction * 0.05, 1.0);
    setWindowOpacity(next_opacity);
}

void PinWindow::setContentGeometry(const QRect &contentRect)
{
    setGeometry(contentRect.adjusted(-SHADOW_MARGIN, -SHADOW_MARGIN, SHADOW_MARGIN, SHADOW_MARGIN));
    updateImageLabelGeometry();
    refreshDisplayedPixmap();
}

void PinWindow::applyClickThroughStyle()
{
    const HWND     window = reinterpret_cast<HWND>(winId());
    const LONG_PTR currentStyle = GetWindowLongPtrW(window, GWL_EXSTYLE);
    const LONG_PTR nextStyle = m_clickThrough ? currentStyle | WS_EX_TRANSPARENT | WS_EX_LAYERED
                                              : currentStyle & ~WS_EX_TRANSPARENT;
    SetWindowLongPtrW(window, GWL_EXSTYLE, nextStyle);
}

void PinWindow::notifyContentGeometryChanged()
{
    const QRect contentGeometry = contentRect();
    if (contentGeometry == m_reportedContentRect) {
        return;
    }
    m_reportedContentRect = contentGeometry;
    emit sigContentGeometryChanged(this, contentGeometry);
}

void PinWindow::refreshDisplayedPixmap()
{
    if (m_imageLabel == nullptr) {
        return;
    }
    if (m_currentImage.isNull() || m_imageLabel->size().isEmpty()) {
        m_imageLabel->clear();
        m_displayedSize = {};
        return;
    }
    const qreal devicePixelRatio = devicePixelRatioF();
    if (m_displayedSize == m_imageLabel->size() && qFuzzyCompare(m_displayedDevicePixelRatio, devicePixelRatio)) {
        return;
    }

    const Qt::TransformationMode mode = m_smoothScaling ? Qt::SmoothTransformation : Qt::FastTransformation;
    const QSize                  targetPixels((QSizeF(m_imageLabel->size()) * devicePixelRatio).toSize());
    QPixmap                      displayed = m_currentImage.scaled(targetPixels, Qt::IgnoreAspectRatio, mode);
    displayed.setDevicePixelRatio(devicePixelRatio);
    m_imageLabel->setPixmap(displayed);
    m_displayedSize = m_imageLabel->size();
    m_displayedDevicePixelRatio = devicePixelRatio;
}

void PinWindow::replaceImage(const QImage &image)
{
    if (image.isNull()) {
        return;
    }
    m_currentImage = QPixmap::fromImage(image);
    m_displayedSize = {};
    refreshDisplayedPixmap();
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

    if (m_currentHandle == ResizeHandle::Move) {
        const QRect contentRect(newGeometry.topLeft() + QPoint(SHADOW_MARGIN, SHADOW_MARGIN),
                                QSize(newGeometry.width() - SHADOW_MARGIN * 2,
                                      newGeometry.height() - SHADOW_MARGIN * 2));
        QScreen    *screen = QGuiApplication::screenAt(contentRect.center());
        if (screen != nullptr) {
            const PinSnapResult snap = PinSnapResolver::resolve(contentRect,
                                                                screen->availableGeometry(),
                                                                m_snapTargets,
                                                                m_snapDistancePx);
            newGeometry.translate(snap.contentRect.topLeft() - contentRect.topLeft());
            emit sigSnapGuidesChanged(this, snap.guides);
        } else {
            emit sigSnapGuidesChanged(this, {});
        }
    } else {
        emit sigSnapGuidesChanged(this, {});
    }

    setGeometry(newGeometry);
    updateImageLabelGeometry();
    notifyContentGeometryChanged();
}
