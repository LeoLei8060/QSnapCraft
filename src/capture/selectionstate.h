#pragma once

#include <QFlags>
#include <QPoint>
#include <QRect>
#include <QVector>

enum class SelectionEdge { kNone = 0x0, kLeft = 0x1, kTop = 0x2, kRight = 0x4, kBottom = 0x8, kMove = 0x10 };
Q_DECLARE_FLAGS(SelectionEdges, SelectionEdge)
Q_DECLARE_OPERATORS_FOR_FLAGS(SelectionEdges)

class SelectionState
{
public:
    explicit SelectionState(const QRect &bounds = {});

    void  setBounds(const QRect &bounds);
    QRect bounds() const;

    void  setSelection(const QRect &selection);
    QRect current() const;
    bool  isValid() const;
    void  clear();

    void beginDrag(const QPoint &point);
    void dragTo(const QPoint &point);
    void endDrag();

    void beginResize(SelectionEdges edges, bool fromCenter);
    void resizeTo(const QPoint &point);
    void endResize();

    void beginMove(const QPoint &point);
    void moveTo(const QPoint &point);
    void moveBy(const QPoint &delta);
    void endMove();

    void adjustByKeyboard(int key, bool shrink, bool enlarge, bool fromCenter);

private:
    QRect clampToBounds(const QRect &selection) const;

    QRect          m_bounds;
    QRect          m_selection;
    QPoint         m_dragStart;
    QPoint         m_moveStartPoint;
    QRect          m_moveStartSelection;
    QPoint         m_resizeStartPoint;
    QRect          m_resizeStartSelection;
    SelectionEdges m_resizeEdges;
    bool           m_isDragging{false};
    bool           m_isMoving{false};
    bool           m_isResizing{false};
    bool           m_resizeFromCenter{false};
};

class SelectionHistory
{
public:
    explicit SelectionHistory(int maxAreas = 30, bool loop = true);

    void setLimit(int maxAreas);
    int  limit() const;
    void setLoop(bool loop);
    bool loop() const;

    void  push(const QRect &selection);
    void  clear();
    int   count() const;
    QRect current() const;
    bool  next();
    bool  previous();

private:
    int wrapIndex(int index) const;

    QVector<QRect> m_areas;
    int            m_cursor{-1};
    int            m_limit{30};
    bool           m_loop{true};
};
