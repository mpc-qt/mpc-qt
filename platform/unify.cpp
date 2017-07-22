#include <QObject>
#include <QProcess>
#include "unify.h"

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
#include <dlfcn.h>
#endif


QString Platform::fixedConfigPath(QString configPath)
{
#ifdef Q_OS_WIN
    // backward compatability sucks
    configPath.replace("AppData/Local","AppData/Roaming");
#endif
    return configPath;
}

QString Platform::sanitizedFilename(QString fileName)
{
#ifdef Q_OS_WIN
    fileName.replace(':', '.');
#endif
    return fileName;
}

bool Platform::tiledDesktopsExist()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    return true;
#endif
    return false;
}

bool Platform::tilingDesktopActive()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    QProcessEnvironment env;
    QStringList tilers({ "awesome", "bspwm", "dwm", "i3", "larswm", "ion",
        "qtile", "ratpoison", "stumpwm", "wmii", "xmonad"});
    QString desktop = env.value("XDG_DESKTOP_SESSION");
    if (tilers.contains(desktop))
        return true;
    desktop = env.value("XDG_DATA_DIRS");
    for (QString &wm : tilers)
        if (desktop.contains(wm))
            return true;
    for (QString &wm: tilers) {
        QProcess process;
        process.start("pgrep", QStringList({wm}));
        process.waitForFinished();
        if (!process.readAllStandardOutput().isEmpty())
            return true;
    }
#endif
    return false;
}

void Platform::disableAutomaticAccel(QWidget *what)
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    static auto symbol = "_ZN19KAcceleratorManager10setNoAccelEP7QWidget";

    void *d = dlopen("libKF5WidgetsAddons.so", RTLD_LAZY);
    if (!d)
        return;
    typedef void (*DisablerFunc)(QWidget *);
    DisablerFunc setNoAccel;
    setNoAccel = reinterpret_cast<DisablerFunc>(dlsym(d, symbol));
    if (setNoAccel)
        setNoAccel(what);
    dlclose(d);
#endif
}
