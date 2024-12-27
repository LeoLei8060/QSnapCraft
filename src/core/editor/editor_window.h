#pragma once

#include <QWidget>

class EditorWindow : public QWidget {
    Q_OBJECT

public:
    explicit EditorWindow(QWidget* parent = nullptr);
    ~EditorWindow() override = default;

    void setImage(const QPixmap& pixmap);

private:
    QPixmap currentImage;
};
