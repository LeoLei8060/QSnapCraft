#include "settingswindow.h"

#include "aboutsettings.h"
#include "config/globalconfig.h"
#include "controlsettings.h"
#include "generalsettings.h"
#include "interfacesettings.h"
#include "ocrsettings.h"
#include "outputsettings.h"
#include "pinsettings.h"
#include "screenshotsettings.h"
#include "ui/glyphicon.h"

#include <QCloseEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QShowEvent>
#include <QSize>
#include <QStackedWidget>
#include <QVBoxLayout>

SettingsWindow::SettingsWindow(QWidget *parent)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("SettingsWindow"));
    setWindowTitle(tr("QSnapCraft 首选项"));
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowModality(Qt::NonModal);
    resize(920, 650);
    setMinimumSize(760, 520);

    setupUi();
    loadPreferences();
}

SettingsWindow::~SettingsWindow() = default;

void SettingsWindow::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *content = new QWidget(this);
    auto *contentLayout = new QHBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    m_pageList = new QListWidget(content);
    m_pageList->setObjectName(QStringLiteral("list_SettingsPages"));
    m_pageList->setFixedWidth(176);
    m_pageList->setFrameShape(QFrame::NoFrame);
    m_pageList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    contentLayout->addWidget(m_pageList);

    m_pageStack = new QStackedWidget(content);
    m_pageStack->setObjectName(QStringLiteral("stack_SettingsPages"));
    contentLayout->addWidget(m_pageStack, 1);
    rootLayout->addWidget(content, 1);

    m_generalSettings = new GeneralSettings(this);
    m_generalSettings->setObjectName(QStringLiteral("page_General"));
    m_screenshotSettings = new ScreenshotSettings(this);
    m_screenshotSettings->setObjectName(QStringLiteral("page_Screenshot"));
    m_outputSettings = new OutputSettings(this);
    m_outputSettings->setObjectName(QStringLiteral("page_Output"));
    m_interfaceSettings = new InterfaceSettings(this);
    m_interfaceSettings->setObjectName(QStringLiteral("page_CaptureBar"));
    m_pinSettings = new PinSettings(this);
    m_pinSettings->setObjectName(QStringLiteral("page_Pin"));
    m_ocrSettings = new OcrSettings(this);
    m_ocrSettings->setObjectName(QStringLiteral("page_OCR"));
    m_controlSettings = new ControlSettings(this);
    m_controlSettings->setObjectName(QStringLiteral("page_Hotkeys"));
    m_aboutSettings = new AboutSettings(this);
    m_aboutSettings->setObjectName(QStringLiteral("page_About"));

    const struct
    {
        QWidget  *page;
        QString   title;
        GlyphIcon icon;
    } pages[] = {{m_generalSettings, tr("常规"), GlyphIcon::kPreferences},
                 {m_screenshotSettings, tr("截图"), GlyphIcon::kCaptureRegion},
                 {m_outputSettings, tr("输出"), GlyphIcon::kSave},
                 {m_interfaceSettings, tr("桌面工具条"), GlyphIcon::kOptions},
                 {m_pinSettings, tr("贴图"), GlyphIcon::kPin},
                 {m_ocrSettings, tr("OCR"), GlyphIcon::kOcr},
                 {m_controlSettings, tr("快捷键"), GlyphIcon::kActiveWindow},
                 {m_aboutSettings, tr("关于"), GlyphIcon::kAbout}};

    for (const auto &page : pages) {
        auto *scrollArea = new QScrollArea(m_pageStack);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setWidgetResizable(true);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setWidget(page.page);
        m_pageStack->addWidget(scrollArea);
        auto *item = new QListWidgetItem(createGlyphIcon(page.icon), page.title, m_pageList);
        item->setSizeHint(QSize(0, 42));
    }

    auto *footer = new QWidget(this);
    footer->setObjectName(QStringLiteral("settingsFooter"));
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(16, 12, 16, 12);
    footerLayout->setSpacing(8);
    m_restorePageButton = new QPushButton(tr("恢复本页默认值"), footer);
    m_restorePageButton->setObjectName(QStringLiteral("btn_RestorePageDefaults"));
    m_applyButton = new QPushButton(tr("应用"), footer);
    m_applyButton->setObjectName(QStringLiteral("btn_Apply"));
    m_cancelButton = new QPushButton(tr("取消"), footer);
    m_cancelButton->setObjectName(QStringLiteral("btn_Cancel"));
    m_okButton = new QPushButton(tr("确定"), footer);
    m_okButton->setObjectName(QStringLiteral("btn_Ok"));
    m_okButton->setDefault(true);
    footerLayout->addWidget(m_restorePageButton);
    footerLayout->addStretch();
    footerLayout->addWidget(m_applyButton);
    footerLayout->addWidget(m_cancelButton);
    footerLayout->addWidget(m_okButton);
    rootLayout->addWidget(footer);

    setStyleSheet(QStringLiteral(
        "QDialog#SettingsWindow { background: #272727; color: #E8E8E8; }"
        "QWidget { color: #E8E8E8; }"
        "QListWidget#list_SettingsPages { background: #202020; border: none; padding: 10px 8px; outline: none; }"
        "QListWidget#list_SettingsPages::item { padding: 9px 10px; border-radius: 5px; }"
        "QListWidget#list_SettingsPages::item:hover { background: #343434; }"
        "QListWidget#list_SettingsPages::item:selected { background: #0A84FF; color: white; }"
        "QStackedWidget#stack_SettingsPages { background: #272727; }"
        "QScrollArea, QScrollArea > QWidget > QWidget { background: #272727; border: none; }"
        "QWidget#settingsFooter { background: #202020; border-top: 1px solid #3B3B3B; }"
        "QGroupBox { border: 1px solid #444; border-radius: 6px; margin-top: 12px; padding-top: 10px;"
        " font-weight: 600; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 5px; }"
        "QLineEdit, QSpinBox, QComboBox { background: #343434; border: 1px solid #525252; border-radius: 4px;"
        " min-height: 26px; padding: 0 6px; selection-background-color: #0A84FF; }"
        "QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border-color: #0A84FF; }"
        "QPushButton { background: #3A3A3A; border: 1px solid #555; border-radius: 4px; min-height: 28px;"
        " padding: 0 12px; }"
        "QPushButton:hover { background: #474747; }"
        "QPushButton:pressed { background: #303030; }"
        "QPushButton#btn_Apply, QPushButton#btn_Ok { background: #0A84FF; border-color: #0A84FF; color: white; }"
        "QPushButton#btn_Apply:hover, QPushButton#btn_Ok:hover { background: #2E98FF; }"
        "QPushButton#btn_Apply:disabled, QPushButton#btn_Ok:disabled { background: #303030;"
        " border-color: #444; color: #777; }"
        "QCheckBox { spacing: 8px; }"));

    connect(m_pageList, &QListWidget::currentRowChanged, m_pageStack, &QStackedWidget::setCurrentIndex);
    connect(m_pageList, &QListWidget::currentRowChanged, this, [this](int row) {
        m_restorePageButton->setEnabled(row >= 0 && row < m_pageList->count() - 1);
    });
    connect(m_restorePageButton, &QPushButton::clicked, this, &SettingsWindow::onRestorePageClicked);
    connect(m_applyButton, &QPushButton::clicked, this, &SettingsWindow::onApplyClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &SettingsWindow::onCancelClicked);
    connect(m_okButton, &QPushButton::clicked, this, &SettingsWindow::onOkClicked);
    connect(m_generalSettings, &GeneralSettings::sigChanged, this, &SettingsWindow::onPageChanged);
    connect(m_screenshotSettings, &ScreenshotSettings::sigChanged, this, &SettingsWindow::onPageChanged);
    connect(m_outputSettings, &OutputSettings::sigChanged, this, &SettingsWindow::onPageChanged);
    connect(m_interfaceSettings, &InterfaceSettings::sigChanged, this, &SettingsWindow::onPageChanged);
    connect(m_pinSettings, &PinSettings::sigChanged, this, &SettingsWindow::onPageChanged);
    connect(m_ocrSettings, &OcrSettings::sigChanged, this, &SettingsWindow::onPageChanged);
    connect(m_controlSettings, &ControlSettings::sigChanged, this, &SettingsWindow::onPageChanged);

    m_pageList->setCurrentRow(0);
}

