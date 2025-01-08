#pragma once

#include "utils/FontManager.h"
#include <QVector>
#include <QWidget>

class QToolButton;

class Toolbar : public QWidget
{
    Q_OBJECT

public:
    enum Tool {
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
        Copy
    };

    explicit Toolbar(QWidget *parent = nullptr);
    ~Toolbar() override = default;

signals:
    void toolSelected(Tool tool);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void         initializeUI();
    QToolButton *createToolButton(Tool tool, const QString &iconText, const QString &tooltip);
    QString      getIconChar(Tool tool) const;

    QVector<QToolButton *> m_buttons;
    Tool                   m_currentTool{Tool::RectangleBtn};
};
