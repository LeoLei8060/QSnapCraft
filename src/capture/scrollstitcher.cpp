#include "scrollstitcher.h"

#include <QPainter>
#include <QtMath>

#include <algorithm>
#include <cstring>
#include <limits>

namespace {
constexpr int   PROBE_HEIGHT = 64;
constexpr int   PROBE_COLUMN_STEP = 4;
constexpr qreal MATCH_TOLERANCE = 6.0;
constexpr int   MAX_STITCH_HEIGHT = 100000;
constexpr int   MAX_STITCH_WIDTH = 100000;

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

qreal columnDifference(
    const QImage &previous, int previous_column, const QImage &next, int next_column, int top, int height)
{
    qint64 difference = 0;
    int    samples = 0;
    for (int y = top; y < top + height; y += PROBE_COLUMN_STEP) {
        const QRgb previousPixel = previous.pixel(previous_column, y);
        const QRgb nextPixel = next.pixel(next_column, y);
        difference += qAbs(qRed(previousPixel) - qRed(nextPixel));
        difference += qAbs(qGreen(previousPixel) - qGreen(nextPixel));
        difference += qAbs(qBlue(previousPixel) - qBlue(nextPixel));
        samples += 3;
    }
    return samples > 0 ? qreal(difference) / samples : 0.0;
}

bool rowsMatch(const QImage &image, int first_row, int second_row)
{
    if (first_row < 0 || second_row < 0 || first_row >= image.height() || second_row >= image.height()) {
        return false;
    }
    const int row_bytes = image.width() * static_cast<int>(image.depth() / 8);
    return std::memcmp(image.constScanLine(first_row), image.constScanLine(second_row), row_bytes) == 0;
}

bool columnsMatch(const QImage &image, int first_column, int second_column)
{
    if (first_column < 0 || second_column < 0 || first_column >= image.width() || second_column >= image.width()) {
        return false;
    }
    for (int y = 0; y < image.height(); ++y) {
        if (image.pixel(first_column, y) != image.pixel(second_column, y)) {
            return false;
        }
    }
    return true;
}
} // namespace

