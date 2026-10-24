#include "pinpool.h"

#include "config/globalconfig.h"
#include "pinboard/pinimagelimits.h"
#include "pinboard/pinscreenfit.h"
#include "pinboard/pinsnapoverlay.h"
#include "pinboard/pinwindow.h"
#include "pinboard/textimagerenderer.h"
#include "platform/windows/elementdetector.h"

#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QCursor>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QImageReader>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMimeData>
#include <QSaveFile>
#include <QScreen>
#include <QTimer>
#include <QUuid>

#include <windows.h>

#include <algorithm>

PinPool::PinPool(const QString &storageDirectory, QObject *parent)
    : QObject(parent)
    , m_storageDirectory(storageDirectory)
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(250);
    connect(&m_saveTimer, &QTimer::timeout, this, &PinPool::savePersistedPins);
    m_foreignSnapTimer.setInterval(250);
    connect(&m_foreignSnapTimer, &QTimer::timeout, this, &PinPool::refreshForeignSnapTargets);
    m_foreignSnapTimer.start();
    connect(GlobalConfig::instance(), &GlobalConfig::configChanged, this, &PinPool::applyPinSettings);
    applyPinSettings();
    if (!m_storageDirectory.isEmpty()) {
        loadPersistedPins();
    }
}

PinPool::~PinPool()
{
    savePersistedPins();
    while (!m_pinWindows.empty()) {
        QPointer<PinWindow> pin_window = m_pinWindows.back();
        m_pinWindows.pop_back();
        delete pin_window.data();
    }
    m_recentClosedPins.clear();
    m_bulkHiddenPins.clear();
}

PinWindow *PinPool::pinImage(const QPixmap &pixmap, const QPoint &position, const QSize &logicalSize)
{
    if (pixmap.isNull()) {
        return nullptr;
    }

    auto *pin_window = new PinWindow();
    connect(pin_window, &QObject::destroyed, this, [this]() { removeNullPins(); });
    connect(pin_window, &PinWindow::sigCloseRequested, this, &PinPool::removeClosedPin);
    connect(pin_window, &PinWindow::sigContentGeometryChanged, this, [this](PinWindow *, const QRect &) {
        refreshSnapTargets();
    });
    connect(pin_window, &PinWindow::sigSnapGuidesChanged, this, &PinPool::updateSnapOverlay);
    connect(pin_window, &PinWindow::sigStateChanged, this, [this](PinWindow *) { schedulePersistenceSave(); });
    connect(pin_window, &PinWindow::sigGroupAssignmentRequested, this, &PinPool::assignPinGroup);
    connect(pin_window, &PinWindow::sigCreateGroupRequested, this, &PinPool::createGroupForPin);
    connect(pin_window, &PinWindow::sigGroupActionRequested, this, &PinPool::handleGroupAction);
    pin_window->setPinId(QUuid::createUuid().toString(QUuid::WithoutBraces));
    pin_window->setGroups(m_groups);
    const ApplicationPreferences preferences = GlobalConfig::instance()->getPreferences();
    pin_window->setShadowEnabled(preferences.pin.showShadow);
    pin_window->setStayOnTop(preferences.pin.stayOnTop);
    pin_window->setSnapDistancePx(preferences.pin.snapDistancePx);
    pin_window->setSmoothScaling(preferences.pin.smoothScaling);
    pin_window->setZoomAtCursor(preferences.pin.zoomAtCursor);
    QSize       contentSize = logicalSize.isValid() ? logicalSize : pixmap.deviceIndependentSize().toSize();
    QPixmap     limitedPixmap = pixmap;
    const QSize boundedSize = boundedPinImageSize(pixmap.size());
    if (boundedSize != pixmap.size()) {
        if (!logicalSize.isValid()) {
            contentSize = boundedSize;
        }
        limitedPixmap = pixmap.scaled(boundedSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        limitedPixmap.setDevicePixelRatio(pixmap.devicePixelRatio());
    }
    pin_window->pinImage(limitedPixmap, QRect(position, contentSize));
    m_pinWindows.push_back(pin_window);
    refreshSnapTargets();
    schedulePersistenceSave();
    return pin_window;
}

int PinPool::pinCount() const
{
    return static_cast<int>(m_pinWindows.size());
}

void PinPool::setRecoveryLimit(int limit)
{
    m_recoveryLimit = qBound(0, limit, 50);
    pruneRecoveryPool();
}

const QVector<PinGroupData> &PinPool::groups() const
{
    return m_groups;
}

QString PinPool::createGroup(const QString &name)
{
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty()) {
        return {};
    }
    const PinGroupData group{QUuid::createUuid().toString(QUuid::WithoutBraces), trimmedName};
    m_groups.append(group);
    refreshGroupMenus();
    schedulePersistenceSave();
    return group.id;
}

