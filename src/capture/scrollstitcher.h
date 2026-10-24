#pragma once

#include <QImage>
#include <QRect>

class ScrollStitcher
{
public:
    void reset(const QImage &first_frame);
    bool appendFrame(const QImage &next_frame, int *added_rows = nullptr);

    const QImage &image() const;
    QRect         viewport() const;

private:
    enum class ScrollAxis { kUnknown, kVertical, kHorizontal };

    struct OverlapMatch
    {
        int   offset{0};
        qreal cost{-1.0};
    };

    OverlapMatch verticalOverlapFor(const QImage &previous, const QImage &next, const QRect &viewport) const;
    OverlapMatch horizontalOverlapFor(const QImage &previous, const QImage &next, const QRect &viewport) const;
    void         appendRows(const QImage &frame, const QRect &viewport, int added_rows);
    void         appendColumns(const QImage &frame, const QRect &viewport, int added_columns);

    QImage     m_image;
    QImage     m_previousFrame;
    QRect      m_viewport;
    ScrollAxis m_axis{ScrollAxis::kUnknown};
    bool       m_viewportDetected{false};
};
