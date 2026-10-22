#include "paintscene.h"

#include "paintitem.h"
#include "painttextitem.h"
#include "paintwidths.h"

#include <QFont>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QKeyEvent>
#include <QLineF>
#include <QPainter>
#include <QPainterPathStroker>
#include <QTextDocument>
#include <QUndoStack>
#include <QVariant>

#include <algorithm>
#include <cstddef>

namespace {
constexpr int EFFECT_TILES_PER_TICK = 8;

enum class PaintProperty {
    kColor,
    kPenWidth,
    kFilled,
    kArrowStyle,
    kMarkerOpacity,
    kEffectMode,
    kEffectLevel,
    kFontFamily,
    kFontSize,
    kFontBold,
    kFontItalic,
    kTextOutlined,
    kTextBackground
};

QVariant itemPropertyValue(QGraphicsItem *item, PaintProperty property)
{
    if (auto *paint_item = dynamic_cast<PaintItem *>(item)) {
        switch (property) {
        case PaintProperty::kColor:
            return paint_item->color();
        case PaintProperty::kPenWidth:
            return paint_item->penWidth();
        case PaintProperty::kFilled:
            return paint_item->isFilled();
        case PaintProperty::kArrowStyle:
            return paint_item->tool() == PaintTool::kArrow
                       ? QVariant::fromValue(static_cast<int>(paint_item->arrowStyle()))
                       : QVariant();
        case PaintProperty::kMarkerOpacity:
            return paint_item->tool() == PaintTool::kMarker ? paint_item->markerOpacity() : QVariant();
        case PaintProperty::kEffectMode:
            return paint_item->tool() == PaintTool::kMosaic
                       ? QVariant::fromValue(static_cast<int>(paint_item->effectMode()))
                       : QVariant();
        case PaintProperty::kEffectLevel:
            return paint_item->tool() == PaintTool::kMosaic ? paint_item->effectLevel() : QVariant();
        default:
            return {};
        }
    }
    if (auto *text_item = dynamic_cast<PaintTextItem *>(item)) {
        switch (property) {
        case PaintProperty::kColor:
            return text_item->textColor();
        case PaintProperty::kFontFamily:
            return text_item->font().family();
        case PaintProperty::kFontSize:
            return text_item->font().pixelSize();
        case PaintProperty::kFontBold:
            return text_item->font().bold();
        case PaintProperty::kFontItalic:
            return text_item->font().italic();
        case PaintProperty::kTextOutlined:
            return text_item->isOutlined();
        case PaintProperty::kTextBackground:
            return text_item->hasBackground();
        default:
            return {};
        }
    }
    return {};
}

void applyItemProperty(QGraphicsItem *item, PaintProperty property, const QVariant &value)
{
    if (auto *paint_item = dynamic_cast<PaintItem *>(item)) {
        switch (property) {
        case PaintProperty::kColor:
            paint_item->setColor(value.value<QColor>());
            break;
        case PaintProperty::kPenWidth:
            paint_item->setPenWidth(value.toReal());
            break;
        case PaintProperty::kFilled:
            paint_item->setFilled(value.toBool());
            break;
        case PaintProperty::kArrowStyle:
            paint_item->setArrowStyle(static_cast<ArrowStyle>(value.toInt()));
            break;
        case PaintProperty::kMarkerOpacity:
            paint_item->setMarkerOpacity(value.toInt());
            break;
        case PaintProperty::kEffectMode:
            paint_item->setEffectMode(static_cast<PaintEffectMode>(value.toInt()));
            break;
        case PaintProperty::kEffectLevel:
            paint_item->setEffectLevel(value.toInt());
            break;
        default:
            break;
        }
        return;
    }

    if (auto *text_item = dynamic_cast<PaintTextItem *>(item)) {
        switch (property) {
        case PaintProperty::kColor:
            text_item->setTextColor(value.value<QColor>());
            break;
        case PaintProperty::kFontFamily: {
            QFont font = text_item->font();
            font.setFamily(value.toString());
            text_item->setFont(font);
            text_item->update();
            break;
        }
        case PaintProperty::kFontSize: {
            QFont font = text_item->font();
            font.setPixelSize(qMax(6, value.toInt()));
            text_item->setFont(font);
            text_item->update();
            break;
        }
        case PaintProperty::kFontBold: {
            QFont font = text_item->font();
            font.setBold(value.toBool());
            text_item->setFont(font);
            text_item->update();
            break;
        }
        case PaintProperty::kFontItalic: {
            QFont font = text_item->font();
            font.setItalic(value.toBool());
            text_item->setFont(font);
            text_item->update();
            break;
        }
        case PaintProperty::kTextOutlined:
            text_item->setOutlined(value.toBool());
            break;
        case PaintProperty::kTextBackground:
            text_item->setBackgroundEnabled(value.toBool());
            break;
        default:
            break;
        }
    }
}

struct PropertyChange
{
    QGraphicsItem *item;
    QVariant       before;
    QVariant       after;
};

class ChangeItemPropertyCommand : public QUndoCommand
{
public:
    ChangeItemPropertyCommand(PaintProperty property, QVector<PropertyChange> changes, const QString &text)
        : QUndoCommand(text)
        , m_property(property)
        , m_changes(std::move(changes))
    {}

