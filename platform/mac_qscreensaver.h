#ifndef MAC_QSCREENSAVER_H
#define MAC_QSCREENSAVER_H
#include "qabstractscreensaver.h"

class QScreenSaver : public QAbstractScreenSaver
{
    Q_OBJECT
public:
    QScreenSaver(QObject *parent = NULL);

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
};

#endif // MAC_QSCREENSAVER_H
