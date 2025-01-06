#include "colorpick.h"
#include <QColor>
#include <QColorDialog>
#include <QGridLayout>
#include <QHBoxLayout>

#define BTN_STYLE \
    "QToolButton { background-color: %1; border: 1px solid #6a6a6a; }\
QToolButton:hover { border: 2px solid #6a6a6a;  }"

ColorPick::ColorPick(QWidget *parent)
{
    setWindowFlags(Qt::Widget);
    setupUI();
    initializeConnect();
}

ColorPick::~ColorPick() {}

void ColorPick::setColor(const QColor &color)
{
    if (color == m_selectColor)
        return;
    m_selectColor = color;
    updateSelectedBtn();
}

void ColorPick::setupUI()
{
    m_colors = QVector<QColor>{"#000000", "#FF0000", "#00FF00", "#0000FF", "#FFFF00",
                               "#00FFFF", "#FF00FF", "#FFA500", "#800080", "#A52A2A",
                               "#FFFFFF", "#808080", "#ADD8E6", "#90EE90", "#FFB6C1",
                               "#FFFFE0", "#E6E6FA", "#8B0000", "#006400", "#FFC0CB"};

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setSpacing(2);
    layout->setContentsMargins(1, 1, 1, 1);

    m_colorBtn = new QToolButton(this);
    m_colorBtn->setFixedSize(26, 26);
    m_colorBtn->setStyleSheet(QString(BTN_STYLE).arg("black"));
    layout->addWidget(m_colorBtn);

    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->setSpacing(2);
    gridLayout->setContentsMargins(1, 1, 1, 1);

    int row = 0, col = 0;
    for (const QColor &color : m_colors) {
        QToolButton *btn = new QToolButton(this);
        btn->setFixedSize(12, 12);
        btn->setStyleSheet(QString(BTN_STYLE).arg(color.name()));

        gridLayout->addWidget(btn, row, col, 1, 1);

        m_selectColorBtns.push_back(btn);

        if (col == 9) {
            ++row;
            col = 0;
        } else
            ++col;
    }

    layout->addLayout(gridLayout);
}

void ColorPick::initializeConnect()
{
    for (int i = 0; i < m_selectColorBtns.count(); ++i) {
        connect(m_selectColorBtns[i], &QToolButton::clicked, this, [this, i]() {
            setColor(m_colors[i]);
        });
    }

    connect(m_colorBtn, &QToolButton::clicked, this, &ColorPick::onShowColorDialog);
}

void ColorPick::onShowColorDialog()
{
    setColor(QColorDialog::getColor(m_selectColor));
}

void ColorPick::updateSelectedBtn()
{
    m_colorBtn->setStyleSheet(QString(BTN_STYLE).arg(m_selectColor.name()));
}
