#ifndef QSCREENSAVER_MAC_H
#define QSCREENSAVER_MAC_H
#include "screensaver.h"

class QScreenSaverMac : public QScreenSaver
{
    Q_OBJECT
public:
    QScreenSaverMac(QObject *parent = NULL);

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

#endif // QSCREENSAVER_MAC_H