    void undo() override
    {
        for (const PropertyChange &change : m_changes) {
            applyItemProperty(change.item, m_property, change.before);
        }
    }

    void redo() override
    {
        for (const PropertyChange &change : m_changes) {
            applyItemProperty(change.item, m_property, change.after);
        }
    }

private:
    PaintProperty           m_property;
    QVector<PropertyChange> m_changes;
};

class AddPaintItemCommand : public QUndoCommand
{
public:
    AddPaintItemCommand(PaintScene *scene, QGraphicsItem *item, const QString &text)
        : QUndoCommand(text)
        , m_item(item)
        , m_skipFirstRedo(true)
    {
        Q_UNUSED(scene)
    }

    void undo() override { m_item->hide(); }
    void redo() override
    {
        if (m_skipFirstRedo) {
            m_skipFirstRedo = false;
            m_item->show();
            return;
        }
        m_item->show();
    }

private:
    QGraphicsItem *m_item;
    bool           m_skipFirstRedo;
};

class VisibilityCommand : public QUndoCommand
{
public:
    explicit VisibilityCommand(const QList<QGraphicsItem *> &items, const QString &text)
        : QUndoCommand(text)
        , m_items(items)
    {}

    void undo() override
    {
        for (QGraphicsItem *item : m_items) {
            item->show();
        }
    }

    void redo() override
    {
        for (QGraphicsItem *item : m_items) {
            item->hide();
            item->setSelected(false);
        }
    }

private:
    QList<QGraphicsItem *> m_items;
};

class MoveItemsCommand : public QUndoCommand
{
public:
    MoveItemsCommand(QVector<QPair<QGraphicsItem *, QPointF>> starts, QVector<QPair<QGraphicsItem *, QPointF>> ends)
        : QUndoCommand(QStringLiteral("移动标注"))
        , m_starts(std::move(starts))
        , m_ends(std::move(ends))
        , m_skipFirstRedo(true)
    {}

    void undo() override
    {
        for (const auto &entry : m_starts) {
            entry.first->setPos(entry.second);
        }
    }

    void redo() override
    {
        if (m_skipFirstRedo) {
            m_skipFirstRedo = false;
            return;
        }
        for (const auto &entry : m_ends) {
            entry.first->setPos(entry.second);
        }
    }

private:
    QVector<QPair<QGraphicsItem *, QPointF>> m_starts;
    QVector<QPair<QGraphicsItem *, QPointF>> m_ends;
    bool                                     m_skipFirstRedo;
};

class ScaleItemCommand : public QUndoCommand
{
public:
    ScaleItemCommand(QGraphicsItem     *item,
                     const QTransform &startTransform,
                     const QPointF    &startPosition,
                     const QTransform &endTransform,
                     const QPointF    &endPosition)
        : QUndoCommand(QStringLiteral("缩放标注"))
        , m_item(item)
        , m_startTransform(startTransform)
        , m_startPosition(startPosition)
        , m_endTransform(endTransform)
        , m_endPosition(endPosition)
    {}

    void undo() override
    {
        m_item->setTransform(m_startTransform);
        m_item->setPos(m_startPosition);
    }

    void redo() override
    {
        m_item->setTransform(m_endTransform);
        m_item->setPos(m_endPosition);
    }

private:
    QGraphicsItem *m_item;
    QTransform     m_startTransform;
    QPointF        m_startPosition;
    QTransform     m_endTransform;
    QPointF        m_endPosition;
};
} // namespace

