#ifndef WIN_QSCREENSAVER_H
#define WIN_QSCREENSAVER_H
#include "qabstractscreensaver.h"

class QScreenSaver : public QAbstractScreenSaver
{
    Q_OBJECT
public:
    explicit QScreenSaver(QObject *parent = NULL);

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

#endif // WIN_QSCREENSAVER_H
