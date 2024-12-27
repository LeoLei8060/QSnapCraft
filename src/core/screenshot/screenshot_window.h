#pragma once

#include <QWidget>

class ScreenshotWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenshotWindow(QWidget *parent = nullptr);
    ~ScreenshotWindow() override = default;

public slots:
    void start();
};
