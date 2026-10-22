#pragma once

#include <array>
#include <QColor>
#include <QGraphicsScene>
#include <QPair>
#include <QPointF>
#include <QString>
#include <QTimer>

class QUndoStack;
class PaintItem;
class PaintTextItem;
class QGraphicsSceneMouseEvent;
class QVariant;

enum class PaintTool {
    kSelect,
    kRectangle,
    kEllipse,
    kLine,
    kArrow,
    kPencil,
    kMarker,
    kMosaic,
    kText,
    kEraser,
    kNumber
};

enum class ArrowStyle { kSolid, kSolidTail, kHollowTail };

enum class PaintEffectMode { kMosaic, kBlur };

class PaintScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit PaintScene(QObject *parent = nullptr);
    ~PaintScene() override;

    void            setTool(PaintTool tool);
    PaintTool       tool() const;
    void            setColor(const QColor &color);
    QColor          color() const;
    void            setPenWidth(qreal width);
    qreal           penWidth() const;
    void            setFilled(bool filled);
    bool            isFilled() const;
    void            setArrowStyle(ArrowStyle style);
    ArrowStyle      arrowStyle() const;
    void            setMarkerOpacity(int opacity);
    int             markerOpacity() const;
    void            setEffectMode(PaintEffectMode mode);
    PaintEffectMode effectMode() const;
    void            setEffectLevel(int level);
    int             effectLevel() const;
    void            setFontFamily(const QString &family);
    QString         fontFamily() const;
    void            setFontSize(int size);
    int             fontSize() const;
    void            setFontBold(bool bold);
    bool            isFontBold() const;
    void            setFontItalic(bool italic);
    bool            isFontItalic() const;
    void            setTextOutlined(bool outlined);
    bool            isTextOutlined() const;
    void            setTextBackgroundEnabled(bool enabled);
    bool            isTextBackgroundEnabled() const;

    QUndoStack    *undoStack() const;
    void           setBackgroundImage(const QImage &image);
    PaintItem     *createItemAt(const QPointF &position);
    PaintTextItem *createTextAt(const QPointF &position);
    void           updateActiveItem(const QPointF &position);
    void           appendActivePoint(const QPointF &position);
    void           finishActiveItem();
    void           clearAll();
    int            visibleItemCount() const;
    void           deleteSelected();
    void           copySelected();
    void           pasteCopied();
    void           moveSelectedBy(const QPointF &delta);
    qreal          adjustPenWidth(int direction);

signals:
    void sigSelectionChanged();
    void sigRepaintRequested();
    void sigPenWidthChanged(qreal width);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void           applyItemFlags(QGraphicsItem *item);
    QGraphicsItem *resizeHandleItemAt(const QPointF &position) const;
    void           applySelectedProperty(int property, const QVariant &value);
    void           scheduleEffectTileUpdate();
    void           processEffectTileUpdates();

    PaintTool                                m_tool{PaintTool::kRectangle};
    QColor                                   m_color{255, 82, 82};
    qreal                                    m_penWidth{2.0};
    bool                                     m_filled{false};
    ArrowStyle                               m_arrowStyle{ArrowStyle::kSolid};
    int                                      m_markerOpacity{110};
    PaintEffectMode                          m_effectMode{PaintEffectMode::kMosaic};
    std::array<int, 2>                       m_effectLevels{8, 8};
    QString                                  m_fontFamily;
    int                                      m_fontSize{24};
    bool                                     m_fontBold{false};
    bool                                     m_fontItalic{false};
    bool                                     m_textOutlined{false};
    bool                                     m_textBackground{false};
    QUndoStack                              *m_undoStack{nullptr};
    PaintItem                               *m_activeItem{nullptr};
    QPointF                                  m_textStart;
    int                                      m_nextNumber{1};
    int                                      m_activeNumber{1};
    QImage                                   m_backgroundImage;
    QVector<class PaintItem *>               m_clipboardItems;
    QVector<PaintTextItem *>                 m_clipboardTextItems;
    QVector<QPair<QGraphicsItem *, QPointF>> m_moveStarts;
    QGraphicsItem                           *m_resizeItem{nullptr};
    QPointF                                  m_resizeStartPosition;
    qreal                                    m_resizeStartScale{1.0};
    qreal                                    m_resizeBaseWidth{1.0};
    std::array<QColor, 11>                   m_toolColors;
    QTimer                                   m_effectTileTimer;
};
