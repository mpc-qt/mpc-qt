#ifndef UNIX_QSCREENSAVER_H
#define UNIX_QSCREENSAVER_H

#include <QObject>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusMessage>
#include "qabstractscreensaver.h"

Q_DECLARE_METATYPE(unsigned);
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

private slots:
    void dbus_inhibit(QDBusPendingCallWatcher *call);
    void dbus_uninhibit(QDBusPendingCallWatcher *call);
    void dbus_launch(QDBusPendingCallWatcher *call);
    void dbus_lock(QDBusPendingCallWatcher *call);

private:
    QDBusInterface dbus;
    uint32_t inhibitToken;
    bool inhibitCalled;
};

#endif // UNIX_QSCREENSAVER_H
