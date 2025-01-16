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
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // 设置固定大小
    setFixedHeight(34);

    // 初始化按钮
    initializeUI();
}

QString Toolbar::getIconChar(Tool tool) const
{
    // 返回每个工具对应的字体图标码
    switch (tool) {
    case Tool::MoveBtn:
        return QString(QChar(0xe601));
    case Tool::RectangleBtn:
        return QString(QChar(0xe622)); // 矩形
    case Tool::EllipseBtn:
        return QString(QChar(0xe610)); // 椭圆
    case Tool::PolyLineBtn:
        return QString(QChar(0xe621)); // 直线
    case Tool::ArrowBtn:
        return QString(QChar(0xe600)); // 箭头
    case Tool::PencilBtn:
        return QString(QChar(0xe632)); // 画笔
    case Tool::MarkerBtn:
        return QString(QChar(0xe81b)); // 马克笔
    case Tool::MosaicBtn:
        return QString(QChar(0xe613)); // 马赛克
    case Tool::TextBtn:
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
    case Tool::ColorBtn:
        return QString(QChar(0xe6cf)); // 颜色图标
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
    const QVector<std::tuple<Tool, QString>> tools = {{Tool::MoveBtn, tr("Move")},
                                                      {Tool::ColorBtn,
                                                       tr("Color")}, // 移到第二个位置
                                                      {Tool::RectangleBtn, tr("Rectangle")},
                                                      {Tool::EllipseBtn, tr("Ellipse")},
                                                      {Tool::PolyLineBtn, tr("Line")},
                                                      {Tool::ArrowBtn, tr("Arrow")},
                                                      {Tool::PencilBtn, tr("Pencil")},
                                                      {Tool::MarkerBtn, tr("Marker")},
                                                      {Tool::MosaicBtn, tr("Mosaic")},
                                                      {Tool::TextBtn, tr("Text")},
                                                      {Tool::Eraser, tr("Eraser")},
                                                      {Tool::Undo, tr("Undo")},
                                                      {Tool::Redo, tr("Redo")},
                                                      {Tool::Pin, tr("Pin to Screen")},
                                                      {Tool::Exit, tr("Exit")},
                                                      {Tool::Save, tr("Save")},
                                                      {Tool::Copy, tr("Copy")}};

    // 添加分隔符的位置
    const QVector<int> separatorPositions = {1,
                                             10,
                                             12}; // 在颜色按钮后、橡皮擦后和撤销重做后添加分隔符

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

        // 如果是颜色按钮，保存引用
        if (tool == Tool::ColorBtn) {
            m_colorButton = button;
        }
    }
}

bool Toolbar::isCheckableTool(Tool tool) const
{
    // 返回工具是否需要显示选中状态
    switch (tool) {
    case Tool::MoveBtn:
    case Tool::RectangleBtn:
    case Tool::EllipseBtn:
    case Tool::PolyLineBtn:
    case Tool::ArrowBtn:
    case Tool::PencilBtn:
    case Tool::MarkerBtn:
    case Tool::MosaicBtn:
    case Tool::TextBtn:
    case Tool::Eraser:
        return true;
    default:
        return false;
    }
}

void Toolbar::setCurrentTool(Tool tool)
{
    if (m_currentTool != tool) {
        m_currentTool = tool;
        // 更新所有按钮的选中状态
        for (QToolButton *btn : m_buttons) {
            if (btn && btn->property("tool").isValid()) {
                Tool btnTool = static_cast<Tool>(btn->property("tool").toInt());
                if (isCheckableTool(btnTool)) {
                    btn->setChecked(btnTool == tool);
                }
            }
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

    // 存储工具类型
    button->setProperty("tool", static_cast<int>(tool));

    // 设置是否可选中
    if (isCheckableTool(tool)) {
        button->setCheckable(true);
        button->setChecked(tool == m_currentTool);
    }

    // 如果是颜色按钮，设置特殊样式
    if (tool == Tool::ColorBtn) {
        button->setObjectName("colorBtn");
        button->setText(""); // 不显示图标
        button->setStyleSheet(QString(R"(
            QToolButton {
                background-color: black;
                border: 1px solid transparent;
                border-radius: 4px;
                margin: 8px;
            }
            QToolButton:hover {
                border: 1px solid #6a6a6a;
            }
            QToolButton:pressed {
                border: 1px solid #6a6a6a;
                background-color: rgba(0, 0, 0, 0.8);
            }
        )"));
    } else {
        // 其他按钮的通用样式
        QString styleSheet = R"(
            QToolButton {
                color: #666666;
                background: transparent;
                border: none;
                border-radius: 4px;
                padding: 4px;
            }
            QToolButton:hover {
                color: #333333;
                background: rgba(0, 0, 0, 0.1);
            }
            QToolButton:pressed {
                color: #000000;
                background: rgba(0, 0, 0, 0.15);
            })";

        // 为可选中的按钮添加选中状态的样式
        if (isCheckableTool(tool)) {
            styleSheet += R"(
            QToolButton:checked {
                color: #000000;
                background: rgba(0, 0, 0, 0.15);
                border: 1px solid #666666;
            })";
        }

        button->setStyleSheet(styleSheet);
    }

    // 连接按钮点击信号
    connect(button, &QToolButton::clicked, this, [this, tool]() {
        if (isCheckableTool(tool)) {
            setCurrentTool(tool);
        }
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
