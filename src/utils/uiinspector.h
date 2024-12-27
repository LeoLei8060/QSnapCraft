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
    QRect getElementRect(IUIAutomationElement *element);

    IUIAutomation *m_automation;
};