bool PinPool::pasteFromClipboard()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    if (mimeData != nullptr) {
        for (const QString &format : {QStringLiteral("image/gif"), QStringLiteral("image/webp")}) {
            const QByteArray data = mimeData->data(format);
            if (data.isEmpty()) {
                continue;
            }
            QBuffer      buffer;
            QImageReader reader(&buffer, format.mid(6).toLatin1());
            buffer.setData(data);
            buffer.open(QIODevice::ReadOnly);
            const QImage firstFrame = reader.read();
            if (firstFrame.isNull()) {
                continue;
            }
            QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
            if (screen == nullptr) {
                screen = QGuiApplication::primaryScreen();
            }
            if (screen == nullptr) {
                return false;
            }
            const QSize  logicalSize = firstFrame.deviceIndependentSize().toSize();
            const QRect  available = screen->availableGeometry();
            const QPoint cursor = QCursor::pos();
            const int    maxX = qMax(available.left(), available.right() - logicalSize.width() + 1);
            const int    maxY = qMax(available.top(), available.bottom() - logicalSize.height() + 1);
            const QPoint position(qBound(available.left(), cursor.x() + 16, maxX),
                                  qBound(available.top(), cursor.y() + 16, maxY));
            PinWindow   *pinWindow = pinImage(QPixmap::fromImage(firstFrame), position, logicalSize);
            if (pinWindow != nullptr) {
                pinWindow->pinAnimation(data, format, QRect(position, logicalSize));
                return true;
            }
        }
    }

    QImage image = QApplication::clipboard()->image();
    if (image.isNull()) {
        if (mimeData == nullptr) {
            return false;
        }

        const TextImageFormat format = mimeData->hasHtml() ? TextImageFormat::kHtml : TextImageFormat::kPlainText;
        const QString         content = mimeData->hasHtml() ? mimeData->html() : mimeData->text();
        image = renderTextToImage(content, format, GlobalConfig::instance()->getPreferences().pin.textToImage);
        if (image.isNull()) {
            return false;
        }
    }

    image = limitPinImageSize(image);
    if (image.isNull()) {
        return false;
    }
    const QPixmap pixmap = QPixmap::fromImage(image);
    const QSize   image_size = pixmap.deviceIndependentSize().toSize();
    QScreen      *screen = QGuiApplication::screenAt(QCursor::pos());
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }
    if (screen == nullptr) {
        return false;
    }
    const QRect  available = screen->availableGeometry();
    const QPoint cursor = QCursor::pos();
    const int    max_x = qMax(available.left(), available.right() - image_size.width() + 1);
    const int    max_y = qMax(available.top(), available.bottom() - image_size.height() + 1);
    const QPoint position(qBound(available.left(), cursor.x() + 16, max_x),
                          qBound(available.top(), cursor.y() + 16, max_y));

    pinImage(pixmap, position, image_size);
    return true;
}

void PinPool::hideAllPins()
{
    removeNullPins();
    if (m_snapOverlay) {
        m_snapOverlay->setGuides({});
    }
    for (const QPointer<PinWindow> &pinWindow : m_pinWindows) {
        if (pinWindow != nullptr && pinWindow->isVisible()) {
            pinWindow->hide();
            if (std::find(m_bulkHiddenPins.begin(), m_bulkHiddenPins.end(), pinWindow) == m_bulkHiddenPins.end()) {
                m_bulkHiddenPins.emplace_back(pinWindow);
            }
        }
    }
    refreshSnapTargets();
    schedulePersistenceSave();
}

void PinPool::showAllPins()
{
    removeNullPins();
    for (const QPointer<PinWindow> &pinWindow : m_bulkHiddenPins) {
        if (pinWindow != nullptr) {
            pinWindow->show();
            pinWindow->raise();
        }
    }
    m_bulkHiddenPins.clear();
    refreshSnapTargets();
    schedulePersistenceSave();
}

void PinPool::togglePinsVisibility()
{
    removeNullPins();
    const bool has_visible_pin = std::any_of(m_pinWindows.begin(), m_pinWindows.end(), [](const auto &pin_window) {
        return pin_window != nullptr && pin_window->isVisible();
    });
    if (has_visible_pin) {
        hideAllPins();
    } else {
        showAllPins();
    }
}