PaintScene::PaintScene(QObject *parent)
    : QGraphicsScene(parent)
    , m_undoStack(new QUndoStack(this))
{
    m_toolColors.fill(m_color);
    m_toolPenWidths.fill(m_penWidth);
    m_toolPenWidths[static_cast<std::size_t>(PaintTool::kMosaic)] = 14.0;
    m_fontFamily = QFont().family();
    m_effectTileTimer.setSingleShot(true);
    connect(&m_effectTileTimer, &QTimer::timeout, this, &PaintScene::processEffectTileUpdates);
    connect(m_undoStack, &QUndoStack::indexChanged, this, [this]() {
        emit sigRepaintRequested();
        scheduleEffectTileUpdate();
    });
}

PaintScene::~PaintScene()
{
    m_effectTileTimer.stop();
    m_undoStack->disconnect(this);
    m_undoStack->clear();
    for (PaintItem *item : m_clipboardItems) {
        delete item;
    }
    for (PaintTextItem *item : m_clipboardTextItems) {
        delete item;
    }
}

void PaintScene::setTool(PaintTool tool)
{
    if (tool == m_tool) {
        return;
    }
    stopItemEditing();
    m_toolColors[static_cast<std::size_t>(m_tool)] = m_color;
    m_toolPenWidths[static_cast<std::size_t>(m_tool)] = m_penWidth;
    m_tool = tool;
    m_color = m_toolColors[static_cast<std::size_t>(m_tool)];
    m_penWidth = m_toolPenWidths[static_cast<std::size_t>(m_tool)];
    for (QGraphicsItem *item : items()) {
        setItemEditingEnabled(item, false);
        if (auto *text_item = dynamic_cast<PaintTextItem *>(item)) {
            text_item->setTextInteractionFlags(Qt::NoTextInteraction);
        }
    }
    emit sigPenWidthChanged(m_penWidth);
    emit sigRepaintRequested();
}

PaintTool PaintScene::tool() const
{
    return m_tool;
}

void PaintScene::setColor(const QColor &color)
{
    if (color.isValid()) {
        m_color = color;
        m_toolColors[static_cast<std::size_t>(m_tool)] = color;
        applySelectedProperty(static_cast<int>(PaintProperty::kColor), color);
        emit sigRepaintRequested();
    }
}

QColor PaintScene::color() const
{
    return m_color;
}

void PaintScene::setPenWidth(qreal width)
{
    m_penWidth = qMax(0.5, width);
    m_toolPenWidths[static_cast<std::size_t>(m_tool)] = m_penWidth;
    applySelectedProperty(static_cast<int>(PaintProperty::kPenWidth), m_penWidth);
    emit sigPenWidthChanged(m_penWidth);
    scheduleEffectTileUpdate();
}

qreal PaintScene::penWidth() const
{
    return m_penWidth;
}

void PaintScene::setFilled(bool filled)
{
    m_filled = filled;
    applySelectedProperty(static_cast<int>(PaintProperty::kFilled), filled);
    scheduleEffectTileUpdate();
}

bool PaintScene::isFilled() const
{
    return m_filled;
}

void PaintScene::setArrowStyle(ArrowStyle style)
{
    m_arrowStyle = style;
    applySelectedProperty(static_cast<int>(PaintProperty::kArrowStyle), static_cast<int>(style));
}

ArrowStyle PaintScene::arrowStyle() const
{
    return m_arrowStyle;
}

void PaintScene::setMarkerOpacity(int opacity)
{
    m_markerOpacity = qBound(0, opacity, 255);
    applySelectedProperty(static_cast<int>(PaintProperty::kMarkerOpacity), m_markerOpacity);
}

int PaintScene::markerOpacity() const
{
    return m_markerOpacity;
}

void PaintScene::setEffectMode(PaintEffectMode mode)
{
    m_effectMode = mode;
    applySelectedProperty(static_cast<int>(PaintProperty::kEffectMode), static_cast<int>(mode));
    applySelectedProperty(static_cast<int>(PaintProperty::kEffectLevel), m_effectLevels[static_cast<std::size_t>(mode)]);
    scheduleEffectTileUpdate();
}

PaintEffectMode PaintScene::effectMode() const
{
    return m_effectMode;
}

void PaintScene::setEffectLevel(int level)
{
    m_effectLevels[static_cast<std::size_t>(m_effectMode)] = qBound(1, level, 64);
    applySelectedProperty(static_cast<int>(PaintProperty::kEffectLevel),
                          m_effectLevels[static_cast<std::size_t>(m_effectMode)]);
    scheduleEffectTileUpdate();
}

int PaintScene::effectLevel() const
{
    return m_effectLevels[static_cast<std::size_t>(m_effectMode)];
}

