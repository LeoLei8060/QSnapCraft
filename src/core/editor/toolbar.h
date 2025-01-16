#pragma once

#include "utils/FontManager.h"
#include <QVector>
#include <QWidget>
#include <QToolButton>

class Toolbar : public QWidget
{
    Q_OBJECT

public:
    enum Tool {
        MoveBtn,
        RectangleBtn,
        EllipseBtn,
        PolyLineBtn,
        ArrowBtn,
        PencilBtn,
        MarkerBtn,
        MosaicBtn,
        TextBtn,
        Eraser,
        Undo,
        Redo,
        Pin,
        Exit,
        Save,
        Copy,
        ColorBtn
    };

    explicit Toolbar(QWidget *parent = nullptr);
    ~Toolbar() override = default;

    QToolButton *getColorButton() const { return m_colorButton; }
    void setCurrentTool(Tool tool);  // 新增：设置当前工具

signals:
    void toolSelected(Tool tool);
    void colorButtonClicked(const QPoint &pos);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void         initializeUI();
    QToolButton *createToolButton(Tool tool, const QString &iconText, const QString &tooltip);
    QString      getIconChar(Tool tool) const;
    bool         isCheckableTool(Tool tool) const;  // 新增：判断工具是否需要显示选中状态

    QVector<QToolButton *> m_buttons;
    Tool                   m_currentTool{Tool::MoveBtn};  // 新增：当前选中的工具
    QColor                m_currentColor{Qt::black};
    QToolButton          *m_colorButton{nullptr};
};
