#include <windows.h>
#include "win_qscreensaver.h"

QScreenSaver::QScreenSaver(QObject *parent) : QAbstractScreenSaver(parent)
{

}

QSet<QScreenSaver::Ability> QScreenSaver::abilities()
{
    return QSet<Ability>() << Inhibit << Uninhibit << LockScreen;
}

void QScreenSaver::inhibitSaver(const QString &reason)
{
    Q_UNUSED(reason);
    if (isInhibiting)
        return;

    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED);
    isInhibiting = true;
    emit inhibitedSaver();
}

void QScreenSaver::uninhibitSaver()
{
    if (!isInhibiting)
        return;

    SetThreadExecutionState(ES_CONTINUOUS);
    isInhibiting = false;
    emit uninhibitedSaver();
}

void QScreenSaver::launchSaver()
{
    emit failed(LaunchSaver);
}

void QScreenSaver::lockScreen()
{
    if (LockWorkStation())
        emit screenLocked();
    else
        emit failed(LockScreen);
}

void QScreenSaver::hibernateSystem()
{
    // FIXME
    emit failed(Hibernate);
}

void QScreenSaver::suspendSystem()
{
    // FIXME
    emit failed(Suspend);
}

void QScreenSaver::shutdownSystem()
{
    // FIXME
    emit failed(Shutdown);
}