bool PinPool::recoverLastPin()
{
    removeNullPins();
    while (!m_recentClosedPins.empty()) {
        QPointer<PinWindow> pin_window = m_recentClosedPins.back();
        m_recentClosedPins.pop_back();
        if (pin_window == nullptr) {
            continue;
        }
        pin_window->show();
        pin_window->raise();
        pin_window->activateWindow();
        pin_window->setFocus();
        refreshSnapTargets();
        schedulePersistenceSave();
        return true;
    }
    return false;
}

bool PinPool::closePinAt(const QPoint &globalPosition)
{
    PinWindow *pinWindow = pinWindowAt(globalPosition);
    if (pinWindow == nullptr) {
        return false;
    }
    pinWindow->close();
    return true;
}

bool PinPool::toggleClickThroughAt(const QPoint &global_position)
{
    PinWindow *pinWindow = pinWindowAt(global_position);
    if (pinWindow != nullptr) {
        pinWindow->setClickThrough(!pinWindow->isClickThrough());
        return true;
    }
    return false;
}

void PinPool::ensurePinsVisible()
{
    removeNullPins();
    const QList<QScreen *> screens = QGuiApplication::screens();
    if (screens.isEmpty()) {
        return;
    }

    QVector<QRect> availableScreens;
    for (QScreen *screen : screens) {
        availableScreens.append(screen->availableGeometry());
    }

    for (const QPointer<PinWindow> &pin_window : m_pinWindows) {
        if (pin_window == nullptr) {
            continue;
        }

        const QPoint position = clampPinToScreens(pin_window->frameGeometry(), availableScreens);
        pin_window->move(position);
    }
}

void PinPool::applyPinSettings()
{
    removeNullPins();
    const PinSettingsData settings = GlobalConfig::instance()->getPreferences().pin;
    setRecoveryLimit(settings.recoveryLimit);
    const bool shadowChanged = m_lastShowShadow != settings.showShadow;
    m_lastShowShadow = settings.showShadow;
    for (const QPointer<PinWindow> &pin_window : m_pinWindows) {
        if (pin_window != nullptr) {
            if (shadowChanged) {
                pin_window->setShadowEnabled(settings.showShadow);
            }
            pin_window->setStayOnTop(settings.stayOnTop);
            pin_window->setSnapDistancePx(settings.snapDistancePx);
            pin_window->setSmoothScaling(settings.smoothScaling);
            pin_window->setZoomAtCursor(settings.zoomAtCursor);
        }
    }
    refreshSnapTargets();
}

void PinPool::removeClosedPin(PinWindow *window)
{
    const auto existing = std::find(m_recentClosedPins.begin(), m_recentClosedPins.end(), window);
    if (existing != m_recentClosedPins.end()) {
        m_recentClosedPins.erase(existing);
    }
    if (m_snapOverlay) {
        m_snapOverlay->setGuides({});
    }
    const auto hidden = std::find(m_bulkHiddenPins.begin(), m_bulkHiddenPins.end(), window);
    if (hidden != m_bulkHiddenPins.end()) {
        m_bulkHiddenPins.erase(hidden);
    }
    if (m_recoveryLimit == 0) {
        const auto active = std::find(m_pinWindows.begin(), m_pinWindows.end(), window);
        if (active != m_pinWindows.end()) {
            m_pinWindows.erase(active);
        }
        window->deleteLater();
        QTimer::singleShot(0, this, [this]() { refreshSnapTargets(); });
        schedulePersistenceSave();
        return;
    }
    m_recentClosedPins.emplace_back(window);
    pruneRecoveryPool();
    QTimer::singleShot(0, this, [this]() { refreshSnapTargets(); });
    schedulePersistenceSave();
}

void PinPool::removeNullPins()
{
    const auto is_null = [](const QPointer<PinWindow> &pin_window) { return pin_window.isNull(); };
    m_pinWindows.erase(std::remove_if(m_pinWindows.begin(), m_pinWindows.end(), is_null), m_pinWindows.end());
    m_recentClosedPins.erase(std::remove_if(m_recentClosedPins.begin(), m_recentClosedPins.end(), is_null),
                             m_recentClosedPins.end());
    m_bulkHiddenPins.erase(std::remove_if(m_bulkHiddenPins.begin(), m_bulkHiddenPins.end(), is_null),
                           m_bulkHiddenPins.end());
}

