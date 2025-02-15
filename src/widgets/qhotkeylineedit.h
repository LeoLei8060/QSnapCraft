#ifndef QHOTKEYLINEEDIT_H
#define QHOTKEYLINEEDIT_H

#include <QDebug>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLineEdit>
#include <QTimer>
#include <QToolButton>
#include <QWidget>

class QHotKeyLineEdit : public QWidget
{
    Q_OBJECT
public:
    enum State { Empty, Setting, Invalid, Valid };

    QHotKeyLineEdit(QWidget *parent = nullptr);

    void setText(const QString &text, bool isValid = true);

    QString text() const { return m_lineEdit->text(); }

    void setShortcut(const QString &shortcut);

    QString shortcut() const { return m_lineEdit->text(); }

    void setState(State state);

signals:
    void sigStatusChanged(const QString &status);
    void sigKeyPressed(const QString &key, Qt::KeyboardModifiers modifiers);
    void sigClearKey();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

    void resizeEvent(QResizeEvent *event) override;

private:
    void initUI();

    void updateUI();

    void updateButtonPosition();

    void handleKeyEvent(QKeyEvent *event);

    void clearShortcut();

    QString stateToString(State state) const;

    QLineEdit   *m_lineEdit;
    QToolButton *m_clearButton;
    QToolButton *m_statusButton;
    State        m_current_state;
    bool         m_setting_mode;
};

#endif // QHOTKEYLINEEDIT_H
