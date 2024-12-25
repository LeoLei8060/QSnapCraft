#include "uiautomation.h"
#include <QDebug>

UIAutomation::UIAutomation()
    : automation(nullptr)
{
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        qDebug() << "Failed to initialize COM";
        return;
    }

    hr = CoCreateInstance(CLSID_CUIAutomation,
                          nullptr,
                          CLSCTX_INPROC_SERVER,
                          IID_IUIAutomation,
                          reinterpret_cast<void **>(&automation));

    if (FAILED(hr)) {
        qDebug() << "Failed to create UI Automation instance";
    }
}

UIAutomation::~UIAutomation()
{
    if (automation) {
        automation->Release();
    }
    CoUninitialize();
}

std::vector<WindowInfo> UIAutomation::getWindows()
{
    std::vector<WindowInfo> windows;
    if (!automation)
        return windows;

    IUIAutomationElement *root = nullptr;
    HRESULT               hr = automation->GetRootElement(&root);
    if (FAILED(hr))
        return windows;

    IUIAutomationCondition *isWindow = nullptr;
    VARIANT                 varWindow;
    VariantInit(&varWindow);
    varWindow.vt = VT_BOOL;
    varWindow.boolVal = VARIANT_TRUE;
    hr = automation->CreatePropertyCondition(UIA_IsWindowPatternAvailablePropertyId,
                                             varWindow,
                                             &isWindow);
    VariantClear(&varWindow);

    if (FAILED(hr)) {
        root->Release();
        return windows;
    }

    IUIAutomationElementArray *windowArray = nullptr;
    hr = root->FindAll(TreeScope_Descendants, isWindow, &windowArray);
    if (SUCCEEDED(hr) && windowArray) {
        int count;
        windowArray->get_Length(&count);
        for (int i = 0; i < count; i++) {
            IUIAutomationElement *window = nullptr;
            if (SUCCEEDED(windowArray->GetElement(i, &window))) {
                BSTR name;
                if (SUCCEEDED(window->get_CurrentName(&name))) {
                    HWND handle;
                    window->get_CurrentNativeWindowHandle(reinterpret_cast<UIA_HWND *>(&handle));
                    if (handle && IsWindowVisible(handle)) {
                        WindowInfo info;
                        info.title = QString::fromWCharArray(name);
                        info.handle = handle;
                        windows.push_back(info);
                    }
                    SysFreeString(name);
                }
                window->Release();
            }
        }
        windowArray->Release();
    }
    isWindow->Release();
    root->Release();

    return windows;
}

WindowInfo UIAutomation::getWindowControls(HWND handle, const QPoint &mousePos)
{
    WindowInfo info;
    info.handle = handle;

    wchar_t title[256];
    GetWindowTextW(handle, title, 256);
    info.title = QString::fromWCharArray(title);

    if (!automation)
        return info;

    IUIAutomationElement *element = nullptr;
    HRESULT               hr = automation->ElementFromHandle(handle, &element);
    if (FAILED(hr) || !element)
        return info;

    getControlsRecursive(element, info.controls, mousePos);
    element->Release();

    return info;
}

void UIAutomation::getControlsRecursive(IUIAutomationElement     *element,
                                        std::vector<ControlInfo> &controls,
                                        const QPoint             &mousePos)
{
    if (!element)
        return;

    CONTROLTYPEID typeId;
    element->get_CurrentControlType(&typeId);
    QString controlType = getControlTypeName(typeId);

    if (controlType != "Unknown") {
        BOOL isEnabled;
        element->get_CurrentIsEnabled(&isEnabled);

        if (isEnabled) {
            RECT rect;
            element->get_CurrentBoundingRectangle(&rect);
            QRect bounds(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);

            if (mousePos.isNull() || bounds.contains(mousePos)) {
                ControlInfo info;
                info.bounds = bounds;
                info.isEnabled = true;
                info.type = controlType;
                controls.push_back(info);
            }
        }
    }

    IUIAutomationTreeWalker *walker = nullptr;
    automation->get_ControlViewWalker(&walker);
    if (!walker)
        return;

    IUIAutomationElement *child = nullptr;
    walker->GetFirstChildElement(element, &child);

    while (child) {
        getControlsRecursive(child, controls, mousePos);

        IUIAutomationElement *next = nullptr;
        walker->GetNextSiblingElement(child, &next);

        child->Release();
        child = next;
    }

    walker->Release();
}

QString UIAutomation::getControlTypeName(CONTROLTYPEID typeId)
{
    switch (typeId) {
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
    default:
        return "Unknown";
    }
}
