#pragma once

#include <QDialog>

class QPlainTextEdit;

class OcrDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OcrDialog(const QString &text, QWidget *parent = nullptr);

    QString text() const;

private:
    void copyText();

    QPlainTextEdit *m_editor{nullptr};
};
