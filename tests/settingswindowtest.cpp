#include "ui/settings/settingswindow.h"
#include "config/globalconfig.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QtTest>

class SettingsWindowTest : public QObject
{
    Q_OBJECT

private slots:
    void cancelDiscardsDraftChanges();
    void applyPersistsChanges();
    void outputChangesUseSnipSettings();
    void showsHotkeyRegistrationStatus();
    void exposesAdditionalSnipOptions();
    void exposesSelectionUnitSetting();
    void exposesCaptureStartModes();
    void appliesPinAndOcrSettings();
    void rendersSettingsPages();
};

void SettingsWindowTest::cancelDiscardsDraftChanges()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    GlobalConfig::instance()->setConfigPath(directory.filePath(QStringLiteral("config.ini")));

    ApplicationPreferences preferences;
    preferences.general.autoStart = true;
    QVERIFY(GlobalConfig::instance()->setPreferences(preferences));

    SettingsWindow window;
    window.show();
    auto *autoStart = window.findChild<QCheckBox *>(QStringLiteral("check_AutoStart"));
    auto *cancel = window.findChild<QPushButton *>(QStringLiteral("btn_Cancel"));
    auto *apply = window.findChild<QPushButton *>(QStringLiteral("btn_Apply"));
    QVERIFY(autoStart);
    QVERIFY(cancel);
    QVERIFY(apply);

    autoStart->setChecked(false);
    QVERIFY(apply->isEnabled());
    QTest::mouseClick(cancel, Qt::LeftButton);

    QVERIFY(autoStart->isChecked());
    QVERIFY(!apply->isEnabled());
    QVERIFY(GlobalConfig::instance()->getPreferences().general.autoStart);
}

void SettingsWindowTest::applyPersistsChanges()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("config.ini"));
    GlobalConfig::instance()->setConfigPath(path);

    ApplicationPreferences preferences;
    preferences.general.autoStart = true;
    QVERIFY(GlobalConfig::instance()->setPreferences(preferences));

    SettingsWindow window;
    window.show();
    auto *autoStart = window.findChild<QCheckBox *>(QStringLiteral("check_AutoStart"));
    auto *apply = window.findChild<QPushButton *>(QStringLiteral("btn_Apply"));
    QVERIFY(autoStart);
    QVERIFY(apply);

    autoStart->setChecked(false);
    QTest::mouseClick(apply, Qt::LeftButton);

    QVERIFY(!GlobalConfig::instance()->getPreferences().general.autoStart);
    QSettings settings(path, QSettings::IniFormat);
    QCOMPARE(settings.value(QStringLiteral("General/AutoStart")).toBool(), false);
    QVERIFY(!apply->isEnabled());
}

void SettingsWindowTest::outputChangesUseSnipSettings()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("config.ini"));
    GlobalConfig::instance()->setConfigPath(path);
    QVERIFY(GlobalConfig::instance()->setPreferences(ApplicationPreferences{}));

    SettingsWindow window;
    window.show();
    auto *quickPath = window.findChild<QLineEdit *>(QStringLiteral("lineEdit_QuickSavePath"));
    auto *apply = window.findChild<QPushButton *>(QStringLiteral("btn_Apply"));
    QVERIFY(quickPath);
    QVERIFY(apply);

    quickPath->setText(QStringLiteral("C:/Quick"));
    QTest::mouseClick(apply, Qt::LeftButton);

    QSettings settings(path, QSettings::IniFormat);
    QCOMPARE(settings.value(QStringLiteral("Snip/quick_save_path")).toString(), QStringLiteral("C:/Quick"));
}

void SettingsWindowTest::showsHotkeyRegistrationStatus()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    GlobalConfig::instance()->setConfigPath(directory.filePath(QStringLiteral("config.ini")));
    QVERIFY(GlobalConfig::instance()->setPreferences(ApplicationPreferences{}));

    SettingsWindow    window;
    const QStringList status_names = {
        QStringLiteral("label_HotkeyCaptureStatus"),
        QStringLiteral("label_HotkeyCaptureCopyStatus"),
        QStringLiteral("label_HotkeyClickThroughStatus"),
    };
    for (const QString &object_name : status_names) {
        auto *status = window.findChild<QLabel *>(object_name);
        QVERIFY2(status, qPrintable(object_name));
        QVERIFY(!status->text().isEmpty());
    }
}

void SettingsWindowTest::exposesAdditionalSnipOptions()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("config.ini"));
    GlobalConfig::instance()->setConfigPath(path);
    QVERIFY(GlobalConfig::instance()->setPreferences(ApplicationPreferences{}));

    SettingsWindow window;
    auto          *never_capture_cursor = window.findChild<QCheckBox *>(QStringLiteral("check_NeverCaptureCursor"));
    auto          *disable_mouse_scroll = window.findChild<QCheckBox *>(QStringLiteral("check_DisableMouseScroll"));
    auto          *uppercase_hex_color = window.findChild<QCheckBox *>(QStringLiteral("check_UppercaseHexColor"));
    auto          *apply = window.findChild<QPushButton *>(QStringLiteral("btn_Apply"));
    QVERIFY(never_capture_cursor);
    QVERIFY(disable_mouse_scroll);
    QVERIFY(uppercase_hex_color);
    QVERIFY(apply);

    never_capture_cursor->setChecked(true);
    disable_mouse_scroll->setChecked(true);
    uppercase_hex_color->setChecked(false);
    QTest::mouseClick(apply, Qt::LeftButton);

    const CaptureOptions options = GlobalConfig::instance()->getPreferences().capture;
    QVERIFY(options.neverCaptureCursor);
    QVERIFY(options.disableMouseScroll);
    QVERIFY(!options.uppercaseHexColor);
}

