#include "uiinspector.h"
#include <QDebug>
#include <QElapsedTimer>

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
        CONTROLTYPEID controlType;
        element->get_CurrentControlType(&controlType);

        auto qrect = getElementRect(element);
        //        qDebug() << "result: " << getControlTypeName(controlType) << elementName << qrect;
        element->Release();
        return qrect;
    }

    return QRect();
}

QString UIInspector::getControlTypeName(CONTROLTYPEID controlType)
{
    switch (controlType) {
    case UIA_ButtonControlTypeId:
        return "Button";
    case UIA_CalendarControlTypeId:
        return "Calendar";
    case UIA_CheckBoxControlTypeId:
        return "CheckBox";
    case UIA_ComboBoxControlTypeId:
        return "ComboBox";
    case UIA_EditControlTypeId:
        return "Edit";
    case UIA_HyperlinkControlTypeId:
        return "Hyperlink";
    case UIA_ImageControlTypeId:
        return "Image";
    case UIA_ListItemControlTypeId:
        return "ListItem";
    case UIA_ListControlTypeId:
        return "List";
    case UIA_MenuControlTypeId:
        return "Menu";
    case UIA_MenuBarControlTypeId:
        return "MenuBar";
    case UIA_MenuItemControlTypeId:
        return "MenuItem";
    case UIA_ProgressBarControlTypeId:
        return "ProgressBar";
    case UIA_RadioButtonControlTypeId:
        return "RadioButton";
    case UIA_ScrollBarControlTypeId:
        return "ScrollBar";
    case UIA_SliderControlTypeId:
        return "Slider";
    case UIA_SpinnerControlTypeId:
        return "Spinner";
    case UIA_StatusBarControlTypeId:
        return "StatusBar";
    case UIA_TabControlTypeId:
        return "Tab";
    case UIA_TabItemControlTypeId:
        return "TabItem";
    case UIA_TextControlTypeId:
        return "Text";
    case UIA_ToolBarControlTypeId:
        return "ToolBar";
    case UIA_ToolTipControlTypeId:
        return "ToolTip";
    case UIA_TreeControlTypeId:
        return "Tree";
    case UIA_TreeItemControlTypeId:
        return "TreeItem";
    case UIA_WindowControlTypeId:
        return "Window";
    case UIA_DocumentControlTypeId:
        return "Document";
    case UIA_GroupControlTypeId:
        return "Group";
    default:
        return QString("Unknown%1").arg((int) controlType);
    }
}

QRect UIInspector::getElementRect(IUIAutomationElement *element)
{
    RECT rect;
    element->get_CurrentBoundingRectangle(&rect);
    return QRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
    //    return QString("(%1,%2,%3,%4)").arg(rect.left).arg(rect.top).arg(rect.right).arg(rect.bottom);
}

QString UIInspector::getElementStates(IUIAutomationElement *element)
{
    QStringList states;

    BOOL value;
    element->get_CurrentIsEnabled(&value);
    if (!value)
        states << "Disabled";

    element->get_CurrentIsOffscreen(&value);
    if (value)
        states << "Offscreen";

    element->get_CurrentHasKeyboardFocus(&value);
    if (value)
        states << "Focused";

    return states.join(", ");
}