void PinPool::pruneRecoveryPool()
{
    while (static_cast<int>(m_recentClosedPins.size()) > m_recoveryLimit) {
        QPointer<PinWindow> oldest = m_recentClosedPins.front();
        m_recentClosedPins.erase(m_recentClosedPins.begin());
        if (oldest == nullptr) {
            continue;
        }
        const auto active = std::find(m_pinWindows.begin(), m_pinWindows.end(), oldest);
        if (active != m_pinWindows.end()) {
            m_pinWindows.erase(active);
        }
        delete oldest.data();
    }
}

void PinPool::refreshSnapTargets()
{
    removeNullPins();
    for (const QPointer<PinWindow> &pinWindow : m_pinWindows) {
        if (pinWindow == nullptr) {
            continue;
        }
        if (!pinWindow->isVisible()) {
            pinWindow->setSnapTargets({});
            continue;
        }
        QVector<QRect> targets;
        for (const QPointer<PinWindow> &other : m_pinWindows) {
            if (other != nullptr && other != pinWindow && other->isVisible()) {
                targets.append(other->contentRect());
            }
        }
        targets += m_foreignSnapTargets;
        pinWindow->setSnapTargets(targets);
    }
}

void PinPool::refreshForeignSnapTargets()
{
    m_foreignSnapTargets = ElementDetector::enumerateWindowRects();
    if (!m_pinWindows.empty()) {
        refreshSnapTargets();
    }
}

void PinPool::updateSnapOverlay(PinWindow *window, const QVector<QLineF> &guides)
{
    Q_UNUSED(window)
    if (guides.isEmpty()) {
        if (m_snapOverlay) {
            m_snapOverlay->setGuides({});
        }
        return;
    }
    if (!m_snapOverlay) {
        m_snapOverlay = std::make_unique<PinSnapOverlay>();
    }
    m_snapOverlay->setGuides(guides);
}

void PinPool::schedulePersistenceSave()
{
    if (!m_loadingPersistence && !m_storageDirectory.isEmpty()) {
        m_saveTimer.start();
    }
}

void PinPool::savePersistedPins()
{
    if (m_storageDirectory.isEmpty()) {
        return;
    }
    QDir directory(m_storageDirectory);
    if (!QDir().mkpath(m_storageDirectory)) {
        return;
    }

    QJsonArray groupsArray;
    for (const PinGroupData &group : m_groups) {
        QJsonObject object;
        object.insert(QStringLiteral("id"), group.id);
        object.insert(QStringLiteral("name"), group.name);
        groupsArray.append(object);
    }

    QJsonArray pinsArray;
    for (const QPointer<PinWindow> &pinWindow : m_pinWindows) {
        if (pinWindow == nullptr) {
            continue;
        }
        const bool    animated = pinWindow->isAnimated();
        const QString format = animated
                                       && pinWindow->animationFormat().endsWith(QStringLiteral("webp"),
                                                                                Qt::CaseInsensitive)
                                   ? QStringLiteral("webp")
                                   : QStringLiteral("gif");
        const QString imageName = pinWindow->pinId()
                                  + (animated ? QStringLiteral(".") + format : QStringLiteral(".png"));
        QSaveFile imageFile(directory.filePath(imageName));
        if (!imageFile.open(QIODevice::WriteOnly)) {
            continue;
        }
        const bool imageWritten = animated ? (imageFile.write(pinWindow->animationData())
                                              == pinWindow->animationData().size())
                                           : pinWindow->currentImage().toImage().save(&imageFile, "PNG");
        if (!imageWritten || !imageFile.commit()) {
            continue;
        }
        const QRect content = pinWindow->contentRect();
        QJsonObject object;
        object.insert(QStringLiteral("id"), pinWindow->pinId());
        object.insert(QStringLiteral("groupId"), pinWindow->groupId());
        object.insert(QStringLiteral("image"), imageName);
        object.insert(QStringLiteral("animation"), animated);
        object.insert(QStringLiteral("format"), format);
        object.insert(QStringLiteral("x"), content.x());
        object.insert(QStringLiteral("y"), content.y());
        object.insert(QStringLiteral("width"), content.width());
        object.insert(QStringLiteral("height"), content.height());
        object.insert(QStringLiteral("opacity"), pinWindow->opacity());
        object.insert(QStringLiteral("clickThrough"), pinWindow->isClickThrough());
        object.insert(QStringLiteral("shadow"), pinWindow->shadowEnabled());
        object.insert(QStringLiteral("visible"), pinWindow->isVisible());
        pinsArray.append(object);
    }

    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("groups"), groupsArray);
    root.insert(QStringLiteral("pins"), pinsArray);
    QSaveFile file(directory.filePath(QStringLiteral("pins.json")));
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    file.commit();
}

