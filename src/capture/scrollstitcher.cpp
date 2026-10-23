#include "scrollstitcher.h"

#include <QPainter>
#include <QtMath>

#include <algorithm>
#include <limits>

namespace {
constexpr int   PROBE_HEIGHT = 64;
constexpr int   PROBE_COLUMN_STEP = 4;
constexpr qreal MATCH_TOLERANCE = 6.0;
constexpr int   MAX_STITCH_HEIGHT = 100000;

QRect changedRect(const QImage &previous, const QImage &next)
{
    if (previous.size() != next.size() || previous.format() != next.format()) {
        return {};
    }

    const QImage left = previous.convertToFormat(QImage::Format_RGB32);
    const QImage right = next.convertToFormat(QImage::Format_RGB32);
    int          left_bound = left.width();
    int          top_bound = left.height();
    int          right_bound = -1;
    int          bottom_bound = -1;
    for (int y = 0; y < left.height(); ++y) {
        const QRgb *left_line = reinterpret_cast<const QRgb *>(left.constScanLine(y));
        const QRgb *right_line = reinterpret_cast<const QRgb *>(right.constScanLine(y));
        for (int x = 0; x < left.width(); ++x) {
            if (left_line[x] != right_line[x]) {
                left_bound = qMin(left_bound, x);
                top_bound = qMin(top_bound, y);
                right_bound = qMax(right_bound, x);
                bottom_bound = qMax(bottom_bound, y);
            }
        }
    }
    if (right_bound < left_bound || bottom_bound < top_bound) {
        return {};
    }
    bottom_bound = qMin(left.height() - 1, bottom_bound + 1);
    return QRect(QPoint(left_bound, top_bound), QPoint(right_bound, bottom_bound));
}

qreal rowDifference(const QImage &previous, int previous_row, const QImage &next, int next_row, int left, int width)
{
    const QRgb *previous_line = reinterpret_cast<const QRgb *>(previous.constScanLine(previous_row));
    const QRgb *next_line = reinterpret_cast<const QRgb *>(next.constScanLine(next_row));
    qint64      difference = 0;
    int         samples = 0;
    for (int x = left; x < left + width; x += PROBE_COLUMN_STEP) {
        difference += qAbs(qRed(previous_line[x]) - qRed(next_line[x]));
        difference += qAbs(qGreen(previous_line[x]) - qGreen(next_line[x]));
        difference += qAbs(qBlue(previous_line[x]) - qBlue(next_line[x]));
        samples += 3;
    }
    return samples > 0 ? qreal(difference) / samples : 0.0;
}
} // namespace

void ScrollStitcher::reset(const QImage &first_frame)
{
    m_image = first_frame.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    m_previousFrame = m_image;
    m_viewport = m_image.rect();
    m_viewportDetected = false;
}

bool ScrollStitcher::appendFrame(const QImage &next_frame, int *added_rows)
{
    if (added_rows != nullptr) {
        *added_rows = 0;
    }
    if (m_image.isNull() || next_frame.isNull() || next_frame.size() != m_previousFrame.size()) {
        return false;
    }

    const QImage next = next_frame.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    if (!m_viewportDetected) {
        QRect viewport = changedRect(m_previousFrame, next);
        if (!viewport.isValid()) {
            return false;
        }

        const qreal area_ratio = qreal(viewport.width() * viewport.height()) / qreal(next.width() * next.height());
        if (area_ratio > 0.9) {
            viewport = next.rect();
        }
        m_viewport = viewport.intersected(next.rect());
        if (m_viewport.width() < 4 || m_viewport.height() < 8) {
            return false;
        }
        m_image = m_previousFrame.copy(m_viewport);
        m_viewportDetected = true;
    }

    const QImage previous_view = m_previousFrame.copy(m_viewport);
    const QImage next_view = next.copy(m_viewport);
    const int    overlap = overlapFor(previous_view, next_view, next_view.rect());
    if (overlap <= 0 || overlap >= next_view.height()) {
        return false;
    }

    appendRows(next, m_viewport, overlap);
    m_previousFrame = next;
    if (added_rows != nullptr) {
        *added_rows = overlap;
    }
    return true;
}

const QImage &ScrollStitcher::image() const
{
    return m_image;
}

QRect ScrollStitcher::viewport() const
{
    return m_viewport;
}

int ScrollStitcher::overlapFor(const QImage &previous, const QImage &next, const QRect &viewport) const
{
    const int probe_height = qMin(PROBE_HEIGHT, qMax(8, viewport.height() / 3));
    const int max_shift = viewport.height() - probe_height;
    int       best_shift = 0;
    qreal     best_cost = std::numeric_limits<qreal>::max();
    for (int shift = 1; shift <= max_shift; ++shift) {
        qreal cost = 0.0;
        for (int row = 0; row < probe_height; ++row) {
            cost += rowDifference(previous,
                                  viewport.top() + shift + row,
                                  next,
                                  viewport.top() + row,
                                  viewport.left(),
                                  viewport.width());
        }
        cost /= probe_height;
        if (cost < best_cost) {
            best_cost = cost;
            best_shift = shift;
        }
    }
    return best_cost <= MATCH_TOLERANCE ? best_shift : 0;
}

void ScrollStitcher::appendRows(const QImage &frame, const QRect &viewport, int added_rows)
{
    const int next_height = qMin(MAX_STITCH_HEIGHT, m_image.height() + added_rows);
    if (next_height <= m_image.height()) {
        return;
    }

    QImage   combined(m_image.width(), next_height, QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&combined);
    painter.drawImage(0, 0, m_image);
    painter.drawImage(QPoint(0, m_image.height()),
                      frame,
                      QRect(viewport.left(), viewport.bottom() - added_rows + 1, viewport.width(), added_rows));
    painter.end();
    m_image = std::move(combined);
}
