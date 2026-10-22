#include "paintview.h"

#include "paintscene.h"

#include <QPainter>

PaintView::PaintView(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new PaintScene(this))
{
    setScene(m_scene);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setRenderHint(QPainter::Antialiasing, true);
    connect(m_scene, &PaintScene::sigRepaintRequested, this, [this]() { viewport()->update(); });
    connect(m_scene, &QGraphicsScene::selectionChanged, this, [this]() { viewport()->update(); });
}

PaintView::~PaintView()
{
    disconnect(m_scene, nullptr, this, nullptr);
}

void PaintView::setBackgroundImage(const QImage &image)
{
    m_background = image;
    m_scene->setBackgroundImage(image);
    setSceneRect(QRectF(m_background.rect()));
    viewport()->update();
}

const QImage &PaintView::backgroundImage() const
{
    return m_background;
}

PaintScene *PaintView::paintScene() const
{
    return m_scene;
}

QImage PaintView::renderSelection(const QRect &selection) const
{
    if (m_background.isNull() || !selection.isValid()) {
        return {};
    }

    const QRect clipped = selection.intersected(m_background.rect());
    if (!clipped.isValid()) {
        return {};
    }

    QImage output(clipped.size(), QImage::Format_ARGB32_Premultiplied);
    output.fill(Qt::transparent);
    QPainter painter(&output);
    painter.drawImage(QPoint(0, 0), m_background, clipped);
    m_scene->render(&painter, QRectF(QPointF(0, 0), QSizeF(clipped.size())), QRectF(clipped));
    return output;
}

void PaintView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(viewport());
    if (!m_background.isNull()) {
        painter.drawImage(QPoint(0, 0), m_background);
    }
    m_scene->render(&painter, QRectF(viewport()->rect()), QRectF(viewport()->rect()));
    drawOverlay(painter);
}

void PaintView::drawOverlay(QPainter &painter)
{
    Q_UNUSED(painter)
}
