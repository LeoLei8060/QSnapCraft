#include "elementdetector.h"

#include <QElapsedTimer>

#include <oleacc.h>
#include <oleauto.h>

#include <algorithm>
#include <memory>

namespace {
constexpr int  MAX_WINDOWS = 400;
constexpr int  MSAA_MAX_STEPS = 61;
constexpr int  MSAA_TIME_LIMIT_MS = 201;
constexpr UINT WINDOW_PROBE_TIMEOUT_MS = 20;

template<typename T>
using ComPtr = std::unique_ptr<T, void (*)(T *)>;

template<typename T>
ComPtr<T> makeComPtr(T *pointer)
{
    return ComPtr<T>(pointer, [](T *value) {
        if (value != nullptr) {
            value->Release();
        }
    });
}

class VariantGuard
{
public:
    VariantGuard() { VariantInit(&m_value); }
    ~VariantGuard() { VariantClear(&m_value); }
    VARIANT *get() { return &m_value; }

private:
    VARIANT m_value;
};

void initializeCom()
{
    static const HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    Q_UNUSED(result)
}

bool isOwnWindow(HWND window)
{
    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    return processId == GetCurrentProcessId();
}

bool isDesktopWindow(HWND window)
{
    wchar_t   className[128]{};
    const int length = GetClassNameW(window, className, 128);
    if (length <= 0) {
        return false;
    }
    const QString name = QString::fromWCharArray(className, length);
    return name == QStringLiteral("Progman") || name == QStringLiteral("WorkerW");
}

bool isTaskbarWindow(HWND window)
{
    wchar_t   className[128]{};
    const int length = GetClassNameW(window, className, 128);
    if (length <= 0) {
        return false;
    }
    const QString name = QString::fromWCharArray(className, length);
    return name == QStringLiteral("Shell_TrayWnd") || name == QStringLiteral("Shell_SecondaryTrayWnd");
}

bool windowBounds(HWND window, QRect *rect)
{
    WINDOWINFO info{};
    info.cbSize = sizeof(info);
    if (!GetWindowInfo(window, &info)) {
        return false;
    }
    *rect = QRect(info.rcWindow.left,
                  info.rcWindow.top,
                  info.rcWindow.right - info.rcWindow.left,
                  info.rcWindow.bottom - info.rcWindow.top);
    return rect->isValid();
}

HWND foreignWindowUnderPoint(const QPoint &position)
{
    POINT point{position.x(), position.y()};
    HWND  current = WindowFromPoint(point);
    while (current != nullptr) {
        if (!isOwnWindow(current) && IsWindowVisible(current) && !IsIconic(current)) {
            const HWND root = GetAncestor(current, GA_ROOT);
            QRect      rect;
            if (root != nullptr && !isOwnWindow(root) && !isDesktopWindow(root) && windowBounds(root, &rect)
                && rect.contains(position)) {
                return root;
            }
        }
        current = GetWindow(current, GW_HWNDNEXT);
    }
    return nullptr;
}

bool canParticipate(HWND window, const QPoint &position, QRect *rect)
{
    if (window == nullptr || isOwnWindow(window) || !IsWindow(window) || !IsWindowVisible(window) || IsIconic(window)
        || IsHungAppWindow(window)) {
        return false;
    }
    if (!windowBounds(window, rect) || !rect->contains(position)) {
        return false;
    }

    ULONG_PTR response = 0;
    return SendMessageTimeoutW(window, WM_NULL, 0, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, WINDOW_PROBE_TIMEOUT_MS, &response)
           != 0;
}

QString windowClassName(HWND window)
{
    wchar_t   className[256]{};
    const int length = GetClassNameW(window, className, 256);
    return length > 0 ? QString::fromWCharArray(className, length) : QString();
}

QRect accessibleRect(IAccessible *accessible, VARIANT child)
{
    long left = 0;
    long top = 0;
    long width = 0;
    long height = 0;
    if (FAILED(accessible->accLocation(&left, &top, &width, &height, child)) || width <= 0 || height <= 0) {
        return {};
    }
    return QRect(left, top, width, height);
}

void descendAccessible(IAccessible *root, const QPoint &position, QVector<QRect> *hierarchy, QString *name)
{
    ComPtr<IAccessible> current = makeComPtr(root);
    if (current) {
        current->AddRef();
    }

    QElapsedTimer timer;
    timer.start();
    QVector<QRect> visited;

    for (int step = 0; step < MSAA_MAX_STEPS && current; ++step) {
        VariantGuard child;
        if (FAILED(current->accHitTest(position.x(), position.y(), child.get()))) {
            break;
        }

        ComPtr<IAccessible> next(nullptr, current.get_deleter());
        QRect               hit;
        if (child.get()->vt == VT_DISPATCH && child.get()->pdispVal != nullptr) {
            IAccessible *candidate = nullptr;
            if (SUCCEEDED(
                    child.get()->pdispVal->QueryInterface(IID_IAccessible, reinterpret_cast<void **>(&candidate)))) {
                next = makeComPtr(candidate);
                VARIANT self;
                VariantInit(&self);
                self.vt = VT_I4;
                self.lVal = CHILDID_SELF;
                hit = accessibleRect(next.get(), self);
            }
        } else if (child.get()->vt == VT_I4) {
            hit = accessibleRect(current.get(), *child.get());
        }

        if (!hit.isValid() || !hit.contains(position) || visited.contains(hit)) {
            break;
        }
        if (!hierarchy->isEmpty()
            && hit.width() * hit.height() >= hierarchy->last().width() * hierarchy->last().height()) {
            break;
        }

        visited.append(hit);
        hierarchy->append(hit);
        if (hit.width() <= 2 || hit.height() <= 2 || timer.elapsed() > MSAA_TIME_LIMIT_MS || !next) {
            break;
        }
        current = std::move(next);
    }

    if (!hierarchy->isEmpty() && name != nullptr && current) {
        VARIANT self;
        VariantInit(&self);
        self.vt = VT_I4;
        self.lVal = CHILDID_SELF;
        BSTR value = nullptr;
        if (SUCCEEDED(current->get_accName(self, &value)) && value != nullptr) {
            *name = QString::fromWCharArray(value);
            SysFreeString(value);
        }
    }
}
} // namespace