void PaintScene::setFontFamily(const QString &family)
{
    if (family.trimmed().isEmpty()) {
        return;
    }
    m_fontFamily = family;
    applySelectedProperty(static_cast<int>(PaintProperty::kFontFamily), family);
}

QString PaintScene::fontFamily() const
{
    return m_fontFamily;
}

void PaintScene::setFontSize(int size)
{
    m_fontSize = qMax(6, size);
    applySelectedProperty(static_cast<int>(PaintProperty::kFontSize), m_fontSize);
}

int PaintScene::fontSize() const
{
    return m_fontSize;
}

void PaintScene::setFontBold(bool bold)
{
    m_fontBold = bold;
    applySelectedProperty(static_cast<int>(PaintProperty::kFontBold), bold);
}

bool PaintScene::isFontBold() const
{
    return m_fontBold;
}

void PaintScene::setFontItalic(bool italic)
{
    m_fontItalic = italic;
    applySelectedProperty(static_cast<int>(PaintProperty::kFontItalic), italic);
}

bool PaintScene::isFontItalic() const
{
    return m_fontItalic;
}

void PaintScene::setTextOutlined(bool outlined)
{
    m_textOutlined = outlined;
    applySelectedProperty(static_cast<int>(PaintProperty::kTextOutlined), outlined);
}

bool PaintScene::isTextOutlined() const
{
    return m_textOutlined;
}

void PaintScene::setTextBackgroundEnabled(bool enabled)
{
    m_textBackground = enabled;
    applySelectedProperty(static_cast<int>(PaintProperty::kTextBackground), enabled);
}

bool PaintScene::isTextBackgroundEnabled() const
{
    return m_textBackground;
}

QUndoStack *PaintScene::undoStack() const
{
    return m_undoStack;
}

void PaintScene::clearAll()
{
    m_effectTileTimer.stop();
    m_undoStack->clear();
    m_activeItem = nullptr;
    m_editableItem = nullptr;
    m_resizeItem = nullptr;
    m_moveStarts.clear();
    clearSelection();
    clear();
    emit sigRepaintRequested();
}

void PaintScene::setBackgroundImage(const QImage &image)
{
    m_backgroundImage = image;
}

PaintItem *PaintScene::createItemAt(const QPointF &position)
{
    if (m_tool == PaintTool::kNone || m_tool == PaintTool::kSelect || m_tool == PaintTool::kText) {
        return nullptr;
    }
    stopItemEditing();
    auto *item = new PaintItem(m_tool, position, m_color, m_penWidth);
    item->setFilled(m_filled);
    item->setArrowStyle(m_arrowStyle);
    item->setMarkerOpacity(m_markerOpacity);
    item->setEffectMode(m_effectMode);
    item->setEffectLevel(m_effectMode, m_effectLevels[static_cast<std::size_t>(m_effectMode)]);
    if (m_tool == PaintTool::kEraser || m_tool == PaintTool::kMosaic) {
        QImage source = m_backgroundImage.copy();
        if (!source.isNull()) {
            const QList<QGraphicsItem *> selected = selectedItems();
            clearSelection();
            QPainter     painter(&source);
            const QRectF area = sceneRect();
            render(&painter, area, area);
            painter.end();
            for (QGraphicsItem *selected_item : selected) {
                selected_item->setSelected(true);
            }
        }
        item->setSourceImage(source);
    } else {
        item->setSourceImage(m_backgroundImage);
    }
    if (m_tool == PaintTool::kNumber) {
        m_activeNumber = m_nextNumber;
        item->setNumber(m_activeNumber);
    }
    addItem(item);
    applyItemFlags(item);
    m_activeItem = item;
    emit sigRepaintRequested();
    return item;
}

PaintTextItem *PaintScene::createTextAt(const QPointF &position)
{
    stopItemEditing();
    auto *text = new PaintTextItem();
    QFont font(m_fontFamily);
    font.setPixelSize(m_fontSize);
    font.setBold(m_fontBold);
    font.setItalic(m_fontItalic);
    text->setFont(font);
    text->setTextColor(m_color);
    text->setOutlined(m_textOutlined);
    text->setBackgroundEnabled(m_textBackground);
    text->setPos(position);
    text->setTextInteractionFlags(Qt::TextEditorInteraction);
    addItem(text);
    beginItemEditing(text);
    connect(text->document(), &QTextDocument::contentsChanged, this, [this]() { emit sigRepaintRequested(); });
    m_undoStack->push(new AddPaintItemCommand(this, text, QStringLiteral("添加文字")));
    text->setFocus();
    emit sigRepaintRequested();
    return text;
}

