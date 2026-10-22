#pragma once

#include "ui/magnifier.h"

#include <QColor>
#include <QImage>
#include <QPoint>
#include <QWidget>

class ColorPickerWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ColorPickerWindow(QWidget *parent = nullptr);
    ~ColorPickerWindow() override;

    void start();

    static QColor colorAt(const QImage &image, const QPoint &position);

signals:
    void sigColorPicked(const QColor &color);
    void sigCancelled();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void updatePointerPosition(const QPoint &position);
    void finish(bool picked);

    QRect     m_desktopRect;
    QImage    m_image;
    QPixmap   m_background;
    QPoint    m_pointerPosition;
    QColor    m_currentColor;
    Magnifier m_magnifier;
    bool      m_finished{true};
};