ElementDetector::ElementDetector(HWND self)
    : m_self(self)
{
    initializeCom();
}

void ElementDetector::refresh()
{
    initializeCom();
    m_windows.clear();
    HWND foregroundRoot = GetAncestor(GetForegroundWindow(), GA_ROOT);
    if (foregroundRoot == nullptr || isOwnWindow(foregroundRoot) || isDesktopWindow(foregroundRoot)) {
        foregroundRoot = nullptr;
    }

    EnumWindows(
        [](HWND window, LPARAM parameter) -> BOOL {
            auto *detector = reinterpret_cast<ElementDetector *>(parameter);
            if (detector->m_windows.size() >= MAX_WINDOWS) {
                return FALSE;
            }
            if (window == detector->m_self || isOwnWindow(window) || isDesktopWindow(window) || !IsWindowVisible(window)
                || IsIconic(window)) {
                return TRUE;
            }
            QRect rect;
            if (windowBounds(window, &rect)) {
                detector->m_windows.append(WindowEntry{window, rect});
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(this));

    const int topLevelCount = m_windows.size();
    for (int index = 0; index < topLevelCount && m_windows.size() < MAX_WINDOWS; ++index) {
        const HWND topLevel = m_windows.at(index).window;
        EnumChildWindows(
            topLevel,
            [](HWND child, LPARAM parameter) -> BOOL {
                auto *detector = reinterpret_cast<ElementDetector *>(parameter);
                if (detector->m_windows.size() >= MAX_WINDOWS) {
                    return FALSE;
                }
                if (child == detector->m_self || isOwnWindow(child) || !IsWindowVisible(child)) {
                    return TRUE;
                }
                QRect rect;
                if (windowBounds(child, &rect)) {
                    detector->m_windows.append(WindowEntry{child, rect});
                }
                return TRUE;
            },
            reinterpret_cast<LPARAM>(this));
    }

    if (foregroundRoot != nullptr) {
        std::stable_partition(m_windows.begin(), m_windows.end(), [foregroundRoot](const WindowEntry &entry) {
            return entry.window == foregroundRoot || GetAncestor(entry.window, GA_ROOT) == foregroundRoot;
        });
    }
}

int ElementDetector::windowCount() const
{
    return m_windows.size();
}

ElementCandidate ElementDetector::detectAt(const QPoint &screenPosition) const
{
    if (const HWND preferred = foreignWindowUnderPoint(screenPosition)) {
        const ElementCandidate candidate = probeWindow(preferred, screenPosition);
        if (candidate.isValid()) {
            return candidate;
        }
    }
    for (const WindowEntry &entry : m_windows) {
        if (!entry.rect.contains(screenPosition)) {
            continue;
        }
        const ElementCandidate candidate = probeWindow(entry.window, screenPosition);
        if (candidate.isValid()) {
            return candidate;
        }
    }
    return {};
}

QVector<ElementCandidate> ElementDetector::detectHierarchyAt(const QPoint &screenPosition) const
{
    if (const HWND preferred = foreignWindowUnderPoint(screenPosition)) {
        QVector<ElementCandidate> hierarchy = probeHierarchy(preferred, screenPosition);
        if (!hierarchy.isEmpty()) {
            return hierarchy;
        }
    }
    for (const WindowEntry &entry : m_windows) {
        if (!entry.rect.contains(screenPosition)) {
            continue;
        }
        QVector<ElementCandidate> hierarchy = probeHierarchy(entry.window, screenPosition);
        if (!hierarchy.isEmpty()) {
            return hierarchy;
        }
    }
    return {};
}

QVector<QRect> ElementDetector::enumerateWindowRects()
{
    initializeCom();
    QVector<QRect> result;
    EnumWindows(
        [](HWND window, LPARAM parameter) -> BOOL {
            auto *rects = reinterpret_cast<QVector<QRect> *>(parameter);
            if (rects->size() >= MAX_WINDOWS) {
                return FALSE;
            }
            if (isOwnWindow(window) || isDesktopWindow(window) || isTaskbarWindow(window) || !IsWindowVisible(window)
                || IsIconic(window)) {
                return TRUE;
            }
            QRect rect;
            if (windowBounds(window, &rect)) {
                rects->append(rect);
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&result));
    return result;
}

ElementCandidate ElementDetector::probeWindow(HWND window, const QPoint &screenPosition) const
{
    QRect windowRect;
    if (!canParticipate(window, screenPosition, &windowRect)) {
        return {};
    }

    ElementCandidate best;
    const LONG       objectIds[] = {OBJID_CLIENT, OBJID_WINDOW};
    for (LONG objectId : objectIds) {
        IAccessible *rawAccessible = nullptr;
        if (FAILED(
                AccessibleObjectFromWindow(window, objectId, IID_IAccessible, reinterpret_cast<void **>(&rawAccessible)))
            || rawAccessible == nullptr) {
            continue;
        }
        ComPtr<IAccessible> accessible = makeComPtr(rawAccessible);

        VariantGuard child;
        if (FAILED(accessible->accHitTest(screenPosition.x(), screenPosition.y(), child.get()))) {
            continue;
        }

        QRect hit;
        if (child.get()->vt == VT_I4) {
            hit = accessibleRect(accessible.get(), *child.get());
        } else if (child.get()->vt == VT_DISPATCH && child.get()->pdispVal != nullptr) {
            IAccessible *childAccessible = nullptr;
            if (SUCCEEDED(child.get()->pdispVal->QueryInterface(IID_IAccessible,
                                                                reinterpret_cast<void **>(&childAccessible)))) {
                ComPtr<IAccessible> ownedChild = makeComPtr(childAccessible);
                VARIANT             self;
                VariantInit(&self);
                self.vt = VT_I4;
                self.lVal = CHILDID_SELF;
                hit = accessibleRect(ownedChild.get(), self);
            }
        }

        if (hit.isValid() && hit.contains(screenPosition)
            && (!best.isValid()
                || qint64(hit.width()) * hit.height() < qint64(best.rect.width()) * best.rect.height())) {
            best.rect = hit;
            best.className = windowClassName(window);
            best.fromAccessibility = true;
        }
    }

    if (!best.isValid() && windowRect.width() >= 2 && windowRect.height() >= 2) {
        best.rect = windowRect;
        best.className = windowClassName(window);
    }
    return best;
}

QVector<ElementCandidate> ElementDetector::probeHierarchy(HWND window, const QPoint &screenPosition) const
{
    QRect windowRect;
    if (!canParticipate(window, screenPosition, &windowRect)) {
        return {};
    }

    QVector<ElementCandidate> result;
    result.append(ElementCandidate{windowRect, {}, windowClassName(window), false});

    const LONG     objectIds[] = {OBJID_CLIENT, OBJID_WINDOW};
    QVector<QRect> bestHierarchy;
    QString        bestName;
    for (LONG objectId : objectIds) {
        IAccessible *rawAccessible = nullptr;
        if (FAILED(
                AccessibleObjectFromWindow(window, objectId, IID_IAccessible, reinterpret_cast<void **>(&rawAccessible)))
            || rawAccessible == nullptr) {
            continue;
        }
        ComPtr<IAccessible> accessible = makeComPtr(rawAccessible);
        QVector<QRect>      hierarchy;
        QString             name;
        descendAccessible(accessible.get(), screenPosition, &hierarchy, &name);
        if (!hierarchy.isEmpty()
            && (bestHierarchy.isEmpty()
                || hierarchy.last().width() * hierarchy.last().height()
                       < bestHierarchy.last().width() * bestHierarchy.last().height())) {
            bestHierarchy = hierarchy;
            bestName = name;
        }
    }

    for (const QRect &rect : bestHierarchy) {
        if (result.last().rect == rect) {
            continue;
        }
        result.append(ElementCandidate{rect, {}, windowClassName(window), true});
    }
    if (!bestName.isEmpty() && result.size() > 1) {
        result.last().name = bestName;
    }
    return result;
}
