#pragma once

#include <QWidget>

class PinWindow : public QWidget {
    Q_OBJECT

public:
    explicit PinWindow(QWidget* parent = nullptr);
    ~PinWindow() override = default;

    void setImage(const QPixmap& pixmap);

private:
    QPixmap currentImage;
};
