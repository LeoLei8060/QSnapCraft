#include "qhotkeylineedit.h"

QHotKeyLineEdit::QHotKeyLineEdit(QWidget *parent)
    : QWidget(parent)
    , m_current_state(Empty)
    , m_setting_mode(false)
{
    initUI();
    updateUI();
}

void QHotKeyLineEdit::setText(const QString &text, bool isValid)
{
    if (text.isEmpty()) {
        setState(Empty);
        m_lineEdit->clear();
    } else {
        m_lineEdit->setText(text);
        setState(isValid ? Valid : Invalid);
    }
    m_lineEdit->clearFocus(); // 让控件失去焦点
    clearFocus();
}

void QHotKeyLineEdit::setShortcut(const QString &shortcut)
{
    m_lineEdit->setText(shortcut);
    setState(shortcut.isEmpty() ? Empty : Valid);
}

void QHotKeyLineEdit::setState(State state)
{
    if (m_current_state == state)
        return;
    qDebug() << "Status changed: " << state;
    m_current_state = state;
    updateUI();
    emit sigStatusChanged(stateToString(m_current_state));
}

void QHotKeyLineEdit::resetText(const QString &text)
{
    emit sigKeyPressed(text);
}

bool QHotKeyLineEdit::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_lineEdit) {
        switch (event->type()) {
        case QEvent::KeyPress: {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            handleKeyEvent(keyEvent);
            return true;
        }
        case QEvent::FocusIn: {
            if (m_lineEdit->text().isEmpty())
                setState(Setting);
            return true;
        }
        case QEvent::FocusOut: {
            if (m_lineEdit->text().isEmpty())
                setState(Empty);
        }
        case QEvent::KeyRelease: {
            return m_current_state == Setting; // 在Setting状态下阻止所有按键释放事件
        }
        case QEvent::InputMethod: {
            // 阻止输入法输入
            return true;
        }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void QHotKeyLineEdit::resizeEvent(QResizeEvent *event)
{
    updateUI();
    QWidget::resizeEvent(event);
}

void QHotKeyLineEdit::initUI()
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_lineEdit = new QLineEdit();
    m_lineEdit->setMinimumWidth(120);
    m_lineEdit->installEventFilter(this);

    // 禁用输入法
    m_lineEdit->setAttribute(Qt::WA_InputMethodEnabled, false);

    // 内部清除按钮
    m_clearButton = new QToolButton(m_lineEdit);
    m_clearButton->setFixedSize(16, 16);       // 设置固定大小
    m_clearButton->setCursor(Qt::ArrowCursor); // 设置鼠标样式为箭头
    m_clearButton->setStyleSheet(
        "QToolButton { border: none; background: transparent; }"
        "QToolButton { border-image: url(:/icons/delete.png); }"); // 使用image而不是border-image
    connect(m_clearButton, &QToolButton::clicked, this, &QHotKeyLineEdit::clearShortcut);

    // 外部状态按钮
    m_statusButton = new QToolButton();
    m_statusButton->setFixedSize(16, 16);
    m_statusButton->setCursor(Qt::ArrowCursor);
    m_statusButton->setStyleSheet("QToolButton { border: none; background: transparent; }");

    layout->addWidget(m_lineEdit);
    layout->addWidget(m_statusButton);
}

void QHotKeyLineEdit::updateUI()
{
    switch (m_current_state) {
    case Empty:
        m_lineEdit->setText("");
        m_lineEdit->setPlaceholderText("");
        m_clearButton->hide();
        m_statusButton->hide();
        break;
    case Setting:
        m_lineEdit->setText("");
        m_lineEdit->setPlaceholderText("按下快捷键");
        m_clearButton->hide();
        m_statusButton->hide();
        break;
    case Invalid:
        m_clearButton->show();
        m_statusButton->show();
        m_statusButton->setStyleSheet("QToolButton { border: none; background: transparent; }"
                                      "QToolButton { border-image: url(:/icons/error.png); }");
        break;
    case Valid:
        m_clearButton->show();
        m_statusButton->show();
        m_statusButton->setStyleSheet("QToolButton { border: none; background: transparent; }"
                                      "QToolButton { border-image: url(:/icons/valid.png); }");
        break;
    }

    // 使用QTimer确保在布局更新后再更新按钮位置
    QTimer::singleShot(0, this, [this]() { updateButtonPosition(); });
}

void QHotKeyLineEdit::updateButtonPosition()
{
    if (!m_clearButton->isVisible())
        return;

    QSize clearSize = m_clearButton->sizeHint();
    QSize statusSize = m_statusButton->sizeHint();

    // 设置清除按钮位置，预留状态按钮的空间
    int clearRight = m_lineEdit->width() - clearSize.width() - 2;
    m_clearButton->move(clearRight, (m_lineEdit->height() - clearSize.height()) / 2 + 1);

    // 调整文本框右边距，为两个按钮预留空间
    m_lineEdit->setStyleSheet(QString("QLineEdit { padding-right: %1px; }")
                                  .arg(clearSize.width() + statusSize.width() + 4));
}

void QHotKeyLineEdit::handleKeyEvent(QKeyEvent *event)
{
    // 只在Setting状态下处理键盘输入
    if (m_current_state == Setting) {
        int                   key = event->key();
        Qt::KeyboardModifiers modifiers = event->modifiers();

        // 过滤无效按键
        if (key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt
            || key == Qt::Key_Meta)
            return;

        // 阻止默认的文本输入行为
        event->accept();

        QStringList keys;

        // 添加修饰键
        if (modifiers & Qt::ControlModifier)
            keys << "Ctrl";
        if (modifiers & Qt::ShiftModifier)
            keys << "Shift";
        if (modifiers & Qt::AltModifier)
            keys << "Alt";
        if (modifiers & Qt::MetaModifier)
            keys << "Meta";

        // 添加主键
        keys << QKeySequence(key).toString();

        // 发送组合键字符串
        emit sigKeyPressed(keys.join("+"));
    } else {
        // 非Setting状态下，阻止所有键盘输入
        event->accept();
    }
}

void QHotKeyLineEdit::clearShortcut()
{
    m_lineEdit->clear();
    setState(Empty);
    m_setting_mode = true;
    m_lineEdit->setFocus();
    setState(Setting);
    emit sigClearKey();
}

QString QHotKeyLineEdit::stateToString(State state) const
{
    switch (state) {
    case Empty:
        return "empty";
    case Setting:
        return "setting";
    case Invalid:
        return "invalid";
    case Valid:
        return "valid";
    }
    return "";
}
