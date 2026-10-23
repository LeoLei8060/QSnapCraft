#include "ocrengine.h"

#include <cstring>
#include <future>
#include <string>
#include <thread>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/base.h>

namespace {
using winrt::Windows::Globalization::Language;
using winrt::Windows::Graphics::Imaging::BitmapAlphaMode;
using winrt::Windows::Graphics::Imaging::BitmapPixelFormat;
using winrt::Windows::Graphics::Imaging::SoftwareBitmap;
using winrt::Windows::Media::Ocr::OcrEngine;
using winrt::Windows::Media::Ocr::OcrResult;
using winrt::Windows::Storage::Streams::Buffer;

bool initializeWinRt(QString *error)
{
    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        return true;
    } catch (const winrt::hresult_error &exception) {
        if (error != nullptr) {
            *error = QString::fromStdWString(exception.message().c_str());
        }
        return false;
    }
}

OcrEngine createEngine(const QString &language, QString *error)
{
    OcrEngine engine = nullptr;
    if (!language.trimmed().isEmpty()) {
        engine = OcrEngine::TryCreateFromLanguage(Language(winrt::hstring(language.toStdWString())));
    }
    if (!engine) {
        engine = OcrEngine::TryCreateFromUserProfileLanguages();
    }
    if (!engine) {
        engine = OcrEngine::TryCreateFromLanguage(Language(L"en-US"));
    }
    if (!engine && error != nullptr) {
        *error = QStringLiteral("系统没有可用的 OCR 语言包。");
    }
    return engine;
}

OcrRecognitionResult recognizeOnMtaThread(const QImage &image, const QString &language)
{
    OcrRecognitionResult result;
    if (!initializeWinRt(&result.error)) {
        return result;
    }

    const OcrEngine engine = createEngine(language, &result.error);
    if (!engine) {
        return result;
    }

    try {
        const QImage   converted = image.convertToFormat(QImage::Format_ARGB32);
        const int      row_bytes = converted.width() * 4;
        const uint32_t buffer_size = static_cast<uint32_t>(row_bytes * converted.height());
        Buffer         buffer(buffer_size);
        buffer.Length(buffer_size);
        auto *target = buffer.data();
        for (int y = 0; y < converted.height(); ++y) {
            std::memcpy(target + y * row_bytes, converted.constScanLine(y), static_cast<std::size_t>(row_bytes));
        }

        const SoftwareBitmap                                   bitmap = SoftwareBitmap::CreateCopyFromBuffer(buffer,
                                                                           BitmapPixelFormat::Bgra8,
                                                                           converted.width(),
                                                                           converted.height(),
                                                                           BitmapAlphaMode::Premultiplied);
        winrt::Windows::Foundation::IAsyncOperation<OcrResult> operation = engine.RecognizeAsync(bitmap);
        const OcrResult                                        recognition = operation.get();
        result.text = QString::fromStdWString(std::wstring(recognition.Text().c_str()));
        result.success = true;
    } catch (const winrt::hresult_error &exception) {
        result.error = QString::fromStdWString(exception.message().c_str());
    }
    return result;
}
} // namespace

bool isOcrEngineAvailable()
{
    std::promise<bool> promise;
    std::future<bool>  future = promise.get_future();
    std::thread([promise = std::move(promise)]() mutable {
        QString error;
        promise.set_value(initializeWinRt(&error) && createEngine({}, &error) != nullptr);
    }).detach();
    return future.get();
}

OcrRecognitionResult recognizeImageText(const QImage &image, const QString &language)
{
    if (image.isNull()) {
        OcrRecognitionResult result;
        result.error = QStringLiteral("OCR 输入图像为空。");
        return result;
    }

    std::promise<OcrRecognitionResult> promise;
    std::future<OcrRecognitionResult>  future = promise.get_future();
    std::thread([promise = std::move(promise), image, language]() mutable {
        promise.set_value(recognizeOnMtaThread(image, language));
    }).detach();
    return future.get();
}