void SettingsWindow::showPage(int index)
{
    if (index < 0 || index >= m_pageList->count()) {
        return;
    }
    m_pageList->setCurrentRow(index);
    show();
    raise();
    activateWindow();
}

void SettingsWindow::showAboutPage()
{
    showPage(m_pageList->count() - 1);
}

void SettingsWindow::showEvent(QShowEvent *event)
{
    if (m_dirtyPages.isEmpty()) {
        loadPreferences();
    }
    QDialog::showEvent(event);
}

void SettingsWindow::closeEvent(QCloseEvent *event)
{
    discardChanges();
    hide();
    event->ignore();
}

void SettingsWindow::reject()
{
    discardChanges();
    QDialog::reject();
}

void SettingsWindow::onPageChanged()
{
    const int index = pageIndex(qobject_cast<QWidget *>(sender()));
    if (index >= 0) {
        m_dirtyPages.insert(index);
    }
    updateDirtyState();
}

void SettingsWindow::onApplyClicked()
{
    applyChanges();
}

void SettingsWindow::onCancelClicked()
{
    discardChanges();
    hide();
}

void SettingsWindow::onOkClicked()
{
    if (applyChanges()) {
        hide();
    }
}

void SettingsWindow::onRestorePageClicked()
{
    const int index = m_pageList->currentRow();
    switch (index) {
    case 0:
        m_generalSettings->restoreDefaults();
        break;
    case 1:
        m_screenshotSettings->restoreDefaults();
        break;
    case 2:
        m_outputSettings->restoreDefaults();
        break;
    case 3:
        m_interfaceSettings->restoreDefaults();
        break;
    case 4:
        m_pinSettings->restoreDefaults();
        break;
    case 5:
        m_ocrSettings->restoreDefaults();
        break;
    case 6:
        m_controlSettings->restoreDefaults();
        break;
    case 7:
        m_aboutSettings->restoreDefaults();
        break;
    default:
        break;
    }
    m_dirtyPages.insert(index);
    updateDirtyState();
}

