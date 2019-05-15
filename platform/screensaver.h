#ifndef QABSTRACTSCREENSAVER_H
#define QABSTRACTSCREENSAVER_H

#include <QObject>
#include <QSet>

// This is probably overkill
class ScreenSaver : public QObject
{
    Q_OBJECT
public:
    enum Ability { Inhibit, Uninhibit, LaunchSaver, LockScreen,
                 Hibernate, Suspend, Shutdown, LogOff };

    explicit ScreenSaver(QObject *parent = nullptr);

    virtual QSet<Ability> abilities() = 0;
    virtual bool inhibiting();

public slots:
    virtual void inhibitSaver(const QString &reason) = 0;
    virtual void uninhibitSaver() = 0;
    virtual void launchSaver() = 0;
    virtual void lockScreen() = 0;
    virtual void hibernateSystem() = 0;
    virtual void suspendSystem() = 0;
    virtual void shutdownSystem() = 0;
    virtual void logOff() = 0;

signals:
    void inhibitedSaver();
    void uninhibitedSaver();
    void launchedSaver();
    void screenLocked();
    void systemHibernated();
    void systemSuspended();
    void systemShutdown();
    void loggedOff();
    void failed(ScreenSaver::Ability what);

protected:
    bool isInhibiting = false;
};

#endif // QABSTRACTSCREENSAVER_H
