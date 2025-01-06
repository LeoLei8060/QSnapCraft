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

    QColor getColor() const { return m_selectColor; }
    void   setColor(const QColor &color);

protected:
    void setupUI();

    void initializeConnect();

private slots:
    void onShowColorDialog();

private:
    void updateSelectedBtn();

    QColor m_selectColor{Qt::black};

    QToolButton           *m_colorBtn;
    QVector<QToolButton *> m_selectColorBtns;
    QVector<QColor>        m_colors;
};