void PinPool::loadPersistedPins()
{
    if (m_storageDirectory.isEmpty()) {
        return;
    }
    QFile file(QDir(m_storageDirectory).filePath(QStringLiteral("pins.json")));
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    m_groups.clear();
    for (const QJsonValue &value : root.value(QStringLiteral("groups")).toArray()) {
        const QJsonObject object = value.toObject();
        const QString     name = object.value(QStringLiteral("name")).toString();
        if (!name.isEmpty()) {
            m_groups.append(PinGroupData{object.value(QStringLiteral("id")).toString(), name});
        }
    }

    m_loadingPersistence = true;
    for (const QJsonValue &value : root.value(QStringLiteral("pins")).toArray()) {
        const QJsonObject object = value.toObject();
        const QString     imageName = object.value(QStringLiteral("image")).toString();
        const QString     imagePath = QDir(m_storageDirectory).filePath(imageName);
        QFile             imageFile(imagePath);
        if (!imageFile.open(QIODevice::ReadOnly)) {
            continue;
        }
        const QByteArray imageData = imageFile.readAll();
        QBuffer          imageBuffer;
        imageBuffer.setData(imageData);
        imageBuffer.open(QIODevice::ReadOnly);
        QImageReader  imageReader(&imageBuffer);
        const QImage  firstFrame = imageReader.read();
        const QPixmap pixmap = QPixmap::fromImage(firstFrame);
        if (pixmap.isNull()) {
            continue;
        }
        const QRect content(object.value(QStringLiteral("x")).toInt(),
                            object.value(QStringLiteral("y")).toInt(),
                            object.value(QStringLiteral("width")).toInt(),
                            object.value(QStringLiteral("height")).toInt());
        pinImage(pixmap, content.topLeft(), content.size());
        PinWindow *pinWindow = m_pinWindows.back();
        if (object.value(QStringLiteral("animation")).toBool(false)) {
            pinWindow->pinAnimation(imageData, object.value(QStringLiteral("format")).toString(), content);
        }
        pinWindow->setPinId(object.value(QStringLiteral("id")).toString());
        pinWindow->setGroup(object.value(QStringLiteral("groupId")).toString());
        pinWindow->setWindowOpacity(object.value(QStringLiteral("opacity")).toDouble(1.0));
        pinWindow->setShadowEnabled(object.value(QStringLiteral("shadow")).toBool(true));
        pinWindow->setClickThrough(object.value(QStringLiteral("clickThrough")).toBool(false));
        if (!object.value(QStringLiteral("visible")).toBool(true)) {
            pinWindow->hide();
            m_recentClosedPins.emplace_back(pinWindow);
        }
    }
    m_loadingPersistence = false;
    refreshGroupMenus();
    refreshSnapTargets();
}

void PinPool::createGroupForPin(PinWindow *window)
{
    bool          accepted = false;
    const QString name = QInputDialog::getText(window,
                                               QStringLiteral("新建贴图分组"),
                                               QStringLiteral("分组名称："),
                                               QLineEdit::Normal,
                                               QString(),
                                               &accepted)
                             .trimmed();
    if (!accepted) {
        return;
    }
    const QString groupId = createGroup(name);
    if (!groupId.isEmpty()) {
        assignPinGroup(window, groupId);
    }
}

void PinPool::assignPinGroup(PinWindow *window, const QString &groupId)
{
    if (window == nullptr) {
        return;
    }
    if (!groupId.isEmpty() && std::none_of(m_groups.cbegin(), m_groups.cend(), [&groupId](const PinGroupData &group) {
            return group.id == groupId;
        })) {
        return;
    }
    window->setGroup(groupId);
    schedulePersistenceSave();
}

void PinPool::refreshGroupMenus()
{
    removeNullPins();
    for (const QPointer<PinWindow> &pinWindow : m_pinWindows) {
        if (pinWindow != nullptr) {
            pinWindow->setGroups(m_groups);
        }
    }
}

bool PinPool::renameGroup(const QString &groupId, const QString &name)
{
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty()) {
        return false;
    }
    for (PinGroupData &group : m_groups) {
        if (group.id == groupId) {
            group.name = trimmedName;
            refreshGroupMenus();
            schedulePersistenceSave();
            return true;
        }
    }
    return false;
}