void PaintScene::updateActiveItem(const QPointF &position)
{
    if (m_activeItem != nullptr) {
        const QRectF previous_bounds = m_activeItem->sceneBoundingRect();
        m_activeItem->updatePoint(position);
        const QRectF current_bounds = m_activeItem->sceneBoundingRect();
        update(previous_bounds.united(current_bounds).adjusted(-2, -2, 2, 2));
        emit sigRepaintRequested();
        scheduleEffectTileUpdate();
    }
}

void PaintScene::appendActivePoint(const QPointF &position)
{
    if (m_activeItem != nullptr) {
        m_activeItem->appendPoint(position);
        emit sigRepaintRequested();
        scheduleEffectTileUpdate();
    }
}

void PaintScene::finishActiveItem()
{
    if (m_activeItem == nullptr) {
        return;
    }
    PaintItem *finished = m_activeItem;
    m_activeItem = nullptr;

    const QRectF bounds = finished->mapRectToScene(finished->boundingRect());
    if (bounds.width() < 2 && bounds.height() < 2 && finished->tool() != PaintTool::kNumber) {
        finished->hide();
        emit sigRepaintRequested();
        return;
    }
    finished->flushEffectTiles();
    m_undoStack->push(new AddPaintItemCommand(this, finished, QStringLiteral("添加标注")));
    if (finished->tool() == PaintTool::kNumber) {
        ++m_nextNumber;
    }
    beginItemEditing(finished);
    emit sigRepaintRequested();
}

int PaintScene::visibleItemCount() const
{
    int count = 0;
    for (QGraphicsItem *item : items()) {
        if (item->isVisible()) {
            ++count;
        }
    }
    return count;
}

void PaintScene::deleteSelected()
{
    const QList<QGraphicsItem *> selected = selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    m_undoStack->push(new VisibilityCommand(selected, QStringLiteral("删除标注")));
    stopItemEditing();
    emit sigSelectionChanged();
}

void PaintScene::copySelected()
{
    for (PaintItem *item : m_clipboardItems) {
        delete item;
    }
    m_clipboardItems.clear();
    for (PaintTextItem *item : m_clipboardTextItems) {
        delete item;
    }
    m_clipboardTextItems.clear();
    for (QGraphicsItem *item : selectedItems()) {
        if (auto *paintItem = dynamic_cast<PaintItem *>(item)) {
            m_clipboardItems.append(paintItem->clone());
        } else if (auto *textItem = dynamic_cast<PaintTextItem *>(item)) {
            m_clipboardTextItems.append(textItem->clone());
        }
    }
}

void PaintScene::pasteCopied()
{
    for (PaintItem *source : m_clipboardItems) {
        PaintItem *copy = source->clone();
        copy->moveBy(12, 12);
        addItem(copy);
        applyItemFlags(copy);
        m_undoStack->push(new AddPaintItemCommand(this, copy, QStringLiteral("粘贴标注")));
    }
    for (PaintTextItem *source : m_clipboardTextItems) {
        PaintTextItem *copy = source->clone();
        copy->moveBy(12, 12);
        addItem(copy);
        applyItemFlags(copy);
        m_undoStack->push(new AddPaintItemCommand(this, copy, QStringLiteral("粘贴文字")));
    }
}

void PaintScene::moveSelectedBy(const QPointF &delta)
{
    QVector<QPair<QGraphicsItem *, QPointF>> starts;
    QVector<QPair<QGraphicsItem *, QPointF>> ends;
    for (QGraphicsItem *item : selectedItems()) {
        starts.append({item, item->pos()});
        item->moveBy(delta.x(), delta.y());
        ends.append({item, item->pos()});
    }
    if (!starts.isEmpty()) {
        m_undoStack->push(new MoveItemsCommand(starts, ends));
    }
    update();
    emit sigRepaintRequested();
}

qreal PaintScene::adjustPenWidth(int direction)
{
    const auto widths = penSizeSteps();
    int        nearest = 0;
    qreal      distance = qAbs(m_penWidth - widths[0]);
    for (int index = 1; index < static_cast<int>(widths.size()); ++index) {
        const qreal candidate = qAbs(m_penWidth - widths[index]);
        if (candidate < distance) {
            nearest = index;
            distance = candidate;
        }
    }
    nearest = qBound(0, nearest + (direction >= 0 ? 1 : -1), static_cast<int>(widths.size()) - 1);
    m_penWidth = widths[nearest];
    m_toolPenWidths[static_cast<std::size_t>(m_tool)] = m_penWidth;
    if (m_activeItem != nullptr) {
        m_activeItem->setPenWidth(m_penWidth);
    }
    emit sigRepaintRequested();
    emit sigPenWidthChanged(m_penWidth);
    scheduleEffectTileUpdate();
    return m_penWidth;
}

void PaintScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    m_moveStarts.clear();
    if (event->button() == Qt::RightButton) {
        stopItemEditing();
        event->accept();
        return;
    }
    PaintItemHandle handle = PaintItemHandle::kNone;
    if (event->button() == Qt::LeftButton) {
        handle = itemHandleAt(event->scenePos());
        if (handle != PaintItemHandle::kNone && handle != PaintItemHandle::kMove) {
            m_resizeHandle = handle;
            m_resizeItem = m_editableItem;
            m_resizeStartPosition = event->scenePos();
            m_resizeStartTransform = m_resizeItem->transform();
            m_resizeStartItemPosition = m_resizeItem->pos();
            m_resizeBaseWidth
                = qMax<qreal>(1.0, m_resizeItem->mapRectToScene(m_resizeItem->boundingRect()).width());
            m_resizeBaseHeight
                = qMax<qreal>(1.0, m_resizeItem->mapRectToScene(m_resizeItem->boundingRect()).height());
            const QRectF local_bounds = m_resizeItem->boundingRect();
            switch (handle) {
            case PaintItemHandle::kTopLeft:
                m_resizeAnchorLocal = local_bounds.bottomRight();
                break;
            case PaintItemHandle::kTop:
                m_resizeAnchorLocal = QPointF(local_bounds.center().x(), local_bounds.bottom());
                break;
            case PaintItemHandle::kTopRight:
                m_resizeAnchorLocal = local_bounds.bottomLeft();
                break;
            case PaintItemHandle::kRight:
                m_resizeAnchorLocal = QPointF(local_bounds.left(), local_bounds.center().y());
                break;
            case PaintItemHandle::kBottomRight:
                m_resizeAnchorLocal = local_bounds.topLeft();
                break;
            case PaintItemHandle::kBottom:
                m_resizeAnchorLocal = QPointF(local_bounds.center().x(), local_bounds.top());
                break;
            case PaintItemHandle::kBottomLeft:
                m_resizeAnchorLocal = local_bounds.topRight();
                break;
            case PaintItemHandle::kLeft:
                m_resizeAnchorLocal = QPointF(local_bounds.right(), local_bounds.center().y());
                break;
            case PaintItemHandle::kNone:
            case PaintItemHandle::kMove:
                break;
            }
            m_resizeAnchorScene
                = m_resizeStartItemPosition + m_resizeStartTransform.map(m_resizeAnchorLocal);
            event->accept();
            return;
        }
    }
    if (event->button() != Qt::LeftButton) {
        QGraphicsScene::mousePressEvent(event);
        return;
    }

    if (handle == PaintItemHandle::kMove) {
        m_itemMoveStartPosition = event->scenePos();
        for (QGraphicsItem *item : selectedItems()) {
            m_moveStarts.append({item, item->pos()});
        }
        event->accept();
        return;
    }

    if (m_tool == PaintTool::kNone || m_tool == PaintTool::kSelect) {
        stopItemEditing();
        event->accept();
        return;
    }

    if (m_tool == PaintTool::kText) {
        createTextAt(event->scenePos());
        event->accept();
        return;
    }

    PaintItem *item = createItemAt(event->scenePos());
    if (item != nullptr) {
        event->accept();
    } else {
        QGraphicsScene::mousePressEvent(event);
    }
}

void PaintScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_resizeItem != nullptr) {
        const QPointF delta = event->scenePos() - m_resizeStartPosition;
        qreal         scale_x = m_resizeStartTransform.m11();
        qreal         scale_y = m_resizeStartTransform.m22();
        const qreal   delta_x = delta.x() / m_resizeBaseWidth;
        const qreal   delta_y = delta.y() / m_resizeBaseHeight;

        switch (m_resizeHandle) {
        case PaintItemHandle::kLeft:
            scale_x -= delta_x;
            break;
        case PaintItemHandle::kRight:
            scale_x += delta_x;
            break;
        case PaintItemHandle::kTop:
            scale_y -= delta_y;
            break;
        case PaintItemHandle::kBottom:
            scale_y += delta_y;
            break;
        case PaintItemHandle::kTopLeft:
            scale_x -= delta_x;
            scale_y -= delta_y;
            break;
        case PaintItemHandle::kTopRight:
            scale_x += delta_x;
            scale_y -= delta_y;
            break;
        case PaintItemHandle::kBottomLeft:
            scale_x -= delta_x;
            scale_y += delta_y;
            break;
        case PaintItemHandle::kBottomRight:
            scale_x += delta_x;
            scale_y += delta_y;
            break;
        case PaintItemHandle::kNone:
        case PaintItemHandle::kMove:
            break;
        }
        const QTransform resized_transform
            = QTransform::fromScale(qMax<qreal>(0.1, scale_x), qMax<qreal>(0.1, scale_y));
        m_resizeItem->setTransform(resized_transform);
        m_resizeItem->setPos(m_resizeAnchorScene - resized_transform.map(m_resizeAnchorLocal));
        update();
        emit sigRepaintRequested();
        event->accept();
        return;
    }
    if (!m_moveStarts.isEmpty() && event->buttons().testFlag(Qt::LeftButton)) {
        const QPointF delta = event->scenePos() - m_itemMoveStartPosition;
        for (const auto &entry : m_moveStarts) {
            entry.first->setPos(entry.second + delta);
        }
        update();
        event->accept();
        return;
    }
    if (m_activeItem != nullptr) {
        updateActiveItem(event->scenePos());
        update();
        event->accept();
        return;
    }
    QGraphicsScene::mouseMoveEvent(event);
}

void PaintScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_resizeItem != nullptr && event->button() == Qt::LeftButton) {
        const QTransform endTransform = m_resizeItem->transform();
        if (endTransform != m_resizeStartTransform) {
            m_undoStack->push(new ScaleItemCommand(m_resizeItem,
                                                  m_resizeStartTransform,
                                                  m_resizeStartItemPosition,
                                                  endTransform,
                                                  m_resizeItem->pos()));
        }
        m_resizeItem = nullptr;
        m_resizeHandle = PaintItemHandle::kNone;
        emit sigSelectionChanged();
        event->accept();
        return;
    }
    if (m_activeItem != nullptr) {
        updateActiveItem(event->scenePos());
        finishActiveItem();
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton && !m_moveStarts.isEmpty()) {
        QVector<QPair<QGraphicsItem *, QPointF>> ends;
        bool                                     moved = false;
        for (const auto &entry : m_moveStarts) {
            ends.append({entry.first, entry.first->pos()});
            moved = moved || entry.second != entry.first->pos();
        }
        if (moved) {
            m_undoStack->push(new MoveItemsCommand(m_moveStarts, ends));
        }
        m_moveStarts.clear();
        emit sigSelectionChanged();
        event->accept();
        return;
    }
    QGraphicsScene::mouseReleaseEvent(event);
}

void PaintScene::keyPressEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) && !selectedItems().isEmpty()) {
        deleteSelected();
        event->accept();
        return;
    }
    if (!selectedItems().isEmpty()
        && (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right || event->key() == Qt::Key_Up
            || event->key() == Qt::Key_Down)) {
        QPointF delta;
        if (event->key() == Qt::Key_Left)
            delta.setX(-1);
        if (event->key() == Qt::Key_Right)
            delta.setX(1);
        if (event->key() == Qt::Key_Up)
            delta.setY(-1);
        if (event->key() == Qt::Key_Down)
            delta.setY(1);
        moveSelectedBy(delta);
        event->accept();
        return;
    }
    QGraphicsScene::keyPressEvent(event);
}

void PaintScene::applyItemFlags(QGraphicsItem *item)
{
    setItemEditingEnabled(item, false);
    if (auto *text_item = dynamic_cast<PaintTextItem *>(item)) {
        text_item->setTextInteractionFlags(Qt::NoTextInteraction);
    }
}

