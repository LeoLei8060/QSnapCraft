#include "app/diagnostics.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest>

class DiagnosticsTest : public QObject
{
    Q_OBJECT

private slots:
    void writesDiagnosticMessagesToRequestedDirectory();
    void redactsUserProfileFromDiagnosticMessages();
    void writesCrashReportWithoutSensitiveData();
};

void DiagnosticsTest::writesDiagnosticMessagesToRequestedDirectory()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QString error;

    QVERIFY2(installDiagnostics(directory.path(), &error), qPrintable(error));
    qWarning() << "release diagnostic probe";
    shutdownDiagnostics();

    QFile logFile(QDir(directory.path()).filePath(QStringLiteral("QSnapCraft.log")));
    QVERIFY(logFile.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString contents = QString::fromUtf8(logFile.readAll());
    QVERIFY(contents.contains(QStringLiteral("release diagnostic probe")));
}

void DiagnosticsTest::redactsUserProfileFromDiagnosticMessages()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QString error;

    QVERIFY2(installDiagnostics(directory.path(), &error), qPrintable(error));
    qWarning() << QDir::homePath() + QStringLiteral("/private/capture.png");
    shutdownDiagnostics();

    QFile logFile(QDir(directory.path()).filePath(QStringLiteral("QSnapCraft.log")));
    QVERIFY(logFile.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString contents = QString::fromUtf8(logFile.readAll());
    QVERIFY(contents.contains(QStringLiteral("%USERPROFILE%/private/capture.png")));
    QVERIFY(!contents.contains(QDir::homePath()));
}

void DiagnosticsTest::writesCrashReportWithoutSensitiveData()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    QProcess            process;
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("QSNAPCRAFT_DIAGNOSTICS_SUPPRESS_UI"), QStringLiteral("1"));
    process.setProcessEnvironment(environment);
    process.start(QStringLiteral(QSNAPCRAFT_CRASH_PROBE_PATH), {directory.path()});
    QVERIFY(process.waitForStarted(3000));
    QVERIFY(process.waitForFinished(5000));
    QVERIFY(process.exitStatus() == QProcess::CrashExit || process.exitCode() != 0);

    const QStringList crashFiles = QDir(directory.path())
                                       .entryList({QStringLiteral("QSnapCraft-crash-*.log")}, QDir::Files);
    QCOMPARE(crashFiles.size(), 1);
    QFile crashFile(QDir(directory.path()).filePath(crashFiles.constFirst()));
    QVERIFY(crashFile.open(QIODevice::ReadOnly));
    const QByteArray bytes = crashFile.readAll();
    QString          report;
    if (bytes.size() >= 2) {
        report = QString::fromUtf16(reinterpret_cast<const char16_t *>(bytes.constData()), bytes.size() / 2);
    }
    QVERIFY(report.contains(QStringLiteral("Exception code:")));
    QVERIFY(report.contains(QStringLiteral("Thread ID:")));
    QVERIFY(!report.contains(QDir::homePath()));
}

QTEST_GUILESS_MAIN(DiagnosticsTest)

#include "diagnosticstest.moc"
