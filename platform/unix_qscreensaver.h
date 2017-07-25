#ifndef UNIX_QSCREENSAVER_H
#define UNIX_QSCREENSAVER_H

#include <QObject>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusMessage>
#include "qabstractscreensaver.h"

Q_DECLARE_METATYPE(unsigned)
Q_DECLARE_METATYPE(uint64_t)

class QScreenSaver : public QAbstractScreenSaver
{
    Q_OBJECT
public:
    explicit QScreenSaver(QObject *parent = 0);

    QSet<Ability> abilities();

public slots:
    void inhibitSaver(const QString &reason);
    void uninhibitSaver();
    void launchSaver();
    void lockScreen();
    void hibernateSystem();
    void suspendSystem();
    void shutdownSystem();

private slots:
    void dbusScreensaver_inhibit(QDBusPendingCallWatcher *call);
    void dbusScreensaver_uninhibit(QDBusPendingCallWatcher *call);
    void dbusScreensaver_launch(QDBusPendingCallWatcher *call);
    void dbusScreensaver_lock(QDBusPendingCallWatcher *call);
    void dbusLogin_hibernate(QDBusPendingCallWatcher *call);
    void dbusLogin_suspend(QDBusPendingCallWatcher *call);
    void dbusLogin_shutdown(QDBusPendingCallWatcher *call);

private:
    QDBusInterface dbusScreensaver;
    QDBusInterface dbusLogin;
    uint32_t inhibitToken = 0;
    bool inhibitCalled = false;
};

#endif // UNIX_QSCREENSAVER_H
