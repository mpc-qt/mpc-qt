#ifndef QSCREENSAVER_UNIX_H
#define QSCREENSAVER_UNIX_H

#include <QObject>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusMessage>
#include "screensaver.h"

Q_DECLARE_METATYPE(unsigned)
Q_DECLARE_METATYPE(uint64_t)

class ScreenSaverUnix : public ScreenSaver
{
    Q_OBJECT
public:
    explicit ScreenSaverUnix(QObject *parent = nullptr);

    QSet<Ability> abilities();

public slots:
    void inhibitSaver(const QString &reason);
    void uninhibitSaver();
    void launchSaver();
    void lockScreen();
    void hibernateSystem();
    void suspendSystem();
    void shutdownSystem();
    void logOff();

private slots:
    void dbusScreensaver_inhibit(QDBusPendingCallWatcher *call);
    void dbusScreensaver_uninhibit(QDBusPendingCallWatcher *call);
    void dbusScreensaver_launch(QDBusPendingCallWatcher *call);
    void dbusScreensaver_lock(QDBusPendingCallWatcher *call);
    void dbusLogin_hibernate(QDBusPendingCallWatcher *call);
    void dbusLogin_suspend(QDBusPendingCallWatcher *call);
    void dbusLogin_shutdown(QDBusPendingCallWatcher *call);
    void dbusLoginSelf_logoff(QDBusPendingCallWatcher *call);

private:
    QDBusInterface dbusScreensaver;
    QDBusInterface dbusLogin;
    QDBusInterface dbusLoginSelf;
    uint32_t inhibitToken = 0;
    bool inhibitCalled = false;
};

#endif // QSCREENSAVER_UNIX_H
