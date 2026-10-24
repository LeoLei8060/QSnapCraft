#include "outputservice.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QStringList>

namespace {
QString resolvedFormat(QString format)
{
    format = format.trimmed().toLower();
    if (format.startsWith('.')) {
        format.remove(0, 1);
    }
    return format.isEmpty() ? QStringLiteral("png") : format;
}

QString expandPattern(QString pattern)
{
    const QDateTime now = QDateTime::currentDateTime();
    pattern.replace(QStringLiteral("$yyyy"), now.toString(QStringLiteral("yyyy")));
    pattern.replace(QStringLiteral("$MM"), now.toString(QStringLiteral("MM")));
    pattern.replace(QStringLiteral("$dd"), now.toString(QStringLiteral("dd")));
    pattern.replace(QStringLiteral("$HH"), now.toString(QStringLiteral("HH")));
    pattern.replace(QStringLiteral("$mm"), now.toString(QStringLiteral("mm")));
    pattern.replace(QStringLiteral("$ss"), now.toString(QStringLiteral("ss")));
    pattern.replace(QStringLiteral("$"), QString());
    return pattern;
}

QString outputPath(const QString &directory, const QString &pattern, const QString &format)
{
    const QString resolvedDirectory = resolveCaptureSaveDirectory(directory);

    QString fileName = expandPattern(pattern.trimmed());
    if (fileName.isEmpty()) {
        fileName = QStringLiteral("QSnapCraft_%1")
                       .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss")));
    }

    const QString extension = resolvedFormat(format);
    if (QFileInfo(fileName).suffix().isEmpty()) {
        fileName += QStringLiteral(".") + extension;
    }
    return QDir(resolvedDirectory).filePath(fileName);
}

QStringList filterExtensions(const QString &selected_filter_or_format)
{
    QStringList extensions;
    QString     patterns = selected_filter_or_format;
    const int   open = patterns.indexOf('(');
    const int   close = patterns.lastIndexOf(')');
    if (open >= 0 && close > open) {
        patterns = patterns.mid(open + 1, close - open - 1);
    }

    for (const QString &token : patterns.split(' ', Qt::SkipEmptyParts)) {
        QString extension = token.trimmed();
        if (extension.startsWith(QStringLiteral("*."))) {
            extension.remove(0, 2);
        } else if (extension.startsWith('.')) {
            extension.remove(0, 1);
        } else if (!extension.contains('.')) {
            continue;
        }
        if (!extension.isEmpty() && !extensions.contains(extension, Qt::CaseInsensitive)) {
            extensions.append(extension.toLower());
        }
    }

    if (extensions.isEmpty() && !selected_filter_or_format.trimmed().isEmpty()) {
        extensions.append(resolvedFormat(selected_filter_or_format));
    }
    return extensions;
}
} // namespace

bool OutputService::copyImage(const QImage &image, QString *error)
{
    if (image.isNull()) {
        if (error != nullptr) {
            *error = QStringLiteral("截图内容为空。");
        }
        return false;
    }
    QApplication::clipboard()->setImage(image);
    return true;
}

bool OutputService::saveImage(
    const QImage &image, const QString &path, const QString &format, int quality, QString *error)
{
    if (image.isNull()) {
        if (error != nullptr) {
            *error = QStringLiteral("截图内容为空。");
        }
        return false;
    }

    if (path.trimmed().isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("保存路径为空。");
        }
        return false;
    }

    const QString directory = QFileInfo(path).absolutePath();
    if (!directory.isEmpty() && !QDir().mkpath(directory)) {
        if (error != nullptr) {
            *error = QStringLiteral("无法创建保存目录：%1").arg(directory);
        }
        return false;
    }
    QString effectiveFormat = QFileInfo(path).suffix();
    if (effectiveFormat.isEmpty()) {
        effectiveFormat = resolvedFormat(format);
    }
    const QByteArray suffix = resolvedFormat(effectiveFormat).toLatin1();
    if (!image.save(path, suffix.constData(), quality)) {
        if (error != nullptr) {
            *error = QStringLiteral("保存截图失败：%1").arg(path);
        }
        return false;
    }
    return true;
}

QString OutputService::quickSave(const QImage &image, const CaptureOptions &options, QString *error)
{
    return saveSnapshot(image,
                        options.quickSavePath,
                        options.filenamePattern,
                        options.imageFormat,
                        options.jpegQuality,
                        error);
}

QString OutputService::saveSnapshot(const QImage  &image,
                                    const QString &directory,
                                    const QString &pattern,
                                    const QString &format,
                                    int            quality,
                                    QString       *error)
{
    const QString path = outputPath(directory, pattern, format);
    return saveImage(image, path, format, quality, error) ? path : QString();
}

QString OutputService::chooseSavePath(QWidget *parent, const CaptureOptions &options, const QImage &image)
{
    const QString suggested = outputPath(options.manualSavePath, options.filenamePattern, options.imageFormat);
    QString       selected_filter;
    const QString path = QFileDialog::getSaveFileName(parent,
                                                      QStringLiteral("保存截图"),
                                                      suggested,
                                                      QStringLiteral("PNG 图像 (*.png);;"
                                                                     "JPEG 图像 (*.jpg *.jpeg);;"
                                                                     "BMP 图像 (*.bmp);;WebP 图像 (*.webp)"),
                                                      &selected_filter);
    return ensurePathExtension(path, selected_filter);
}

QString OutputService::ensurePathExtension(const QString &path, const QString &selectedFilterOrFormat)
{
    if (path.trimmed().isEmpty()) {
        return {};
    }

    const QStringList extensions = filterExtensions(selectedFilterOrFormat);
    if (extensions.isEmpty()) {
        return path;
    }

    const QString suffix = QFileInfo(path).suffix();
    if (extensions.contains(suffix, Qt::CaseInsensitive)) {
        return path;
    }

    const QString extension = extensions.first();
    if (suffix.isEmpty()) {
        return path + QStringLiteral(".") + extension;
    }
    return path.left(path.size() - suffix.size()) + extension;
}
