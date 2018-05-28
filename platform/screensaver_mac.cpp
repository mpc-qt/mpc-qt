#include "screensaver_mac.h"

//FIXME: use IOPMAssertion???

ScreenSaverMac::ScreenSaverMac(QObject *parent) : ScreenSaver(parent)
{

}

QSet<ScreenSaverMac::Ability> ScreenSaverMac::abilities()
{
    return QSet<Ability>();
}

void ScreenSaverMac::inhibitSaver(const QString &reason)
{
    Q_UNUSED(reason);
    emit failed(Inhibit);
}

void ScreenSaverMac::uninhibitSaver() {
    emit failed(Uninhibit);
}

void ScreenSaverMac::launchSaver() {
    emit failed(LaunchSaver);
}

void ScreenSaverMac::lockScreen() {
    emit failed(LockScreen);
}

void ScreenSaverMac::hibernateSystem()
{
    emit failed(Hibernate);
}

void ScreenSaverMac::suspendSystem()
{
    emit failed(Suspend);
}

void ScreenSaverMac::shutdownSystem()
{
    emit failed(Shutdown);
}

void ScreenSaverMac::logOff()
{
    emit failed(LogOff);
}
