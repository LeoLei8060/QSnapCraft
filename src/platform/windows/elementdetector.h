#pragma once

#include <QPoint>
#include <QRect>
#include <QString>
#include <QVector>

#include <windows.h>

struct ElementCandidate
{
    QRect   rect;
    QString name;
    QString className;
    bool    fromAccessibility{false};

    bool isValid() const { return rect.isValid() && rect.width() >= 2 && rect.height() >= 2; }
};

class ElementDetector
{
public:
    explicit ElementDetector(HWND self = nullptr);

    void refresh();
    int  windowCount() const;

    ElementCandidate          detectAt(const QPoint &screenPosition) const;
    QVector<ElementCandidate> detectHierarchyAt(const QPoint &screenPosition) const;

    static QVector<QRect> enumerateWindowRects();

private:
    struct WindowEntry
    {
        HWND  window{nullptr};
        QRect rect;
    };

    ElementCandidate          probeWindow(HWND window, const QPoint &screenPosition) const;
    QVector<ElementCandidate> probeHierarchy(HWND window, const QPoint &screenPosition) const;

    HWND                 m_self{nullptr};
    QVector<WindowEntry> m_windows;
};
