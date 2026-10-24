#include "scrollcaptureindicator.h"

#include <QGuiApplication>
#include <QPainter>
#include <QScreen>

#include <windows.h>

ScrollCaptureIndicator::ScrollCaptureIndicator(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("scrollCaptureIndicator"));
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint
                   | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

void ScrollCaptureIndicator::setSelection(const QRect &global_selection)
{
    m_selection = global_selection;
    const QRect indicator_rect = global_selection.adjusted(-80, -64, 80, 16);
    setGeometry(indicator_rect);
    show();
    raise();

    const HWND     window = reinterpret_cast<HWND>(winId());
    const LONG_PTR style = GetWindowLongPtrW(window, GWL_EXSTYLE);
    SetWindowLongPtrW(window,
                      GWL_EXSTYLE,
                      style | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW);
    update();
}

void ScrollCaptureIndicator::setCapturedSize(const QSize &size)
{
    m_capturedSize = QSize(qMax(0, size.width()), qMax(0, size.height()));
    update();
}

void ScrollCaptureIndicator::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QRect border_rect = m_selection.translated(-geometry().topLeft());
    painter.setPen(QPen(QColor(0, 174, 255), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(border_rect.adjusted(-2, -2, 1, 1));

    const QString hint = QStringLiteral("滚动中… Enter 完成 / Esc 取消 · 已拼接 %1 × %2 px")
                             .arg(m_capturedSize.width())
                             .arg(m_capturedSize.height());
    QFont font = painter.font();
    font.setPixelSize(12);
    font.setBold(true);
    painter.setFont(font);
    const QSize text_size = painter.fontMetrics().size(Qt::TextSingleLine, hint);
    QRect       label(border_rect.left(), border_rect.top() - text_size.height() - 12, text_size.width() + 20, 28);
    if (label.top() < 0) {
        label.moveTop(border_rect.top() + 8);
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(20, 20, 20, 220));
    painter.drawRoundedRect(label, 5, 5);
    painter.setPen(Qt::white);
    painter.drawText(label, Qt::AlignCenter, hint);
}
