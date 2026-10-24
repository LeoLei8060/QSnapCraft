#include "config/preferencessettings.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

class PreferencesSettingsTest : public QObject
{
    Q_OBJECT

private slots:
    void roundTripsApplicationPreferences();
    void writesOutputSettingsToSnipGroup();
    void preservesUnknownSettings();
    void migratesLegacyDoubleClickCloseShortcut();
};

void PreferencesSettingsTest::roundTripsApplicationPreferences()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("config.ini"));
    QSettings     settings(path, QSettings::IniFormat);

    ApplicationPreferences expected;
    expected.general.autoStart = false;
    expected.control.captureHotkey = QStringLiteral("F8");
    expected.control.captureCopyHotkey = QStringLiteral("Ctrl+F8");
    expected.captureBar.enabled = false;
    expected.captureBar.hotspotWidthPx = 7;
    expected.capture.detectWindows = false;
    expected.capture.enterAction = CaptureOutputAction::kSave;
    expected.capture.autoSave = true;
    expected.capture.soundEnabled = true;
    expected.capture.quickSavePath = QStringLiteral("C:/Quick");
    expected.capture.imageFormat = QStringLiteral("JPG");
    expected.pin.recoveryLimit = 7;
    expected.pin.snapDistancePx = 11;
    expected.pin.smoothScaling = false;
    expected.pin.zoomAtCursor = false;

    saveApplicationPreferences(settings, expected);
    settings.sync();
    const ApplicationPreferences actual = loadApplicationPreferences(settings);

    QCOMPARE(actual.general.autoStart, expected.general.autoStart);
    QCOMPARE(actual.control.captureHotkey, expected.control.captureHotkey);
    QCOMPARE(actual.control.captureCopyHotkey, expected.control.captureCopyHotkey);
    QCOMPARE(settings.value(QStringLiteral("Control/ClickThroughHotkey")).toString(), QStringLiteral("F4"));
    QCOMPARE(settings.value(QStringLiteral("Snip/use_physical_selection_size")).toBool(), false);
    QCOMPARE(actual.captureBar.enabled, expected.captureBar.enabled);
    QCOMPARE(actual.captureBar.hotspotWidthPx, expected.captureBar.hotspotWidthPx);
    QCOMPARE(actual.capture.detectWindows, expected.capture.detectWindows);
    QCOMPARE(static_cast<int>(actual.capture.enterAction), static_cast<int>(expected.capture.enterAction));
    QCOMPARE(actual.capture.autoSave, expected.capture.autoSave);
    QCOMPARE(actual.capture.soundEnabled, expected.capture.soundEnabled);
    QCOMPARE(actual.capture.quickSavePath, expected.capture.quickSavePath);
    QCOMPARE(actual.capture.imageFormat, expected.capture.imageFormat);
    QCOMPARE(actual.pin.recoveryLimit, expected.pin.recoveryLimit);
    QCOMPARE(actual.pin.snapDistancePx, expected.pin.snapDistancePx);
    QCOMPARE(actual.pin.smoothScaling, expected.pin.smoothScaling);
    QCOMPARE(actual.pin.zoomAtCursor, expected.pin.zoomAtCursor);
}

void PreferencesSettingsTest::writesOutputSettingsToSnipGroup()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("config.ini"));

    ApplicationPreferences preferences;
    preferences.capture.manualSavePath = QStringLiteral("C:/Manual");
    preferences.capture.quickSavePath = QStringLiteral("C:/Quick");
    preferences.capture.autoSavePath = QStringLiteral("C:/Auto");
    preferences.capture.filenamePattern = QStringLiteral("Capture_$yyyy$MM$dd$");
    preferences.capture.imageFormat = QStringLiteral("PNG");

    {
        QSettings settings(path, QSettings::IniFormat);
        saveApplicationPreferences(settings, preferences);
        settings.sync();
    }

    QSettings settings(path, QSettings::IniFormat);
    QCOMPARE(settings.value(QStringLiteral("Snip/manual_save_path")).toString(), QStringLiteral("C:/Manual"));
    QCOMPARE(settings.value(QStringLiteral("Snip/quick_save_path")).toString(), QStringLiteral("C:/Quick"));
    QCOMPARE(settings.value(QStringLiteral("Snip/auto_save_path")).toString(), QStringLiteral("C:/Auto"));
    QCOMPARE(settings.value(QStringLiteral("Snip/filename_pattern")).toString(), QStringLiteral("Capture_$yyyy$MM$dd$"));
    QCOMPARE(settings.value(QStringLiteral("Snip/format")).toString(), QStringLiteral("PNG"));
    QVERIFY(!settings.contains(QStringLiteral("Output/FileNameFormat")));
}

void PreferencesSettingsTest::preservesUnknownSettings()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("config.ini"));
    QSettings     settings(path, QSettings::IniFormat);
    settings.setValue(QStringLiteral("Other/FutureValue"), QStringLiteral("keep-me"));

    saveApplicationPreferences(settings, ApplicationPreferences{});
    settings.sync();

    QCOMPARE(settings.value(QStringLiteral("Other/FutureValue")).toString(), QStringLiteral("keep-me"));
}

void PreferencesSettingsTest::migratesLegacyDoubleClickCloseShortcut()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("config.ini"));
    QSettings     settings(path, QSettings::IniFormat);
    settings.setValue(QStringLiteral("Control/ClosePinHotkey"), QStringLiteral("DoubleLeftClick"));

    const ApplicationPreferences preferences = loadApplicationPreferences(settings);
    QCOMPARE(preferences.control.closePinHotkey, QStringLiteral("F5"));
}

QTEST_APPLESS_MAIN(PreferencesSettingsTest)

#include "preferencessettingstest.moc"
