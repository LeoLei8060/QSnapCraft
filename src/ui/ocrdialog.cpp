#include "ocrdialog.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

OcrDialog::OcrDialog(const QString &text, QWidget *parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("OcrDialog"));
    setWindowTitle(QStringLiteral("OCR 识别结果"));
    resize(560, 420);

    auto *layout = new QVBoxLayout(this);
    m_editor = new QPlainTextEdit(text, this);
    m_editor->setObjectName(QStringLiteral("edit_OcrText"));
    layout->addWidget(m_editor, 1);

    auto *buttons = new QHBoxLayout();
    buttons->addStretch();
    auto *copy_button = new QPushButton(QStringLiteral("复制文本"), this);
    copy_button->setObjectName(QStringLiteral("btn_CopyOcrText"));
    auto *close_button = new QPushButton(QStringLiteral("关闭"), this);
    close_button->setObjectName(QStringLiteral("btn_CloseOcr"));
    buttons->addWidget(copy_button);
    buttons->addWidget(close_button);
    layout->addLayout(buttons);

    connect(copy_button, &QPushButton::clicked, this, &OcrDialog::copyText);
    connect(close_button, &QPushButton::clicked, this, &QDialog::accept);
}

QString OcrDialog::text() const
{
    return m_editor != nullptr ? m_editor->toPlainText() : QString();
}

void OcrDialog::copyText()
{
    QApplication::clipboard()->setText(text());
}
