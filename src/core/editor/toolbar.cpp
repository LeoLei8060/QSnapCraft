#include "toolbar.h"
#include "../../utils/fontmanager.h"
#include <QDebug>
#include <QFrame>
#include <QHBoxLayout>
#include <QPainter>
#include <QToolButton>

Toolbar::Toolbar(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);

    // 设置固定大小
    //    setFixedSize(450, 40);
    setFixedHeight(34);

    // 初始化按钮
    initializeUI();
}

QString Toolbar::getIconChar(Tool tool) const
{
    // 返回每个工具对应的字体图标码
    switch (tool) {
    case Tool::Rectangle:
        return QString(QChar(0xe622)); // 矩形
    case Tool::Ellipse:
        return QString(QChar(0xe610)); // 椭圆
    case Tool::Line:
        return QString(QChar(0xe621)); // 直线
    case Tool::Arrow:
        return QString(QChar(0xe600)); // 箭头
    case Tool::Pencil:
        return QString(QChar(0xe632)); // 画笔
    case Tool::Marker:
        return QString(QChar(0xe81b)); // 马克笔
    case Tool::Mosaic:
        return QString(QChar(0xe613)); // 马赛克
    case Tool::Text:
        return QString(QChar(0xe854)); // 文字
    case Tool::Eraser:
        return QString(QChar(0xea15)); // 橡皮擦
    case Tool::Undo:
        return QString(QChar(0xe619)); // 撤销
    case Tool::Redo:
        return QString(QChar(0xe62c)); // 重做
    case Tool::Pin:
        return QString(QChar(0xe863)); // 固定
    case Tool::Exit:
        return QString(QChar(0xe607)); // 退出
    case Tool::Save:
        return QString(QChar(0xe782)); // 保存
    case Tool::Copy:
        return QString(QChar(0xe60b)); // 复制
    default:
        return QString();
    }
}

void Toolbar::initializeUI()
{
    auto *layout = new QHBoxLayout(this);
    layout->setSpacing(1);
    layout->setContentsMargins(2, 1, 1, 2);

    // 创建工具按钮
    const QVector<std::tuple<Tool, QString>> tools = {{Tool::Rectangle, tr("Rectangle")},
                                                      {Tool::Ellipse, tr("Ellipse")},
                                                      {Tool::Line, tr("Line")},
                                                      {Tool::Arrow, tr("Arrow")},
                                                      {Tool::Pencil, tr("Pencil")},
                                                      {Tool::Marker, tr("Marker")},
                                                      {Tool::Mosaic, tr("Mosaic")},
                                                      {Tool::Text, tr("Text")},
                                                      {Tool::Eraser, tr("Eraser")},
                                                      {Tool::Undo, tr("Undo")},
                                                      {Tool::Redo, tr("Redo")},
                                                      {Tool::Pin, tr("Pin to Screen")},
                                                      {Tool::Exit, tr("Exit")},
                                                      {Tool::Save, tr("Save")},
                                                      {Tool::Copy, tr("Copy")}};

    // 添加分隔符的位置
    const QVector<int> separatorPositions = {8, 10, 11, 14};

    for (int i = 0; i < tools.size(); ++i) {
        const auto &[tool, tooltip] = tools[i];
        auto *button = createToolButton(tool, getIconChar(tool), tooltip);
        layout->addWidget(button);
        m_buttons.append(button);

        // 在特定位置添加分隔符
        if (separatorPositions.contains(i)) {
            auto *separator = new QFrame(this);
            separator->setFrameShape(QFrame::VLine);
            separator->setFrameShadow(QFrame::Sunken);
            layout->addWidget(separator);
        }
    }
}

QToolButton *Toolbar::createToolButton(Tool tool, const QString &iconText, const QString &tooltip)
{
    auto *button = new QToolButton(this);

    // 设置字体
    QFont iconFont = FontManager::instance()->fontAt(FontManager::IconFont);
    iconFont.setPixelSize(16); // 设置字体大小
    button->setFont(iconFont);

    // 设置文本和样式
    button->setText(iconText);
    button->setToolTip(tooltip);
    button->setFixedSize(32, 32);
    button->setAutoRaise(true);

    // 设置样式表
    button->setStyleSheet(QString(R"(
        QToolButton {
            color: #666666;
            background: transparent;
            border: none;
            border-radius: 4px;
        }
        QToolButton:hover {
            color: #333333;
            background: rgba(0, 0, 0, 0.1);
        }
        QToolButton:pressed {
            color: #000000;
            background: rgba(0, 0, 0, 0.15);
        }
    )"));

    // 连接按钮点击信号
    connect(button, &QToolButton::clicked, this, [this, tool]() {
        m_currentTool = tool;
        emit toolSelected(tool);
    });

    return button;
}

void Toolbar::paintEvent(QPaintEvent *event)
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
