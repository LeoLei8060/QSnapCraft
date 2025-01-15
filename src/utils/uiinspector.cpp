#include "uiinspector.h"
#include <QDebug>

UIInspector::UIInspector()
    : m_automation(nullptr)
{
    HRESULT hr = CoInitialize(nullptr);
    if (SUCCEEDED(hr)) {
        hr = CoCreateInstance(__uuidof(CUIAutomation),
                              nullptr,
                              CLSCTX_INPROC_SERVER,
                              __uuidof(IUIAutomation),
                              reinterpret_cast<void **>(&m_automation));
        if (FAILED(hr)) {
            qDebug() << "Failed to create UIAutomation instance";
        }
    }
}

UIInspector::~UIInspector()
{
    if (m_automation) {
        m_automation->Release();
    }
    CoUninitialize();
}

QRect UIInspector::quickInspect(const POINT &pt)
{
    if (!m_automation)
        return QRect();

    IUIAutomationElement *element = nullptr;
    HRESULT               hr = m_automation->ElementFromPoint(pt, &element);

    if (SUCCEEDED(hr) && element) {
        auto qrect = getElementRect(element);
        //        qDebug() << "result: " << getControlTypeName(controlType) << elementName << qrect;
        element->Release();
        return qrect;
    }

    return QRect();
}

QRect UIInspector::quickInspect(const QPoint &pt)
{
    if (!m_automation)
        return QRect();

    POINT point;
    point.x = pt.x();
    point.y = pt.y();
    IUIAutomationElement *element = nullptr;
    HRESULT               hr = m_automation->ElementFromPoint(point, &element);

    if (SUCCEEDED(hr) && element) {
        auto qrect = getElementRect(element);
        element->Release();
        return qrect;
    }

    return QRect();
}

QRect UIInspector::getElementRect(IUIAutomationElement *element)
{
    RECT rect;
    element->get_CurrentBoundingRectangle(&rect);
    return QRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
}
