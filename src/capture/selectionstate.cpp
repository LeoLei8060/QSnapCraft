#include "selectionstate.h"

#include <Qt>

#include <algorithm>

SelectionState::SelectionState(const QRect &bounds)
    : m_bounds(bounds.normalized())
{}

void SelectionState::setBounds(const QRect &bounds)
{
    m_bounds = bounds.normalized();
    m_selection = clampToBounds(m_selection);
}

QRect SelectionState::bounds() const
{
    return m_bounds;
}

void SelectionState::setSelection(const QRect &selection)
{
    m_selection = clampToBounds(selection.normalized());
}

QRect SelectionState::current() const
{
    return m_selection;
}

bool SelectionState::isValid() const
{
    return m_selection.isValid() && m_selection.width() >= 2 && m_selection.height() >= 2;
}

void SelectionState::clear()
{
    m_selection = {};
    m_isDragging = false;
    m_isMoving = false;
    m_isResizing = false;
}

void SelectionState::beginDrag(const QPoint &point)
{
    m_dragStart = point;
    m_selection = clampToBounds(QRect(point, QSize(2, 2)));
    m_isDragging = true;
}

void SelectionState::dragTo(const QPoint &point)
{
    if (!m_isDragging) {
        return;
    }
    const QPoint topLeft(qMin(m_dragStart.x(), point.x()), qMin(m_dragStart.y(), point.y()));
    const QPoint bottomRight(qMax(m_dragStart.x(), point.x()), qMax(m_dragStart.y(), point.y()));
    m_selection = clampToBounds(
        QRect(topLeft, QSize(bottomRight.x() - topLeft.x() + 1, bottomRight.y() - topLeft.y() + 1)));
}

void SelectionState::endDrag()
{
    m_isDragging = false;
}

void SelectionState::beginResize(SelectionEdges edges, bool fromCenter)
{
    if (!m_selection.isValid() || edges == SelectionEdge::kNone) {
        return;
    }
    m_resizeEdges = edges;
    m_resizeFromCenter = fromCenter;
    m_resizeStartSelection = m_selection;
    m_resizeStartPoint = edges.testFlag(SelectionEdge::kLeft)    ? m_selection.topLeft()
                         : edges.testFlag(SelectionEdge::kRight) ? m_selection.topRight()
                         : edges.testFlag(SelectionEdge::kTop)   ? m_selection.topLeft()
                                                                 : m_selection.bottomLeft();
    m_isResizing = true;
}

void SelectionState::resizeTo(const QPoint &point)
{
    if (!m_isResizing) {
        return;
    }

    const QPoint delta = point - m_resizeStartPoint;
    QRect        next = m_resizeStartSelection;
    if (m_resizeEdges.testFlag(SelectionEdge::kLeft)) {
        next.setLeft(m_resizeStartSelection.left() + delta.x());
        if (m_resizeFromCenter) {
            next.setRight(m_resizeStartSelection.right() - delta.x());
        }
    }
    if (m_resizeEdges.testFlag(SelectionEdge::kRight)) {
        next.setRight(m_resizeStartSelection.right() + delta.x());
        if (m_resizeFromCenter) {
            next.setLeft(m_resizeStartSelection.left() - delta.x());
        }
    }
    if (m_resizeEdges.testFlag(SelectionEdge::kTop)) {
        next.setTop(m_resizeStartSelection.top() + delta.y());
        if (m_resizeFromCenter) {
            next.setBottom(m_resizeStartSelection.bottom() - delta.y());
        }
    }
    if (m_resizeEdges.testFlag(SelectionEdge::kBottom)) {
        next.setBottom(m_resizeStartSelection.bottom() + delta.y());
        if (m_resizeFromCenter) {
            next.setTop(m_resizeStartSelection.top() - delta.y());
        }
    }

    next = clampToBounds(next.normalized());
    if (next.width() >= 2 && next.height() >= 2) {
        m_selection = next;
    }
}

void SelectionState::endResize()
{
    m_isResizing = false;
    m_resizeEdges = SelectionEdge::kNone;
    m_resizeFromCenter = false;
}

void SelectionState::beginMove(const QPoint &point)
{
    if (!m_selection.isValid()) {
        return;
    }
    m_moveStartPoint = point;
    m_moveStartSelection = m_selection;
    m_isMoving = true;
}

void SelectionState::moveTo(const QPoint &point)
{
    if (!m_isMoving) {
        return;
    }
    QRect next = m_moveStartSelection;
    next.translate(point - m_moveStartPoint);
    if (next.left() < m_bounds.left())
        next.moveLeft(m_bounds.left());
    if (next.right() > m_bounds.right())
        next.moveRight(m_bounds.right());
    if (next.top() < m_bounds.top())
        next.moveTop(m_bounds.top());
    if (next.bottom() > m_bounds.bottom())
        next.moveBottom(m_bounds.bottom());
    m_selection = next;
}

