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
    int  overlapFor(const QImage &previous, const QImage &next, const QRect &viewport) const;
    void appendRows(const QImage &frame, const QRect &viewport, int added_rows);

    QImage m_image;
    QImage m_previousFrame;
    QRect  m_viewport;
    bool   m_viewportDetected{false};
};
