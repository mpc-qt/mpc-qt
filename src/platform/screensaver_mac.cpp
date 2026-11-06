#include "logger.h"
#include "screensaver_mac.h"

static constexpr char logModule[] =  "screensaver";

ScreenSaverMac::ScreenSaverMac(QObject *parent) : ScreenSaver(parent)
{

}

QSet<ScreenSaverMac::Ability> ScreenSaverMac::abilities()
{
    QSet<Ability> capabilities;
    capabilities << Inhibit << Uninhibit;
    return capabilities;
}

void ScreenSaverMac::inhibitSaver(const QString &reason)
{
    if (isInhibiting)
        return;

    CFStringRef cfReason = reason.toCFString();
    IOReturn result = IOPMAssertionCreateWithName(
        kIOPMAssertionTypePreventUserIdleDisplaySleep,
        kIOPMAssertionLevelOn, cfReason, &assertionId);
    CFRelease(cfReason);

    if (result != kIOReturnSuccess) {
        Logger::log(logModule, "failed to inhibit screensaver");
        emit failed(Inhibit);
        return;
    }
    isInhibiting = true;
    emit inhibitedSaver();
}

void ScreenSaverMac::uninhibitSaver()
{
    if (!isInhibiting)
        return;

    IOPMAssertionRelease(assertionId);
    assertionId = kIOPMNullAssertionID;
    isInhibiting = false;
    emit uninhibitedSaver();
}

void ScreenSaverMac::launchSaver()
{
    emit failed(LaunchSaver);
}

void ScreenSaverMac::lockScreen()
{
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
