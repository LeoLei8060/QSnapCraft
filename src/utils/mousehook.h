#pragma once

#include <windows.h>
#include <QObject>

class MouseHook : public QObject
{
    Q_OBJECT

public:
    explicit MouseHook(QObject *parent = nullptr);
    ~MouseHook() override;

    void install();
    void uninstall();

    static LRESULT CALLBACK mouseProc(int code, WPARAM wParam, LPARAM lParam);

signals:
    void mouseMove(const POINT &pt);
    void buttonLDown(const POINT &pt);
    void buttonLUp(const POINT &pt);
    void buttonRDown(const POINT &pt);
    void buttonRUp(const POINT &pt);

private:
    HHOOK             m_mouseHook;
    static MouseHook *instance;
    //    static HWND       m_targetWindow;
};