void PinPool::removeGroup(const QString &groupId)
{
    m_groups.erase(std::remove_if(m_groups.begin(),
                                  m_groups.end(),
                                  [&groupId](const PinGroupData &group) { return group.id == groupId; }),
                   m_groups.end());
    for (const QPointer<PinWindow> &pinWindow : m_pinWindows) {
        if (pinWindow != nullptr && pinWindow->groupId() == groupId) {
            pinWindow->setGroup({});
        }
    }
    refreshGroupMenus();
    schedulePersistenceSave();
}

void PinPool::setGroupVisible(const QString &groupId, bool visible)
{
    for (const QPointer<PinWindow> &pinWindow : m_pinWindows) {
        if (pinWindow == nullptr || pinWindow->groupId() != groupId) {
            continue;
        }
        if (visible) {
            const auto closed = std::find(m_recentClosedPins.begin(), m_recentClosedPins.end(), pinWindow);
            if (closed != m_recentClosedPins.end()) {
                m_recentClosedPins.erase(closed);
            }
            const auto hidden = std::find(m_bulkHiddenPins.begin(), m_bulkHiddenPins.end(), pinWindow);
            if (hidden != m_bulkHiddenPins.end()) {
                m_bulkHiddenPins.erase(hidden);
            }
            pinWindow->show();
            pinWindow->raise();
        } else {
            pinWindow->hide();
            if (std::find(m_bulkHiddenPins.begin(), m_bulkHiddenPins.end(), pinWindow) == m_bulkHiddenPins.end()) {
                m_bulkHiddenPins.emplace_back(pinWindow);
            }
        }
    }
    refreshSnapTargets();
    schedulePersistenceSave();
}

void PinPool::closeGroup(const QString &groupId)
{
    for (const QPointer<PinWindow> &pinWindow : m_pinWindows) {
        if (pinWindow != nullptr && pinWindow->groupId() == groupId && pinWindow->isVisible()) {
            pinWindow->close();
        }
    }
}

void PinPool::handleGroupAction(PinWindow *window, const QString &groupId, PinGroupAction action)
{
    switch (action) {
    case PinGroupAction::kShow:
        setGroupVisible(groupId, true);
        break;
    case PinGroupAction::kHide:
        setGroupVisible(groupId, false);
        break;
    case PinGroupAction::kClose:
        closeGroup(groupId);
        break;
    case PinGroupAction::kRename: {
        const auto existing = std::find_if(m_groups.cbegin(), m_groups.cend(), [&groupId](const PinGroupData &group) {
            return group.id == groupId;
        });
        if (existing == m_groups.cend()) {
            break;
        }
        bool          accepted = false;
        const QString name = QInputDialog::getText(window,
                                                   QStringLiteral("重命名分组"),
                                                   QStringLiteral("分组名称："),
                                                   QLineEdit::Normal,
                                                   existing->name,
                                                   &accepted)
                                 .trimmed();
        if (accepted) {
            renameGroup(groupId, name);
        }
        break;
    }
    case PinGroupAction::kRemove:
        removeGroup(groupId);
        break;
    }
}

PinWindow *PinPool::pinWindowAt(const QPoint &globalPosition)
{
    removeNullPins();

#ifdef Q_OS_WIN
    POINT point{globalPosition.x(), globalPosition.y()};
    HWND  underCursor = WindowFromPoint(point);
    if (underCursor != nullptr) {
        underCursor = GetAncestor(underCursor, GA_ROOT);
        for (const QPointer<PinWindow> &pinWindow : m_pinWindows) {
            if (pinWindow != nullptr && pinWindow->isVisible()
                && reinterpret_cast<HWND>(pinWindow->winId()) == underCursor) {
                return pinWindow;
            }
        }
    }

    for (HWND current = GetTopWindow(nullptr); current != nullptr; current = GetWindow(current, GW_HWNDNEXT)) {
        for (const QPointer<PinWindow> &pinWindow : m_pinWindows) {
            if (pinWindow != nullptr && pinWindow->isVisible() && reinterpret_cast<HWND>(pinWindow->winId()) == current
                && pinWindow->frameGeometry().contains(globalPosition)) {
                return pinWindow;
            }
        }
    }
#endif

    for (auto it = m_pinWindows.rbegin(); it != m_pinWindows.rend(); ++it) {
        PinWindow *pinWindow = *it;
        if (pinWindow != nullptr && pinWindow->isVisible() && pinWindow->frameGeometry().contains(globalPosition)) {
            return pinWindow;
        }
    }
    return nullptr;
}
