#pragma once

#include "utils/magnifier.h"
#include "toolbar.h"
#include <QWidget>
#include <QMap>

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

private slots:
    void onToolSelected(Toolbar::Tool tool);

private:
    ResizeHandle hitTest(const QPoint &pos) const;
    void updateCursor(ResizeHandle handle);
    void adjustRect(const QPoint &pos);
    void updateToolbarPosition();

    QImage m_currentImage;
    QRect  m_captureRect;

    Magnifier m_magnifier;
    Toolbar   m_toolbar;

    bool m_isResizing{false};
    QPoint m_dragStartPos;
    ResizeHandle m_currentHandle{ResizeHandle::None};

    QMap<QScreen*, QRect> m_screenGeometries;  // 存储所有屏幕的几何信息
    QRect m_totalGeometry;  // 所有屏幕的总范围
};