void SettingsWindowTest::exposesSelectionUnitSetting()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    GlobalConfig::instance()->setConfigPath(directory.filePath(QStringLiteral("config.ini")));
    QVERIFY(GlobalConfig::instance()->setPreferences(ApplicationPreferences{}));

    SettingsWindow window;
    auto          *physical_units = window.findChild<QCheckBox *>(QStringLiteral("check_PhysicalSelectionSize"));
    auto          *apply = window.findChild<QPushButton *>(QStringLiteral("btn_Apply"));
    QVERIFY(physical_units);
    QVERIFY(apply);

    physical_units->setChecked(true);
    QTest::mouseClick(apply, Qt::LeftButton);
    QVERIFY(GlobalConfig::instance()->getPreferences().capture.physicalSelectionUnits);
}

void SettingsWindowTest::exposesCaptureStartModes()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("config.ini"));
    GlobalConfig::instance()->setConfigPath(path);
    QVERIFY(GlobalConfig::instance()->setPreferences(ApplicationPreferences{}));

    SettingsWindow    window;
    const QStringList hotkey_names = {
        QStringLiteral("hotkey_CaptureFullDesktop"),
        QStringLiteral("hotkey_CaptureLastArea"),
        QStringLiteral("hotkey_CaptureActiveScreen"),
        QStringLiteral("hotkey_CaptureFixedSize"),
    };
    for (const QString &object_name : hotkey_names) {
        QVERIFY2(window.findChild<QWidget *>(object_name), qPrintable(object_name));
    }
    QVERIFY(window.findChild<QSpinBox *>(QStringLiteral("spin_FixedCaptureWidth")));
    QVERIFY(window.findChild<QSpinBox *>(QStringLiteral("spin_FixedCaptureHeight")));
}

void SettingsWindowTest::appliesPinAndOcrSettings()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    GlobalConfig::instance()->setConfigPath(directory.filePath(QStringLiteral("config.ini")));
    QVERIFY(GlobalConfig::instance()->setPreferences(ApplicationPreferences{}));

    SettingsWindow window;
    QVERIFY(window.findChild<QWidget *>(QStringLiteral("page_Pin")));
    QVERIFY(window.findChild<QWidget *>(QStringLiteral("page_OCR")));
    auto *shadow = window.findChild<QCheckBox *>(QStringLiteral("check_PinShadow"));
    auto *stay_on_top = window.findChild<QCheckBox *>(QStringLiteral("check_PinStayOnTop"));
    auto *smoothScaling = window.findChild<QCheckBox *>(QStringLiteral("check_PinSmoothScaling"));
    auto *zoomAtCursor = window.findChild<QCheckBox *>(QStringLiteral("check_PinZoomAtCursor"));
    auto *recoveryLimit = window.findChild<QSpinBox *>(QStringLiteral("spin_PinRecoveryLimit"));
    auto *snapDistance = window.findChild<QSpinBox *>(QStringLiteral("spin_PinSnapDistance"));
    auto *font_size = window.findChild<QSpinBox *>(QStringLiteral("spin_TextImageFontSize"));
    auto *ocr_language = window.findChild<QComboBox *>(QStringLiteral("combo_OcrLanguage"));
    auto *apply = window.findChild<QPushButton *>(QStringLiteral("btn_Apply"));
    QVERIFY(shadow);
    QVERIFY(stay_on_top);
    QVERIFY(smoothScaling);
    QVERIFY(zoomAtCursor);
    QVERIFY(recoveryLimit);
    QVERIFY(snapDistance);
    QVERIFY(font_size);
    QVERIFY(ocr_language);
    QVERIFY(apply);

    shadow->setChecked(false);
    stay_on_top->setChecked(false);
    smoothScaling->setChecked(false);
    zoomAtCursor->setChecked(false);
    recoveryLimit->setValue(9);
    snapDistance->setValue(14);
    font_size->setValue(22);
    ocr_language->setCurrentIndex(0);
    QTest::mouseClick(apply, Qt::LeftButton);

    const ApplicationPreferences preferences = GlobalConfig::instance()->getPreferences();
    QVERIFY(!preferences.pin.showShadow);
    QVERIFY(!preferences.pin.stayOnTop);
    QVERIFY(!preferences.pin.smoothScaling);
    QVERIFY(!preferences.pin.zoomAtCursor);
    QCOMPARE(preferences.pin.recoveryLimit, 9);
    QCOMPARE(preferences.pin.snapDistancePx, 14);
    QCOMPARE(preferences.pin.textToImage.fontSize, 22);
}

void SettingsWindowTest::rendersSettingsPages()
{
    const QString render_directory = qEnvironmentVariable("QSNP_RENDER_DIR");
    if (render_directory.isEmpty()) {
        QSKIP("QSNP_RENDER_DIR is not set");
    }

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    GlobalConfig::instance()->setConfigPath(directory.filePath(QStringLiteral("config.ini")));
    QVERIFY(GlobalConfig::instance()->setPreferences(ApplicationPreferences{}));

    SettingsWindow window;
    window.show();
    auto *pages = window.findChild<QListWidget *>(QStringLiteral("list_SettingsPages"));
    QVERIFY(pages);
    for (int index = 0; index < pages->count(); ++index) {
        pages->setCurrentRow(index);
        QTest::qWait(20);
        const QPixmap image = window.grab();
        QVERIFY(!image.isNull());
        QVERIFY(image.save(render_directory + QStringLiteral("/settings-page-%1.png").arg(index + 1)));
    }
}

QTEST_MAIN(SettingsWindowTest)

#include "settingswindowtest.moc"
