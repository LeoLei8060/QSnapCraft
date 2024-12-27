#pragma once

#include <UIAutomation.h>
#include <windows.h>
#include <QRect>
#include <QString>

class UIInspector
{
public:
    UIInspector();
    ~UIInspector();

    QRect quickInspect(const POINT &pt);

private:
    QString getControlTypeName(CONTROLTYPEID controlType);
    QRect   getElementRect(IUIAutomationElement *element);
    QString getElementStates(IUIAutomationElement *element);

    IUIAutomation *m_automation;
};
