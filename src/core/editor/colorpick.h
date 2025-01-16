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
    void   showAtButton(QToolButton *button);  // 在按钮旁边显示

signals:
    void colorChanged(const QColor &color);  // 新增颜色变化信号

protected:
    void setupUI();
    void initializeConnect();
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;  // 事件过滤器

private:
    void adjustPosition(QToolButton *button);  // 调整位置以避免超出屏幕

    QColor m_selectColor{Qt::black};
    QVector<QToolButton *> m_selectColorBtns;
    QVector<QColor>        m_colors;
};
