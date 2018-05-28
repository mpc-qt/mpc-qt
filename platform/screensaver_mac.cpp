#include "screensaver_mac.h"

//FIXME: use IOPMAssertion???

QScreenSaverMac::QScreenSaverMac(QObject *parent) : QScreenSaver(parent)
{

}

QSet<QScreenSaverMac::Ability> QScreenSaverMac::abilities()
{
    return QSet<Ability>();
}

void QScreenSaverMac::inhibitSaver(const QString &reason)
{
    Q_UNUSED(reason);
    emit failed(Inhibit);
}

void QScreenSaverMac::uninhibitSaver() {
    emit failed(Uninhibit);
}

void QScreenSaverMac::launchSaver() {
    emit failed(LaunchSaver);
}

void QScreenSaverMac::lockScreen() {
    emit failed(LockScreen);
}

void QScreenSaverMac::hibernateSystem()
{
    emit failed(Hibernate);
}

void QScreenSaverMac::suspendSystem()
{
    emit failed(Suspend);
}

void QScreenSaverMac::shutdownSystem()
{
    emit failed(Shutdown);
}

void QScreenSaverMac::logOff()
{
    emit failed(LogOff);
}
