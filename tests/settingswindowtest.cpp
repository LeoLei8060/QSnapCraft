#include "config/globalconfig.h"
#include "ui/settings/settingswindow.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

class SettingsWindowTest : public QObject
{
    Q_OBJECT

private slots:
    void cancelDiscardsDraftChanges();
    void applyPersistsChanges();
    void outputChangesUseSnipSettings();
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
