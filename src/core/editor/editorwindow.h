#pragma once

#include "toolbar.h"
#include "utils/magnifier.h"
#include <QMap>
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

    void start(const QPixmap &image, const QRect &captureRect);

    void hideWindow();

signals:
    void sigEditorFinished();
    void sigCancelEditor();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onToolSelected(Toolbar::Tool tool);

private:
    ResizeHandle hitTest(const QPoint &pos) const;
    void         updateCursor(ResizeHandle handle);
    void         adjustRect(const QPoint &pos);
    void         updateToolbarPosition();
    QString      getSaveFilePath(); // 获取保存文件路径
    void         saveImage();       // 保存图像

    QPixmap                m_screenshotPixmap;
    QRect                  m_captureRect;
    Toolbar                m_toolbar;
    QMap<QScreen *, QRect> m_screenGeometries;
    QRect                  m_totalGeometry;
    QPoint                 m_dragStartPos;
    ResizeHandle           m_currentHandle{ResizeHandle::None};
    bool                   m_isDragging{false};
    bool                   m_modified{false}; // 图像是否被修改
    //    Magnifier              m_magnifier;       // 放大镜
};
