#include "colorpickerwindow.h"

#include "platform/windows/desktopcapture.h"

#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

#include <utility>

ColorPickerWindow::ColorPickerWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint);
    setCursor(Qt::CrossCursor);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    m_magnifier.toggleColorFormat();
}

ColorPickerWindow::~ColorPickerWindow() = default;

void ColorPickerWindow::start()
{
    CaptureFrame frame = captureVirtualDesktop();
    if (!frame.isValid()) {
        qWarning() << frame.error;
        emit sigCancelled();
        return;
    }

    m_desktopRect = frame.logicalBounds;
    m_image = std::move(frame.logicalComposite);
    m_background = QPixmap::fromImage(m_image);
    m_finished = false;

    setGeometry(m_desktopRect);
    show();
    raise();
    activateWindow();
    setFocus();
    updatePointerPosition(QCursor::pos() - m_desktopRect.topLeft());
}

QColor ColorPickerWindow::colorAt(const QImage &image, const QPoint &position)
{
    if (image.isNull() || !image.rect().contains(position)) {
        return {};
    }
    return image.pixelColor(position);
}

void ColorPickerWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.drawPixmap(0, 0, m_background);
    m_magnifier.paint(painter, m_background, m_image, m_pointerPosition);
}

void ColorPickerWindow::mouseMoveEvent(QMouseEvent *event)
{
    updatePointerPosition(event->position().toPoint());
}

void ColorPickerWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        finish(true);
    } else if (event->button() == Qt::RightButton) {
        finish(false);
    }
}

void ColorPickerWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        finish(false);
        return;
    }
    if (event->key() == Qt::Key_Shift) {
        m_magnifier.toggleColorFormat();
        updatePointerPosition(m_pointerPosition);
        return;
    }
    if (event->key() == Qt::Key_C) {
        finish(true);
        return;
    }
    QWidget::keyPressEvent(event);
}

void ColorPickerWindow::closeEvent(QCloseEvent *event)
{
    if (!m_finished) {
        finish(false);
    }
    QWidget::closeEvent(event);
}

void ColorPickerWindow::updatePointerPosition(const QPoint &position)
{
    if (!m_image.rect().contains(position)) {
        return;
    }
    m_pointerPosition = position;
    m_currentColor = colorAt(m_image, position);
    m_magnifier.setCurrentColor(m_currentColor);
    update();
}

void ColorPickerWindow::finish(bool picked)
{
    if (m_finished) {
        return;
    }

    m_finished = true;
    if (picked && m_currentColor.isValid()) {
        QApplication::clipboard()->setText(m_magnifier.getCurrentColorText());
    }
    hide();

    if (picked && m_currentColor.isValid()) {
        emit sigColorPicked(m_currentColor);
    } else {
        emit sigCancelled();
    }
}
