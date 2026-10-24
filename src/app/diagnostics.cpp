#include "diagnostics.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>

#include <windows.h>

#include <cwchar>
#include <string>

namespace {
constexpr qint64 MAX_LOG_BYTES = 2 * 1024 * 1024;
constexpr int    CRASH_PATH_CAPACITY = 32768;
constexpr int    CRASH_VERSION_CAPACITY = 64;

QFile                        g_logFile;
QMutex                       g_logMutex;
QtMessageHandler             g_previousHandler{nullptr};
LPTOP_LEVEL_EXCEPTION_FILTER g_previousFilter{nullptr};
wchar_t                      g_crashDirectory[CRASH_PATH_CAPACITY]{};
wchar_t                      g_crashVersion[CRASH_VERSION_CAPACITY]{};
bool                         g_installed{false};

QString sanitizeDiagnosticMessage(QString message)
{
    const QString homeDirectory = QDir::homePath();
    if (!homeDirectory.isEmpty()) {
        message.replace(homeDirectory, QStringLiteral("%USERPROFILE%"), Qt::CaseInsensitive);
        message.replace(QDir::toNativeSeparators(homeDirectory), QStringLiteral("%USERPROFILE%"), Qt::CaseInsensitive);
    }
    return message;
}

QString messageLevel(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
        return QStringLiteral("DEBUG");
    case QtInfoMsg:
        return QStringLiteral("INFO");
    case QtWarningMsg:
        return QStringLiteral("WARN");
    case QtCriticalMsg:
        return QStringLiteral("ERROR");
    case QtFatalMsg:
        return QStringLiteral("FATAL");
    }
    return QStringLiteral("LOG");
}

void diagnosticMessageHandler(QtMsgType type, const QMessageLogContext &, const QString &message)
{
    const QString line = QStringLiteral("%1 [%2] %3\r\n")
                             .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs),
                                  messageLevel(type),
                                  sanitizeDiagnosticMessage(message));

    const QMutexLocker locker(&g_logMutex);
    if (g_logFile.isOpen()) {
        g_logFile.write(line.toUtf8());
        g_logFile.flush();
    }
}

void writeCrashReport(EXCEPTION_POINTERS *exceptionPointers)
{
    if (g_crashDirectory[0] == L'\0') {
        return;
    }

    SYSTEMTIME utcTime{};
    GetSystemTime(&utcTime);

    wchar_t   crashPath[CRASH_PATH_CAPACITY]{};
    const int pathLength = swprintf_s(crashPath,
                                      CRASH_PATH_CAPACITY,
                                      L"%ls\\QSnapCraft-crash-%04u%02u%02u-%02u%02u%02u.log",
                                      g_crashDirectory,
                                      utcTime.wYear,
                                      utcTime.wMonth,
                                      utcTime.wDay,
                                      utcTime.wHour,
                                      utcTime.wMinute,
                                      utcTime.wSecond);
    if (pathLength <= 0) {
        return;
    }

    const DWORD exceptionCode = exceptionPointers != nullptr && exceptionPointers->ExceptionRecord != nullptr
                                    ? exceptionPointers->ExceptionRecord->ExceptionCode
                                    : 0;
    const void *exceptionAddress = exceptionPointers != nullptr && exceptionPointers->ExceptionRecord != nullptr
                                       ? exceptionPointers->ExceptionRecord->ExceptionAddress
                                       : nullptr;

    wchar_t   record[1024]{};
    const int recordLength = swprintf_s(record,
                                        1024,
                                        L"QSnapCraft crash report\r\n"
                                        L"Version: %ls\r\n"
                                        L"UTC: %04u-%02u-%02uT%02u:%02u:%02uZ\r\n"
                                        L"Exception code: 0x%08lX\r\n"
                                        L"Exception address: %p\r\n"
                                        L"Process ID: %lu\r\n"
                                        L"Thread ID: %lu\r\n",
                                        g_crashVersion,
                                        utcTime.wYear,
                                        utcTime.wMonth,
                                        utcTime.wDay,
                                        utcTime.wHour,
                                        utcTime.wMinute,
                                        utcTime.wSecond,
                                        exceptionCode,
                                        exceptionAddress,
                                        GetCurrentProcessId(),
                                        GetCurrentThreadId());
    if (recordLength <= 0) {
        return;
    }

    HANDLE file
        = CreateFileW(crashPath, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD         bytesWritten = 0;
    const wchar_t byteOrderMark = 0xFEFF;
    WriteFile(file, &byteOrderMark, sizeof(byteOrderMark), &bytesWritten, nullptr);
    WriteFile(file, record, static_cast<DWORD>(recordLength * sizeof(wchar_t)), &bytesWritten, nullptr);
    CloseHandle(file);
}

LONG WINAPI diagnosticCrashHandler(EXCEPTION_POINTERS *exceptionPointers)
{
    writeCrashReport(exceptionPointers);
    wchar_t suppressUi[2]{};
    if (GetEnvironmentVariableW(L"QSNAPCRAFT_DIAGNOSTICS_SUPPRESS_UI", suppressUi, 2) == 0) {
        MessageBoxW(nullptr,
                    L"QSnapCraft encountered an unexpected error and must close. "
                    L"A diagnostic report was saved in the application data directory.",
                    L"QSnapCraft",
                    MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
} // namespace

QString getDiagnosticsDirectory()
{
    const QString appDataDirectory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    return QDir(appDataDirectory).filePath(QStringLiteral("logs"));
}

bool installDiagnostics(const QString &directory, QString *error)
{
    const QString diagnosticsDirectory = directory.trimmed().isEmpty() ? getDiagnosticsDirectory() : directory;
    if (!QDir().mkpath(diagnosticsDirectory)) {
        if (error != nullptr) {
            *error = QStringLiteral("无法创建诊断日志目录。");
        }
        return false;
    }

    if (g_installed) {
        return true;
    }

    const QString   logPath = QDir(diagnosticsDirectory).filePath(QStringLiteral("QSnapCraft.log"));
    const QFileInfo logInfo(logPath);
    if (logInfo.exists() && logInfo.size() > MAX_LOG_BYTES) {
        const QString previousLogPath = logPath + QStringLiteral(".1");
        QFile::remove(previousLogPath);
        QFile::rename(logPath, previousLogPath);
    }

    g_logFile.setFileName(logPath);
    if (!g_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        if (error != nullptr) {
            *error = QStringLiteral("无法打开诊断日志文件。");
        }
        return false;
    }

    const std::wstring nativeDirectory = QDir::toNativeSeparators(diagnosticsDirectory).toStdWString();
    const std::wstring version = QCoreApplication::applicationVersion().toStdWString();
    wcsncpy_s(g_crashDirectory, nativeDirectory.c_str(), _TRUNCATE);
    wcsncpy_s(g_crashVersion, version.c_str(), _TRUNCATE);

    g_previousHandler = qInstallMessageHandler(diagnosticMessageHandler);
    g_previousFilter = SetUnhandledExceptionFilter(diagnosticCrashHandler);
    g_installed = true;
    return true;
}

void shutdownDiagnostics()
{
    if (!g_installed) {
        return;
    }

    {
        const QMutexLocker locker(&g_logMutex);
        g_logFile.flush();
        g_logFile.close();
    }
    qInstallMessageHandler(g_previousHandler);
    SetUnhandledExceptionFilter(g_previousFilter);
    g_previousHandler = nullptr;
    g_previousFilter = nullptr;
    g_crashDirectory[0] = L'\0';
    g_crashVersion[0] = L'\0';
    g_installed = false;
}
