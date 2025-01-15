#pragma once

#include <UIAutomation.h>
#include <windows.h>
#include <QPoint>
#include <QRect>
#include <QString>

class UIInspector
{
public:
    UIInspector();
    ~UIInspector();

    QRect quickInspect(const POINT &pt);
    QRect quickInspect(const QPoint &pt);

private:
    QRect getElementRect(IUIAutomationElement *element);

    IUIAutomation *m_automation;
};