void SettingsWindow::loadPreferences()
{
    m_initialPreferences = GlobalConfig::instance()->getPreferences();
    resetAllPages(m_initialPreferences);
    m_dirtyPages.clear();
    updateDirtyState();
}

void SettingsWindow::discardChanges()
{
    loadPreferences();
}

bool SettingsWindow::applyChanges()
{
    ApplicationPreferences preferences = GlobalConfig::instance()->getPreferences();
    if (m_dirtyPages.contains(0)) {
        m_generalSettings->writePreferences(preferences);
    }
    if (m_dirtyPages.contains(1)) {
        m_screenshotSettings->writePreferences(preferences);
    }
    if (m_dirtyPages.contains(2)) {
        m_outputSettings->writePreferences(preferences);
    }
    if (m_dirtyPages.contains(3)) {
        m_interfaceSettings->writePreferences(preferences);
    }
    if (m_dirtyPages.contains(4)) {
        m_pinSettings->writePreferences(preferences);
    }
    if (m_dirtyPages.contains(5)) {
        m_ocrSettings->writePreferences(preferences);
    }
    if (m_dirtyPages.contains(6)) {
        m_controlSettings->writePreferences(preferences);
    }

    QString error;
    if (!GlobalConfig::instance()->setPreferences(preferences, &error)) {
        QMessageBox::warning(this, tr("无法保存首选项"), error);
        return false;
    }

    m_initialPreferences = GlobalConfig::instance()->getPreferences();
    resetAllPages(m_initialPreferences);
    m_dirtyPages.clear();
    updateDirtyState();
    emit sigPreferencesApplied();
    return true;
}

void SettingsWindow::updateDirtyState()
{
    const bool dirty = !m_dirtyPages.isEmpty();
    m_applyButton->setEnabled(dirty);
    m_okButton->setEnabled(dirty);
}

int SettingsWindow::pageIndex(const QWidget *page) const
{
    if (page == m_generalSettings) {
        return 0;
    }
    if (page == m_screenshotSettings) {
        return 1;
    }
    if (page == m_outputSettings) {
        return 2;
    }
    if (page == m_interfaceSettings) {
        return 3;
    }
    if (page == m_controlSettings) {
        return 6;
    }
    if (page == m_pinSettings) {
        return 4;
    }
    if (page == m_ocrSettings) {
        return 5;
    }
    if (page == m_aboutSettings) {
        return 7;
    }
    return -1;
}

void SettingsWindow::resetAllPages(const ApplicationPreferences &preferences)
{
    const QString config_path = GlobalConfig::instance()->configPath();
    m_generalSettings->setPreferences(preferences);
    m_generalSettings->setConfigPath(config_path);
    m_screenshotSettings->setPreferences(preferences);
    m_outputSettings->setPreferences(preferences);
    m_interfaceSettings->setPreferences(preferences);
    m_pinSettings->setPreferences(preferences);
    m_ocrSettings->setPreferences(preferences);
    m_controlSettings->setPreferences(preferences);
    m_aboutSettings->setPreferences(preferences);
    m_aboutSettings->setConfigPath(config_path);
}
