#pragma once

#include <QRect>
#include <QSize>
#include <QWidget>

class ScrollCaptureIndicator : public QWidget
{
    Q_OBJECT

public:
    explicit ScrollCaptureIndicator(QWidget *parent = nullptr);

    void setSelection(const QRect &global_selection);
    void setCapturedSize(const QSize &size);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QRect m_selection;
    QSize m_capturedSize;
};
