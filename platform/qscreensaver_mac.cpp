#include "qscreensaver_mac.h"

//FIXME: use IOPMAssertion???

QScreenSaver::QScreenSaver(QObject *parent) : QAbstractScreenSaver(parent)
{

}

QSet<QScreenSaver::Ability> QScreenSaver::abilities()
{
    return QSet<Ability>();
}

void QScreenSaver::inhibitSaver(const QString &reason)
{
    Q_UNUSED(reason);
    emit failed(Inhibit);
}

void QScreenSaver::uninhibitSaver() {
    emit failed(Uninhibit);
}

void QScreenSaver::launchSaver() {
    emit failed(LaunchSaver);
}

void QScreenSaver::lockScreen() {
    emit failed(LockScreen);
}

void QScreenSaver::hibernateSystem()
{
    emit failed(Hibernate);
}

void QScreenSaver::suspendSystem()
{
    emit failed(Suspend);
}

void QScreenSaver::shutdownSystem()
{
    emit failed(Shutdown);
}

void QScreenSaver::logOff()
{
    emit failed(LogOff);
}
