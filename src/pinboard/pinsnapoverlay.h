#pragma once

#include <QLineF>
#include <QRect>
#include <QVector>
#include <QWidget>

class PinSnapOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit PinSnapOverlay(QWidget *parent = nullptr);

    void setGuides(const QVector<QLineF> &guides);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<QLineF> m_guides;
};
