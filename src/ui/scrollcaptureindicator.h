#pragma once

#include <QRect>
#include <QWidget>

class ScrollCaptureIndicator : public QWidget
{
    Q_OBJECT

public:
    explicit ScrollCaptureIndicator(QWidget *parent = nullptr);

    void setSelection(const QRect &global_selection);
    void setCapturedHeight(int height);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QRect m_selection;
    int   m_capturedHeight{0};
};
