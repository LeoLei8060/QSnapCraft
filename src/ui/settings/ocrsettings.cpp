#include "ocrsettings.h"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSignalBlocker>
#include <QVBoxLayout>

OcrSettings::OcrSettings(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(16);

    auto *group = new QGroupBox(tr("OCR"), this);
    auto *form = new QFormLayout(group);
    m_language = new QComboBox(group);
    m_language->setObjectName(QStringLiteral("combo_OcrLanguage"));
    m_language->setEditable(true);
    m_language->addItem(tr("自动（跟随系统语言）"), QString());
    m_language->addItem(QStringLiteral("en-US"), QStringLiteral("en-US"));
    m_language->addItem(QStringLiteral("zh-Hans-CN"), QStringLiteral("zh-Hans-CN"));
    form->addRow(tr("识别语言："), m_language);

    auto *hint = new QLabel(tr("截图进入标注状态后按 Ctrl+O，对当前选区执行 OCR。"), group);
    hint->setWordWrap(true);
    form->addRow(hint);
    layout->addWidget(group);
    layout->addStretch();

    connect(m_language, &QComboBox::currentIndexChanged, this, &OcrSettings::sigChanged);
}

void OcrSettings::setPreferences(const ApplicationPreferences &preferences)
{
    const QSignalBlocker blocker(m_language);
    const int            index = m_language->findData(preferences.ocr.language);
    if (index >= 0) {
        m_language->setCurrentIndex(index);
    } else {
        m_language->setEditText(preferences.ocr.language);
    }
}

void OcrSettings::writePreferences(ApplicationPreferences &preferences) const
{
    if (m_language->currentIndex() == 0) {
        preferences.ocr.language.clear();
    } else {
        preferences.ocr.language = m_language->currentData().toString();
        if (preferences.ocr.language.isEmpty()) {
            preferences.ocr.language = m_language->currentText().trimmed();
        }
    }
}

void OcrSettings::restoreDefaults()
{
    ApplicationPreferences preferences;
    setPreferences(preferences);
}
