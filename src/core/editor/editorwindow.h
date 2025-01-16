#pragma once

#include "toolbar.h"
#include "utils/magnifier.h"
#include <memory>
#include <stack>
#include <vector>
#include <QColor>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMap>
#include <QWidget>

// 前向声明
class Shape;
class Polyline;
class Ellipse;
class Rectangle;
class Arrow;
class Freehand;
class Highlighter;
class Mosaic;
class Text;

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

    enum DrawMode {
        DrawPolyline,
        DrawEllipse,
        DrawRectangle,
        DrawArrow,
        DrawFreehand,
        DrawHighlighter,
        DrawMosaic,
        DrawEraser,
        DrawTexts,
        Move
    };

    explicit EditorWindow(QWidget *parent = nullptr);
    ~EditorWindow() override = default;

    void start(const QPixmap &image, const QRect &captureRect);

    void hideWindow();

    void setDrawMode(int mode);
    void setPenColor(const QColor &color); // 添加设置画笔颜色的方法
    void undo();
    void redo();

signals:
    void sigEditorFinished();
    void sigCancelEditor();
    void sigPinImage(const QPixmap &pixmap, const QRect &rect);

protected:
    void     paintEvent(QPaintEvent *event) override;
    void     mouseMoveEvent(QMouseEvent *event) override;
    void     mousePressEvent(QMouseEvent *event) override;
    void     mouseReleaseEvent(QMouseEvent *event) override;
    void     keyPressEvent(QKeyEvent *event) override;
    void     inputMethodEvent(QInputMethodEvent *event) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;

private slots:
    void onToolSelected(Toolbar::Tool tool);

private:
    ResizeHandle hitTest(const QPoint &pos) const;
    void         updateCursor(ResizeHandle handle);
    void         adjustRect(const QPoint &pos);
    void         updateToolbarPosition();
    QString      getSaveFilePath(); // 获取保存文件路径
    void         editorFinished();
    void         updateColorButton(); // 更新颜色按钮样式

    void saveImage(); // 保存图像
    void copyImage(); // 复制
    void pinImage();  // pin
    void shapesToImage(QPixmap &img);

    void createShape(const QPoint &pos);
    void handleTextInput(const QPoint &pos);
    void finishCurrentShape();
    void removeCurrentShape();

    QPixmap                m_screenshotPixmap;
    QRect                  m_captureRect;
    QRect                  m_totalGeometry;
    QMap<QScreen *, QRect> m_screenGeometries;
    Toolbar                m_toolbar;
    QPoint                 m_dragStartPos;
    ResizeHandle           m_currentHandle{ResizeHandle::None};
    bool                   m_isDragging{false};
    bool                   m_modified{false}; // 图像是否被修改
    //    Magnifier              m_magnifier;       // 放大镜

    DrawMode                            m_currentMode{DrawMode::Move};
    std::unique_ptr<Shape>              m_currentShape;
    std::vector<std::unique_ptr<Shape>> m_shapes;
    std::vector<std::unique_ptr<Shape>> m_redoStack;
    int                                 m_mosaicType = 0; // 0 for Rectangle, 1 for Circle
    QString                             m_preeditString;  // 用于存储输入法的预编辑文本
    QColor                              m_penColor;       // 添加画笔颜色成员变量
    QImage                              m_paintImg;

    static QColor s_lastUsedColor; // 静态成员，保存上次使用的颜色
};
