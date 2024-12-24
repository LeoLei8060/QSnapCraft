#ifndef UIAUTOMATION_H
#define UIAUTOMATION_H

#include <QString>
#include <QRect>
#include <QPoint>
#include <vector>
#include <UIAutomation.h>

struct ControlInfo {
    QString type;
    QString name;
    QRect bounds;
    QString value;
    bool isEnabled;
};

struct WindowInfo {
    QString title;
    HWND handle;
    std::vector<ControlInfo> controls;
};

class UIAutomation {
public:
    UIAutomation();
    ~UIAutomation();

    std::vector<WindowInfo> getWindows();
    WindowInfo getWindowControls(HWND handle, const QPoint& mousePos = QPoint());

private:
    IUIAutomation* automation;
    void getControlsRecursive(IUIAutomationElement* element, std::vector<ControlInfo>& controls, const QPoint& mousePos = QPoint());
    QString getControlTypeName(CONTROLTYPEID typeId);
};

#endif // UIAUTOMATION_H
