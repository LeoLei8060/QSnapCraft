#include "outputservice.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QStandardPaths>

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
    QString resolvedDirectory = directory.trimmed();
    if (resolvedDirectory.isEmpty()) {
        resolvedDirectory = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    }

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

    QDir().mkpath(QFileInfo(path).absolutePath());
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
    return QFileDialog::getSaveFileName(parent,
                                        QStringLiteral("保存截图"),
                                        suggested,
                                        QStringLiteral("图片 (*.png *.jpg *.jpeg *.bmp *.webp)"));
}
