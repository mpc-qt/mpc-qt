#include <QObject>
#include <QProcess>
#include <QApplication>
#include <QDir>
#include "unify.h"

// Platform includes
#if defined(Q_OS_WIN)
#include "qscreensaver_win.h"
#elif defined(Q_OS_MAC)
#include "qscreensaver_mac.h"
#else
#include <dlfcn.h>
#include "qscreensaver_unix.h"
#endif


// Platform defines
#if defined(Q_OS_WIN)
    #define QScreenSaverPlatform QScreenSaverWin
#elif defined(Q_OS_MAC)
    #define QScreenSaverPlatform QScreenSaverMac
#else
    #define QScreenSaverPlatform QScreenSaverUnix
#endif


// Platform flags
const bool Platform::isMac =
#if defined(Q_OS_MAC)
    true
#else
    false
#endif
;

const bool Platform::isWindows =
#if defined(Q_OS_WIN)
    true
#else
    false
#endif
;

const bool Platform::isUnix =
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    true
#else
    false
#endif
;


QScreenSaver *Platform::screenSaver()
{
    static QScreenSaverPlatform *ss = nullptr;
    if (ss == nullptr) {
        Q_ASSERT(qApp);
        ss = new QScreenSaverPlatform(qApp);
        QObject::connect(ss, &QScreenSaver::destroyed, [&] {
            ss = nullptr;
        });
    }
    return ss;
}

QString Platform::resourcesPath()
{
    if (Platform::isMac)
        return QApplication::applicationDirPath() + "/../Resources";
    if (Platform::isWindows)
        return QApplication::applicationDirPath();
    if (Platform::isUnix) {
#ifdef MPCQT_PREFIX
        return MPCQT_PREFIX "/share/mpc-qt";
#endif
    }
    return ".";
}

QString Platform::fixedConfigPath(QString configPath)
{
    if (isWindows) {
        // backward compatability sucks
        configPath.replace("AppData/Local","AppData/Roaming");
    }
    return configPath;
}

QString Platform::sanitizedFilename(QString fileName)
{
    if (isWindows)
        fileName.replace(':', '.');
    return fileName;
}

bool Platform::tiledDesktopsExist()
{
    return isUnix;
}

bool Platform::tilingDesktopActive()
{
    if (!isUnix)
        return false;

    QProcessEnvironment env;
    QStringList tilers({ "awesome", "bspwm", "dwm", "i3", "larswm", "ion",
        "qtile", "ratpoison", "stumpwm", "wmii", "xmonad"});
    QString desktop = env.value("XDG_SESSION_DESKTOP", "=");
    if (tilers.contains(desktop))
        return true;
    desktop = env.value("XDG_DATA_DIRS", "=");
    for (QString &wm : tilers)
        if (desktop.contains(wm))
            return true;
    for (QString &wm: tilers) {
        QProcess process;
        process.start("pgrep", QStringList({"-x", wm}));
        process.waitForFinished();
        if (!process.readAllStandardOutput().isEmpty())
            return true;
    }
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
#else
    Q_UNUSED(what);
#endif
}
