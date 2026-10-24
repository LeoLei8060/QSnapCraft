#include "pinwindow.h"

#include "output/outputservice.h"
#include "pinboard/pinimagelimits.h"
#include "pinboard/pinsnapresolver.h"
#include "ui/glyphicon.h"

#include <QAction>
#include <QApplication>
#include <QBuffer>
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
#include <QMovie>
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

void PinWindow::pinImage(const QPixmap &pixmap, const QRect &rect, bool showWindow)
{
    if (m_movie != nullptr) {
        m_movie->stop();
        delete m_movie;
        m_movie = nullptr;
        delete m_animationBuffer;
        m_animationBuffer = nullptr;
    }
    if (m_animationAction != nullptr) {
        m_animationAction->setVisible(false);
        m_animationMenu->menuAction()->setVisible(false);
    }
    m_animationData.clear();
    m_animationFormat.clear();
    m_currentImage = pixmap;
    m_displayedSize = {};
    setContentGeometry(rect);
    notifyContentGeometryChanged();
    if (showWindow) {
        show();
        raise();
    }
    emit sigStateChanged(this);
}

void PinWindow::pinAnimation(const QByteArray &data, const QString &format, const QRect &rect)
{
    if (data.isEmpty()) {
        return;
    }
    m_animationData = data;
    m_animationFormat = format;
    delete m_movie;
    delete m_animationBuffer;
    m_animationBuffer = new QBuffer(this);
    m_animationBuffer->setData(data);
    m_animationBuffer->open(QIODevice::ReadOnly);
    const QString movieFormat = format.startsWith(QStringLiteral("image/")) ? format.mid(6) : format;
    m_movie = new QMovie(m_animationBuffer, movieFormat.toLatin1(), this);
    m_movie->setCacheMode(QMovie::CacheAll);
    m_movie->start();
    if (m_movie->currentPixmap().isNull()) {
        delete m_movie;
        m_movie = nullptr;
        return;
    }
    connect(m_movie, &QMovie::frameChanged, this, [this](int) { m_currentImage = m_movie->currentPixmap(); });
    m_imageLabel->setMovie(m_movie);
    m_currentImage = m_movie->currentPixmap();
    setContentGeometry(rect);
    m_animationAction->setVisible(true);
    m_animationMenu->menuAction()->setVisible(true);
    m_animationAction->setChecked(true);
    show();
    raise();
    emit sigStateChanged(this);
}

