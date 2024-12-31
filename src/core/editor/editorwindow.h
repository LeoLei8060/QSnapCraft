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
    QString getSaveFilePath();  // 获取保存文件路径
    void saveImage();          // 保存图像

    QImage m_currentImage;
    QRect  m_captureRect;
    Toolbar m_toolbar;
    QMap<QScreen*, QRect> m_screenGeometries;
    QRect m_totalGeometry;
    QPoint m_dragStartPos;
    ResizeHandle m_currentHandle{ResizeHandle::None};
    bool m_isDragging{false};
    bool m_modified{false};    // 图像是否被修改
    Magnifier m_magnifier;     // 放大镜
};