void ScrollStitcher::reset(const QImage &first_frame)
{
    m_image = first_frame.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    m_previousFrame = m_image;
    m_viewport = m_image.rect();
    m_viewportDetected = false;
    m_axis = ScrollAxis::kUnknown;
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
        QRect changedViewport = changedRect(m_previousFrame, next);
        if (!changedViewport.isValid()) {
            return false;
        }

        const qreal area_ratio = qreal(changedViewport.width() * changedViewport.height())
                                 / qreal(next.width() * next.height());
        if (area_ratio > 0.9) {
            changedViewport = next.rect();
        }

        QRect verticalViewport = changedViewport;
        verticalViewport.setBottom(qMin(next.height() - 1, verticalViewport.bottom() + 1));
        QRect horizontalViewport = changedViewport;
        horizontalViewport.setRight(qMin(next.width() - 1, horizontalViewport.right() + 1));

        const OverlapMatch vertical = verticalOverlapFor(m_previousFrame, next, verticalViewport);
        const OverlapMatch horizontal = horizontalOverlapFor(m_previousFrame, next, horizontalViewport);
        if (vertical.cost < 0.0 && horizontal.cost < 0.0) {
            return false;
        }
        if (horizontal.cost >= 0.0 && (vertical.cost < 0.0 || horizontal.cost < vertical.cost)) {
            m_axis = ScrollAxis::kHorizontal;
            m_viewport = changedViewport;
        } else {
            m_axis = ScrollAxis::kVertical;
            m_viewport = changedViewport;
        }

        const bool isVertical = m_axis == ScrollAxis::kVertical;
        const int  trailing_edge = isVertical ? m_viewport.bottom() : m_viewport.right();
        const int  frame_limit = isVertical ? next.height() - 1 : next.width() - 1;
        if (trailing_edge + 2 <= frame_limit) {
            const bool nextLineIsScrollable = isVertical ? !rowsMatch(next, trailing_edge + 1, trailing_edge + 2)
                                                         : !columnsMatch(next, trailing_edge + 1, trailing_edge + 2);
            if (nextLineIsScrollable) {
                if (isVertical) {
                    m_viewport.setBottom(trailing_edge + 1);
                } else {
                    m_viewport.setRight(trailing_edge + 1);
                }
            }
        }
        m_viewport = m_viewport.intersected(next.rect());
        if (m_viewport.width() < 4 || m_viewport.height() < 4) {
            return false;
        }
        m_image = m_previousFrame.copy(m_viewport);
        m_viewportDetected = true;
    }

    const QImage previous_view = m_previousFrame.copy(m_viewport);
    const QImage next_view = next.copy(m_viewport);

    if (m_axis == ScrollAxis::kUnknown) {
        const OverlapMatch vertical = verticalOverlapFor(previous_view, next_view, next_view.rect());
        const OverlapMatch horizontal = horizontalOverlapFor(previous_view, next_view, next_view.rect());
        if (vertical.cost < 0.0 && horizontal.cost < 0.0) {
            return false;
        }
        if (horizontal.cost >= 0.0 && (vertical.cost < 0.0 || horizontal.cost < vertical.cost)) {
            m_axis = ScrollAxis::kHorizontal;
        } else {
            m_axis = ScrollAxis::kVertical;
        }
    }

    int addedPixels = 0;
    if (m_axis == ScrollAxis::kHorizontal) {
        const OverlapMatch horizontal = horizontalOverlapFor(previous_view, next_view, next_view.rect());
        if (horizontal.offset <= 0 || horizontal.offset >= next_view.width()) {
            return false;
        }
        appendColumns(next, m_viewport, horizontal.offset);
        addedPixels = horizontal.offset;
    } else {
        const OverlapMatch vertical = verticalOverlapFor(previous_view, next_view, next_view.rect());
        if (vertical.offset <= 0 || vertical.offset >= next_view.height()) {
            return false;
        }
        appendRows(next, m_viewport, vertical.offset);
        addedPixels = vertical.offset;
    }

    m_previousFrame = next;
    if (added_rows != nullptr) {
        *added_rows = addedPixels;
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

ScrollStitcher::OverlapMatch ScrollStitcher::verticalOverlapFor(const QImage &previous,
                                                                const QImage &next,
                                                                const QRect  &viewport) const
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
    return best_cost <= MATCH_TOLERANCE ? OverlapMatch{best_shift, best_cost} : OverlapMatch{};
}

ScrollStitcher::OverlapMatch ScrollStitcher::horizontalOverlapFor(const QImage &previous,
                                                                  const QImage &next,
                                                                  const QRect  &viewport) const
{
    const int probeWidth = qMin(PROBE_HEIGHT, qMax(8, viewport.width() / 3));
    const int maxShift = viewport.width() - probeWidth;
    int       bestShift = 0;
    qreal     bestCost = std::numeric_limits<qreal>::max();
    for (int shift = 1; shift <= maxShift; ++shift) {
        qreal cost = 0.0;
        for (int column = 0; column < probeWidth; ++column) {
            cost += columnDifference(previous,
                                     viewport.left() + shift + column,
                                     next,
                                     viewport.left() + column,
                                     viewport.top(),
                                     viewport.height());
        }
        cost /= probeWidth;
        if (cost < bestCost) {
            bestCost = cost;
            bestShift = shift;
        }
    }
    return bestCost <= MATCH_TOLERANCE ? OverlapMatch{bestShift, bestCost} : OverlapMatch{};
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

void ScrollStitcher::appendColumns(const QImage &frame, const QRect &viewport, int added_columns)
{
    const int nextWidth = qMin(MAX_STITCH_WIDTH, m_image.width() + added_columns);
    if (nextWidth <= m_image.width()) {
        return;
    }

    QImage   combined(nextWidth, m_image.height(), QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&combined);
    painter.drawImage(0, 0, m_image);
    painter.drawImage(QPoint(m_image.width(), 0),
                      frame,
                      QRect(viewport.right() - added_columns + 1, viewport.top(), added_columns, viewport.height()));
    painter.end();
    m_image = std::move(combined);
}