PaintItemHandle PaintScene::itemHandleAt(const QPointF &position) const
{
    const QGraphicsItem *item = m_editableItem;
    if (item == nullptr || !item->isSelected()) {
        return PaintItemHandle::kNone;
    }

    constexpr qreal HANDLE_HIT_RADIUS = 9.0;
    const QRectF    bounds = item->mapRectToScene(item->boundingRect());
    const QPointF   corners[] = {
        bounds.topLeft(),
        bounds.topRight(),
        bounds.bottomLeft(),
        bounds.bottomRight(),
    };
    const PaintItemHandle corner_handles[] = {
        PaintItemHandle::kTopLeft,
        PaintItemHandle::kTopRight,
        PaintItemHandle::kBottomLeft,
        PaintItemHandle::kBottomRight,
    };
    for (std::size_t index = 0; index < std::size(corners); ++index) {
        if (QLineF(position, corners[index]).length() <= HANDLE_HIT_RADIUS) {
            return corner_handles[index];
        }
    }

    const QPointF edge_centers[] = {
        QPointF(bounds.center().x(), bounds.top()),
        QPointF(bounds.right(), bounds.center().y()),
        QPointF(bounds.center().x(), bounds.bottom()),
        QPointF(bounds.left(), bounds.center().y()),
    };
    const PaintItemHandle edge_handles[] = {
        PaintItemHandle::kTop,
        PaintItemHandle::kRight,
        PaintItemHandle::kBottom,
        PaintItemHandle::kLeft,
    };
    for (std::size_t index = 0; index < std::size(edge_centers); ++index) {
        if (QLineF(position, edge_centers[index]).length() <= HANDLE_HIT_RADIUS) {
            return edge_handles[index];
        }
    }

    const QPointF local_position = item->mapFromScene(position);
    if (const auto *paint_item = dynamic_cast<const PaintItem *>(item); paint_item != nullptr && !paint_item->isFilled()) {
        QPainterPathStroker stroker;
        stroker.setWidth(qMax<qreal>(8.0, paint_item->penWidth() + 4.0));
        if (stroker.createStroke(paint_item->shape()).contains(local_position)) {
            return PaintItemHandle::kMove;
        }
        return PaintItemHandle::kNone;
    }
    return item->shape().contains(local_position) ? PaintItemHandle::kMove
                                                  : PaintItemHandle::kNone;
}

void PaintScene::beginItemEditing(QGraphicsItem *item)
{
    stopItemEditing();
    if (item == nullptr) {
        return;
    }
    m_editableItem = item;
    setItemEditingEnabled(item, true);
    item->setSelected(true);
    if (auto *text_item = dynamic_cast<PaintTextItem *>(item)) {
        text_item->setTextInteractionFlags(Qt::TextEditorInteraction);
        text_item->setFocus();
    }
    emit sigSelectionChanged();
}

void PaintScene::stopItemEditing()
{
    m_resizeItem = nullptr;
    m_resizeHandle = PaintItemHandle::kNone;
    m_moveStarts.clear();
    clearSelection();
    for (QGraphicsItem *item : items()) {
        setItemEditingEnabled(item, false);
        if (auto *text_item = dynamic_cast<PaintTextItem *>(item)) {
            text_item->setTextInteractionFlags(Qt::NoTextInteraction);
            text_item->clearFocus();
        }
    }
    m_editableItem = nullptr;
    emit sigSelectionChanged();
}

bool PaintScene::hasEditableItem() const
{
    return m_editableItem != nullptr;
}

QGraphicsItem *PaintScene::editableItem() const
{
    return m_editableItem;
}

void PaintScene::setItemEditingEnabled(QGraphicsItem *item, bool enabled)
{
    item->setFlag(QGraphicsItem::ItemIsSelectable, enabled);
    item->setFlag(QGraphicsItem::ItemIsMovable, enabled);
}

void PaintScene::applySelectedProperty(int property, const QVariant &value)
{
    const PaintProperty     paint_property = static_cast<PaintProperty>(property);
    QVector<PropertyChange> changes;
    for (QGraphicsItem *item : selectedItems()) {
        const QVariant before = itemPropertyValue(item, paint_property);
        if (!before.isValid() || before == value) {
            continue;
        }
        changes.append({item, before, value});
        applyItemProperty(item, paint_property, value);
    }
    if (!changes.isEmpty()) {
        m_undoStack->push(
            new ChangeItemPropertyCommand(paint_property, std::move(changes), QStringLiteral("修改标注属性")));
    }
    emit sigRepaintRequested();
}

void PaintScene::scheduleEffectTileUpdate()
{
    if (!m_effectTileTimer.isActive()) {
        m_effectTileTimer.start(0);
    }
}

void PaintScene::processEffectTileUpdates()
{
    bool pending = false;
    for (QGraphicsItem *item : items()) {
        auto *paint_item = dynamic_cast<PaintItem *>(item);
        if (paint_item == nullptr || !paint_item->isVisible() || !paint_item->hasPendingEffectTiles()) {
            continue;
        }
        paint_item->processEffectTiles(EFFECT_TILES_PER_TICK);
        pending = pending || paint_item->hasPendingEffectTiles();
    }
    if (pending) {
        m_effectTileTimer.start(0);
    }
    emit sigRepaintRequested();
}