void SelectionState::moveBy(const QPoint &delta)
{
    if (!m_selection.isValid()) {
        return;
    }
    QRect next = m_selection;
    next.translate(delta);
    if (next.left() < m_bounds.left())
        next.moveLeft(m_bounds.left());
    if (next.right() > m_bounds.right())
        next.moveRight(m_bounds.right());
    if (next.top() < m_bounds.top())
        next.moveTop(m_bounds.top());
    if (next.bottom() > m_bounds.bottom())
        next.moveBottom(m_bounds.bottom());
    m_selection = next;
}

void SelectionState::endMove()
{
    m_isMoving = false;
}

void SelectionState::adjustByKeyboard(int key, bool shrink, bool enlarge, bool fromCenter)
{
    QRect area = m_selection;
    if (!area.isValid()) {
        return;
    }

    if (!shrink && !enlarge) {
        if (key == Qt::Key_Left)
            area.translate(-1, 0);
        if (key == Qt::Key_Right)
            area.translate(1, 0);
        if (key == Qt::Key_Up)
            area.translate(0, -1);
        if (key == Qt::Key_Down)
            area.translate(0, 1);
    } else {
        const int amount = shrink ? 1 : -1;
        if (key == Qt::Key_Left)
            area.adjust(amount, 0, fromCenter ? -amount : 0, 0);
        if (key == Qt::Key_Right)
            area.adjust(fromCenter ? amount : 0, 0, -amount, 0);
        if (key == Qt::Key_Up)
            area.adjust(0, amount, 0, fromCenter ? -amount : 0);
        if (key == Qt::Key_Down)
            area.adjust(0, fromCenter ? amount : 0, 0, -amount);
    }

    area = clampToBounds(area.normalized());
    if (area.width() >= 2 && area.height() >= 2) {
        m_selection = area;
    }
}

QRect SelectionState::clampToBounds(const QRect &selection) const
{
    if (!selection.isValid() || !m_bounds.isValid()) {
        return {};
    }
    const QRect intersection = selection.intersected(m_bounds);
    return intersection.width() >= 1 && intersection.height() >= 1 ? intersection : QRect();
}

SelectionHistory::SelectionHistory(int maxAreas, bool loop)
    : m_limit(std::max(1, maxAreas))
    , m_loop(loop)
{}

void SelectionHistory::setLimit(int maxAreas)
{
    m_limit = std::max(1, maxAreas);
    while (m_areas.size() > m_limit) {
        m_areas.removeFirst();
    }
    m_cursor = m_areas.isEmpty() ? -1 : m_areas.size() - 1;
}

int SelectionHistory::limit() const
{
    return m_limit;
}

void SelectionHistory::setLoop(bool loop)
{
    m_loop = loop;
}

bool SelectionHistory::loop() const
{
    return m_loop;
}

void SelectionHistory::push(const QRect &selection)
{
    if (!selection.isValid()) {
        return;
    }
    if (!m_areas.isEmpty() && m_areas.last() == selection) {
        m_cursor = m_areas.size() - 1;
        return;
    }

    m_areas.append(selection);
    while (m_areas.size() > m_limit) {
        m_areas.removeFirst();
    }
    m_cursor = m_areas.size() - 1;
}

void SelectionHistory::clear()
{
    m_areas.clear();
    m_cursor = -1;
}

int SelectionHistory::count() const
{
    return m_areas.size();
}

QRect SelectionHistory::current() const
{
    return m_cursor >= 0 && m_cursor < m_areas.size() ? m_areas.at(m_cursor) : QRect();
}

bool SelectionHistory::next()
{
    if (m_areas.isEmpty()) {
        return false;
    }
    const int nextIndex = m_cursor + 1;
    if (nextIndex >= m_areas.size()) {
        if (!m_loop) {
            return false;
        }
        m_cursor = 0;
    } else {
        m_cursor = nextIndex;
    }
    return true;
}

bool SelectionHistory::previous()
{
    if (m_areas.isEmpty()) {
        return false;
    }
    const int previousIndex = m_cursor - 1;
    if (previousIndex < 0) {
        if (!m_loop) {
            return false;
        }
        m_cursor = m_areas.size() - 1;
    } else {
        m_cursor = previousIndex;
    }
    return true;
}

int SelectionHistory::wrapIndex(int index) const
{
    if (m_areas.isEmpty()) {
        return -1;
    }
    return ((index % m_areas.size()) + m_areas.size()) % m_areas.size();
}
