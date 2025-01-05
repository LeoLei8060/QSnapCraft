#pragma once

#include <QToolButton>
#include <QVector>
#include <QWidget>

class ColorPick : public QWidget
{
    Q_OBJECT
public:
    explicit ColorPick(QWidget *parent = nullptr);
    ~ColorPick();

protected:
    void setupUI();

private:
    QToolButton           *m_colorBtn;
    QVector<QToolButton *> m_selectColorBtns;
};
