#pragma once

#include "utils/magnifier.h"
#include <QWidget>

class EditorWindow : public QWidget
{
    Q_OBJECT

public:
    // 调整手柄的位置枚举
    enum class ResizeHandle {
        None,
        TopLeft,
        Top,
        TopRight,
        Right,
        BottomRight,
        Bottom,
        BottomLeft,
        Left,
        Move
    };

    explicit EditorWindow(QWidget *parent = nullptr);
    ~EditorWindow() override = default;

    void setImage(const QImage &image);
    void setData(const QImage &image, const QRect &captureRect);

protected:
    void paintEvent(QPaintEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    ResizeHandle hitTest(const QPoint &pos) const;
    void updateCursor(ResizeHandle handle);
    void adjustRect(const QPoint &pos);

    QImage m_currentImage;
    QRect  m_captureRect;

    Magnifier m_magnifier;

    bool m_isResizing{false};
    QPoint m_dragStartPos;
    ResizeHandle m_currentHandle{ResizeHandle::None};
};
