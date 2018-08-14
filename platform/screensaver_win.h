#ifndef QSCREENSAVER_WIN_H
#define QSCREENSAVER_WIN_H
#include "screensaver.h"

class ScreenSaverWin : public ScreenSaver
{
    Q_OBJECT
public:
    explicit ScreenSaverWin(QObject *parent = nullptr);

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

private:
    bool canShutdown();
};

#endif // QSCREENSAVER_WIN_H
