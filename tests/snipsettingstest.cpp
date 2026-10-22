#include "config/snipsettings.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

class SnipSettingsTest : public QObject
{
    Q_OBJECT

private slots:
    void loadsCaptureOptionsFromSnipGroup();
    void savesCaptureOptionsToSnipGroup();
};

void SnipSettingsTest::loadsCaptureOptionsFromSnipGroup()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("config.ini"));

    {
        QSettings settings(path, QSettings::IniFormat);
        settings.beginGroup(QStringLiteral("Snip"));
        settings.setValue(QStringLiteral("detect_windows"), false);
        settings.setValue(QStringLiteral("detect_ui_elements"), false);
        settings.setValue(QStringLiteral("enter_action"), 2);
        settings.setValue(QStringLiteral("enter_exit"), false);
        settings.setValue(QStringLiteral("border_width"), 6);
        settings.setValue(QStringLiteral("max_areas"), 12);
        settings.setValue(QStringLiteral("play_sound"), true);
        settings.endGroup();
    }

    QSettings            settings(path, QSettings::IniFormat);
    const CaptureOptions options = loadCaptureOptions(settings);

    QVERIFY(!options.detectWindows);
    QVERIFY(!options.detectUiElements);
    QCOMPARE(static_cast<int>(options.enterAction), static_cast<int>(CaptureOutputAction::kPin));
    QVERIFY(!options.enterExit);
    QCOMPARE(options.borderWidth, 6);
    QCOMPARE(options.maxHistoryAreas, 12);
    QVERIFY(options.soundEnabled);
}

void SnipSettingsTest::savesCaptureOptionsToSnipGroup()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("config.ini"));

    CaptureOptions options;
    options.detectWindows = false;
    options.enterAction = CaptureOutputAction::kPin;
    options.enterExit = false;
    options.borderWidth = 7;
    options.maxHistoryAreas = 18;
    options.autoSave = true;
    options.quickSavePath = QStringLiteral("C:/Quick");
    options.imageFormat = QStringLiteral("JPG");
    options.jpegQuality = 82;
    options.uppercaseHexColor = false;

    {
        QSettings settings(path, QSettings::IniFormat);
        saveCaptureOptions(settings, options);
        settings.sync();
    }

    QSettings settings(path, QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("Snip"));
    QCOMPARE(settings.value(QStringLiteral("detect_windows")).toBool(), false);
    QCOMPARE(settings.value(QStringLiteral("enter_action")).toInt(),
             static_cast<int>(CaptureOutputAction::kPin));
    QCOMPARE(settings.value(QStringLiteral("enter_exit")).toBool(), false);
    QCOMPARE(settings.value(QStringLiteral("border_width")).toInt(), 7);
    QCOMPARE(settings.value(QStringLiteral("max_areas")).toInt(), 18);
    QCOMPARE(settings.value(QStringLiteral("auto_save")).toBool(), true);
    QCOMPARE(settings.value(QStringLiteral("quick_save_path")).toString(), QStringLiteral("C:/Quick"));
    QCOMPARE(settings.value(QStringLiteral("format")).toString(), QStringLiteral("JPG"));
    QCOMPARE(settings.value(QStringLiteral("jpeg_quality")).toInt(), 82);
    settings.endGroup();
    settings.beginGroup(QStringLiteral("Misc"));
    QCOMPARE(settings.value(QStringLiteral("uppercase_hex_color")).toBool(), false);
}

QTEST_APPLESS_MAIN(SnipSettingsTest)

#include "snipsettingstest.moc"