void PinWindow::setClickThrough(bool enabled)
{
    if (m_clickThrough == enabled) {
        return;
    }

    m_clickThrough = enabled;
    setAttribute(Qt::WA_TransparentForMouseEvents, enabled);
    applyClickThroughStyle();
    emit sigStateChanged(this);
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
    emit sigStateChanged(this);
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
    emit sigStateChanged(this);
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

QPixmap PinWindow::currentImage() const
{
    return m_currentImage;
}

QString PinWindow::pinId() const
{
    return m_pinId;
}

QString PinWindow::groupId() const
{
    return m_groupId;
}

void PinWindow::setPinId(const QString &pinId)
{
    m_pinId = pinId;
}

void PinWindow::setGroup(const QString &groupId)
{
    if (m_groupId == groupId) {
        return;
    }
    m_groupId = groupId;
    setGroups(m_groups);
    emit sigStateChanged(this);
}

void PinWindow::setGroups(const QVector<PinGroupData> &groups)
{
    m_groups = groups;
    if (m_groupMenu == nullptr) {
        return;
    }
    m_groupMenu->clear();
    QAction *createAction = m_groupMenu->addAction(QStringLiteral("新建分组…"));
    connect(createAction, &QAction::triggered, this, [this]() { emit sigCreateGroupRequested(this); });
    m_groupMenu->addSeparator();
    for (const PinGroupData &group : groups) {
        QMenu   *groupMenu = m_groupMenu->addMenu(group.name);
        QAction *assignAction = groupMenu->addAction(QStringLiteral("归入本组"));
        assignAction->setCheckable(true);
        assignAction->setChecked(group.id == m_groupId);
        connect(assignAction, &QAction::triggered, this, [this, group]() {
            emit sigGroupAssignmentRequested(this, group.id);
        });
        QAction *showAction = groupMenu->addAction(QStringLiteral("显示本组"));
        connect(showAction, &QAction::triggered, this, [this, group]() {
            emit sigGroupActionRequested(this, group.id, PinGroupAction::kShow);
        });
        QAction *hideAction = groupMenu->addAction(QStringLiteral("隐藏本组"));
        connect(hideAction, &QAction::triggered, this, [this, group]() {
            emit sigGroupActionRequested(this, group.id, PinGroupAction::kHide);
        });
        QAction *closeAction = groupMenu->addAction(QStringLiteral("关闭本组"));
        connect(closeAction, &QAction::triggered, this, [this, group]() {
            emit sigGroupActionRequested(this, group.id, PinGroupAction::kClose);
        });
        groupMenu->addSeparator();
        QAction *renameAction = groupMenu->addAction(QStringLiteral("重命名…"));
        connect(renameAction, &QAction::triggered, this, [this, group]() {
            emit sigGroupActionRequested(this, group.id, PinGroupAction::kRename);
        });
        QAction *removeAction = groupMenu->addAction(QStringLiteral("删除分组"));
        connect(removeAction, &QAction::triggered, this, [this, group]() {
            emit sigGroupActionRequested(this, group.id, PinGroupAction::kRemove);
        });
    }
    if (!m_groupId.isEmpty()) {
        m_groupMenu->addSeparator();
        QAction *removeAction = m_groupMenu->addAction(QStringLiteral("移出分组"));
        connect(removeAction, &QAction::triggered, this, [this]() { emit sigGroupAssignmentRequested(this, {}); });
    }
}

double PinWindow::opacity() const
{
    return windowOpacity();
}

bool PinWindow::shadowEnabled() const
{
    return m_shadowEffect != nullptr && m_shadowEffect->isEnabled();
}

bool PinWindow::isAnimated() const
{
    return m_movie != nullptr;
}

QByteArray PinWindow::animationData() const
{
    return m_animationData;
}

QString PinWindow::animationFormat() const
{
    return m_animationFormat;
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
            emit sigStateChanged(this);
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

    m_animationMenu = m_contextMenu->addMenu(QStringLiteral("动画"));
    m_animationMenu->setObjectName(QStringLiteral("menu_Animation"));
    m_animationAction = m_animationMenu->addAction(QStringLiteral("暂停动画"));
    m_animationAction->setObjectName(QStringLiteral("action_ToggleAnimation"));
    m_animationAction->setCheckable(true);
    m_animationMenu->setVisible(false);
    connect(m_animationAction, &QAction::toggled, this, [this](bool playing) {
        if (m_movie == nullptr) {
            return;
        }
        m_animationAction->setText(playing ? QStringLiteral("暂停动画") : QStringLiteral("播放动画"));
        playing ? m_movie->start() : m_movie->stop();
    });
    QAction *previousFrame = m_animationMenu->addAction(QStringLiteral("上一帧"));
    previousFrame->setObjectName(QStringLiteral("action_PreviousFrame"));
    connect(previousFrame, &QAction::triggered, this, [this]() {
        if (m_movie != nullptr && m_movie->frameCount() > 0) {
            m_movie->jumpToFrame((m_movie->currentFrameNumber() - 1 + m_movie->frameCount()) % m_movie->frameCount());
        }
    });
    QAction *nextFrame = m_animationMenu->addAction(QStringLiteral("下一帧"));
    nextFrame->setObjectName(QStringLiteral("action_NextFrame"));
    connect(nextFrame, &QAction::triggered, this, [this]() {
        if (m_movie != nullptr && m_movie->frameCount() > 0) {
            m_movie->jumpToFrame((m_movie->currentFrameNumber() + 1) % m_movie->frameCount());
        }
    });
    m_animationMenu->addSeparator();
    const QVector<QPair<QString, int>> speeds = {
        {QStringLiteral("0.5×"), 50},
        {QStringLiteral("1×"), 100},
        {QStringLiteral("2×"), 200},
    };
    for (const auto &[label, speed] : speeds) {
        QAction *speedAction = m_animationMenu->addAction(label);
        speedAction->setObjectName(QStringLiteral("action_AnimationSpeed%1").arg(speed));
        connect(speedAction, &QAction::triggered, this, [this, speed]() {
            if (m_movie != nullptr) {
                m_movie->setSpeed(speed);
            }
        });
    }

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

    m_groupMenu = m_contextMenu->addMenu(QStringLiteral("分组"));
    m_groupMenu->setObjectName(QStringLiteral("menu_PinGroup"));
    connect(m_groupMenu, &QMenu::aboutToShow, this, [this]() { setGroups(m_groups); });
    setGroups(m_groups);

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
    emit sigStateChanged(this);
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
    emit sigStateChanged(this);
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
    emit sigStateChanged(this);
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
    emit sigStateChanged(this);
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
