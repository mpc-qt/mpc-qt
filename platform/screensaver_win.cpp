#include <windows.h>
#include <powrprof.h>
#include "screensaver_win.h"

QScreenSaverWin::QScreenSaverWin(QObject *parent) : QScreenSaver(parent)
{

}

QSet<QScreenSaverWin::Ability> QScreenSaverWin::abilities()
{
    QSet<Ability> capabilities;
    capabilities << Inhibit << Uninhibit << LockScreen;
    if (canShutdown())
        capabilities << Shutdown << Hibernate << Suspend << LogOff;
    return capabilities;
}

void QScreenSaverWin::inhibitSaver(const QString &reason)
{
    Q_UNUSED(reason);
    if (isInhibiting)
        return;

    SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED |
                            ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED);
    isInhibiting = true;
    emit inhibitedSaver();
}

void QScreenSaverWin::uninhibitSaver()
{
    if (!isInhibiting)
        return;

    SetThreadExecutionState(ES_CONTINUOUS);
    isInhibiting = false;
    emit uninhibitedSaver();
}

void QScreenSaverWin::launchSaver()
{
    emit failed(LaunchSaver);
}

void QScreenSaverWin::lockScreen()
{
    if (LockWorkStation())
        emit screenLocked();
    else
        emit failed(LockScreen);
}

void QScreenSaverWin::hibernateSystem()
{
    if (SetSuspendState(TRUE, FALSE, FALSE))
        emit systemHibernated();
    else
        emit failed(Hibernate);
}

void QScreenSaverWin::suspendSystem()
{
    if(SetSuspendState(FALSE, FALSE, FALSE))
        emit systemSuspended();
    else
        emit failed(Suspend);
}

void QScreenSaverWin::shutdownSystem()
{
    if (ExitWindowsEx(EWX_SHUTDOWN, 0))
        emit systemShutdown();
    else
        emit failed(Shutdown);
}

void QScreenSaverWin::logOff()
{
    if (ExitWindowsEx(EWX_LOGOFF, 0))
        emit loggedOff();
    else
        emit failed(LogOff);
}

bool QScreenSaverWin::canShutdown()
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    // Get a token for this process.
    if (!OpenProcessToken(GetCurrentProcess(),
         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
       return false;

    // Get the LUID for the shutdown privilege.
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,
         &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;  // one privilege to set
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    // Get the shutdown privilege for this process.

    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
         (PTOKEN_PRIVILEGES)NULL, 0);

    return GetLastError() == ERROR_SUCCESS;
}

