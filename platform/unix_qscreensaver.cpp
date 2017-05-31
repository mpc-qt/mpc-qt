#include <QCoreApplication>
#include <QDBusPendingReply>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include "unix_qscreensaver.h"

QScreenSaver::QScreenSaver(QObject *parent)
    : QAbstractScreenSaver(parent),
      dbus("org.freedesktop.ScreenSaver",
           "/ScreenSaver",
           "org.freedesktop.ScreenSaver")
{

}

QSet<QScreenSaver::Ability> QScreenSaver::abilities()
{
    if (!dbus.isValid())
        return QSet<Ability>();

    return QSet<Ability>() << Inhibit << Uninhibit
                           << LaunchSaver << LockScreen;
}

void QScreenSaver::inhibitSaver(const QString &reason)
{
    if (isInhibiting || inhibitCalled)
        return;

    inhibitCalled = true;

    QDBusPendingCall async = dbus.asyncCall("Inhibit",
        QCoreApplication::applicationName(), reason);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(async, this);

    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, &QScreenSaver::dbus_inhibit);
}

void QScreenSaver::uninhibitSaver()
{
    if (!isInhibiting)
        return;

    QDBusPendingCall async = dbus.asyncCall("UnInhibit", inhibitToken);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(async, this);

    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, &QScreenSaver::dbus_uninhibit);

}

void QScreenSaver::launchSaver()
{
    QDBusPendingCall async = dbus.asyncCall("SetActive", true);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(async, this);

    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, &QScreenSaver::dbus_launch);

}

void QScreenSaver::lockScreen()
{
    QDBusPendingCall async = dbus.asyncCall("Lock");
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(async, this);

    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, &QScreenSaver::dbus_lock);

}

void QScreenSaver::dbus_inhibit(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<uint32_t> reply = *call;
    if (reply.isError()) {
        emit failed(Inhibit);
    } else {
        inhibitToken = reply.argumentAt<0>();
        isInhibiting = true;
        emit inhibitedSaver();
    }
    inhibitCalled = false;
    call->deleteLater();
}

void QScreenSaver::dbus_uninhibit(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<> reply = *call;
    if (reply.isError()) {
        emit failed(Uninhibit);
    } else {
        isInhibiting = false;
        emit uninhibitedSaver();
    }
    call->deleteLater();
}

void QScreenSaver::dbus_launch(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<> reply = *call;
    if (reply.isError())
        emit failed(LaunchSaver);
    else
        emit launchedSaver();
    call->deleteLater();
}

void QScreenSaver::dbus_lock(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<> reply = *call;
    if (reply.isError())
        emit failed(LockScreen);
    else
        emit screenLocked();
    call->deleteLater();
}
