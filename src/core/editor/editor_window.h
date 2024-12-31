#pragma once

#include "utils/magnifier.h"
#include <QWidget>

class EditorWindow : public QWidget
{
    Q_OBJECT

public:
    explicit EditorWindow(QWidget *parent = nullptr);
    ~EditorWindow() override = default;

    void setImage(const QImage &image);
    void setData(const QImage &image, const QRect &captureRect);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage m_currentImage;
    QRect  m_captureRect;

    Magnifier m_magnifier;
};
