#include "ocrsettings.h"

#include "platform/windows/ocrengine.h"

#include <QComboBox>
#include <QDesktopServices>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QUrl>
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
    const QStringList availableLanguages = availableOcrLanguages();
    for (const QString &language : availableLanguages) {
        m_language->addItem(language, language);
    }
    form->addRow(tr("识别语言："), m_language);

    auto *availability = new QLabel(group);
    availability->setObjectName(QStringLiteral("label_OcrAvailability"));
    availability->setWordWrap(true);
    availability->setText(availableLanguages.isEmpty()
                              ? tr("未检测到 Windows OCR 语言包。请先在系统语言设置中安装至少一种 OCR 语言。")
                              : tr("当前可用的 OCR 语言：%1").arg(availableLanguages.join(QStringLiteral("、"))));
    auto *languageSettingsButton = new QPushButton(tr("打开 Windows 语言设置"), group);
    languageSettingsButton->setObjectName(QStringLiteral("btn_OpenOcrLanguageSettings"));
    connect(languageSettingsButton, &QPushButton::clicked, this, [this]() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("ms-settings:regionlanguage")));
    });
    auto *availabilityRow = new QWidget(group);
    auto *availabilityLayout = new QHBoxLayout(availabilityRow);
    availabilityLayout->setContentsMargins(0, 0, 0, 0);
    availabilityLayout->addWidget(availability, 1);
    availabilityLayout->addWidget(languageSettingsButton);
    form->addRow(availabilityRow);

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
