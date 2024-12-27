#ifndef MOUSEHOOK_H
#define MOUSEHOOK_H

#include <windows.h>
#include <QObject>

class MouseHook : public QObject
{
    Q_OBJECT
public:
    explicit MouseHook(QObject *parent = nullptr);
    ~MouseHook();

    void install();
    void uninstall();

signals:
    void mouseMove(const POINT &pt);
    void buttonLDown(const POINT &pt);
    void buttonLUp(const POINT &pt);
    void buttonRDown(const POINT &pt);
    void buttonRUp(const POINT &pt);

private:
    static LRESULT CALLBACK mouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    static MouseHook       *instance;
    HHOOK                   m_hook;
};

#endif
