#ifndef QABSTRACTSCREENSAVER_H
#define QABSTRACTSCREENSAVER_H

#include <QObject>
#include <QSet>

// This is probably overkill
class QAbstractScreenSaver : public QObject
{
    Q_OBJECT
public:
    enum Ability { Inhibit, Uninhibit, LaunchSaver, LockScreen };

    explicit QAbstractScreenSaver(QObject *parent = 0);

    virtual QSet<Ability> abilities() = 0;
    virtual bool inhibiting();

public slots:
    virtual void inhibitSaver(const QString &reason) = 0;
    virtual void uninhibitSaver() = 0;
    virtual void launchSaver() = 0;
    virtual void lockScreen() = 0;

signals:
    void inhibitedSaver();
    void uninhibitedSaver();
    void launchedSaver();
    void screenLocked();
    void failed(Ability what);

protected:
    bool isInhibiting = false;
};

#endif // QABSTRACTSCREENSAVER_H
