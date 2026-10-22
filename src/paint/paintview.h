#pragma once

#include <QGraphicsView>
#include <QImage>

class PaintScene;

class PaintView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit PaintView(QWidget *parent = nullptr);
    ~PaintView() override;

    void          setBackgroundImage(const QImage &image);
    const QImage &backgroundImage() const;
    PaintScene   *paintScene() const;

    QImage renderSelection(const QRect &selection) const;

protected:
    void         paintEvent(QPaintEvent *event) override;
    virtual void drawOverlay(QPainter &painter);

private:
    QImage      m_background;
    PaintScene *m_scene{nullptr};
};
