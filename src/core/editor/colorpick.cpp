#include "colorpick.h"
#include <QColor>
#include <QGridLayout>
#include <QPainter>
#include <QApplication>
#include <QScreen>
#include <QEvent>

namespace {
const QString BTN_STYLE = R"(
    QToolButton {
        background-color: %1;
        border: 1px solid #6a6a6a;
        border-radius: 4px;
    }
    QToolButton:hover {
        border: 2px solid #6a6a6a;
    }
    QToolButton:pressed {
        border: 2px solid #6a6a6a;
        background-color: %1;
        opacity: 0.8;
    }
)";
}

ColorPick::ColorPick(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    
    // 安装事件过滤器
    qApp->installEventFilter(this);
    
    setupUI();
    initializeConnect();
}

ColorPick::~ColorPick()
{
    qApp->removeEventFilter(this);
}

void ColorPick::showAtButton(QToolButton *button)
{
    adjustPosition(button);
    show();
    setFocus(); // 确保窗口获得焦点
}

void ColorPick::adjustPosition(QToolButton *button)
{
    QPoint pos = button->mapToGlobal(QPoint(0, 0));
    QSize size = this->sizeHint();
    QRect screen = QApplication::screenAt(pos)->geometry();

    // 尝试在按钮右侧显示
    if (pos.x() + button->width() + size.width() <= screen.right()) {
        pos.setX(pos.x() + button->width());
    }
    // 否则尝试在左侧显示
    else if (pos.x() - size.width() >= screen.left()) {
        pos.setX(pos.x() - size.width());
    }

    // 垂直位置调整
    if (pos.y() + size.height() > screen.bottom()) {
        pos.setY(screen.bottom() - size.height());
    }
    if (pos.y() < screen.top()) {
        pos.setY(screen.top());
    }

    move(pos);
}

void ColorPick::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制背景
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(245, 245, 245, 250));
    painter.drawRoundedRect(rect(), 6, 6);

    // 绘制边框
    painter.setPen(QPen(QColor(200, 200, 200), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 6, 6);

    QWidget::paintEvent(event);
}

bool ColorPick::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QPoint globalPos = QCursor::pos();
        if (!geometry().contains(globalPos)) {
            close();  // 改用 close() 代替 hide()
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void ColorPick::initializeConnect()
{
    for (int i = 0; i < m_selectColorBtns.count(); ++i) {
        connect(m_selectColorBtns[i], &QToolButton::clicked, this, [this, i]() {
            setColor(m_colors[i]);
            close();  // 改用 close() 代替 hide()
        });
    }
}

void ColorPick::setColor(const QColor &color)
{
    if (color == m_selectColor)
        return;
    m_selectColor = color;
    emit colorChanged(color);  // 发送颜色变化信号
}

void ColorPick::setupUI()
{
    m_colors = QVector<QColor>{"#000000", "#FF0000", "#00FF00", "#0000FF", "#FFFF00",
                               "#00FFFF", "#FF00FF", "#FFA500", "#800080", "#A52A2A",
                               "#FFFFFF", "#808080", "#ADD8E6", "#90EE90", "#FFB6C1",
                               "#FFFFE0", "#E6E6FA", "#8B0000", "#006400", "#FFC0CB"};

    QGridLayout *gridLayout = new QGridLayout(this);
    gridLayout->setSpacing(2);
    gridLayout->setContentsMargins(4, 4, 4, 4);

    int row = 0, col = 0;
    for (const QColor &color : m_colors) {
        QToolButton *btn = new QToolButton(this);
        btn->setFixedSize(16, 16);  // 稍微增大按钮尺寸
        btn->setStyleSheet(BTN_STYLE.arg(color.name()));

        gridLayout->addWidget(btn, row, col, 1, 1);
        m_selectColorBtns.push_back(btn);

        if (col == 9) {
            ++row;
            col = 0;
        } else
            ++col;
    }
}
