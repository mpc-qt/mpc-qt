#include <QObject>
#include <QProcess>
#include <QApplication>
#include "logger.h"
#include "unify.h"

// Platform includes
#if defined(Q_OS_WIN)
#include "devicemanager_win.h"
#include "screensaver_win.h"
#else
#include "devicemanager_unix.h"
#include "screensaver_unix.h"
#endif


// Platform defines
#if defined(Q_OS_WIN)
    #define ScreenSaverPlatform     ScreenSaverWin
    #define DeviceManagerPlatform   DeviceManagerWin
#else
    #define ScreenSaverPlatform     ScreenSaverUnix
    #define DeviceManagerPlatform   DeviceManagerUnix
#endif


// Platform flags
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


DeviceManager *Platform::deviceManager()
{
    static DeviceManagerPlatform *dm = nullptr;
    if (dm == nullptr) {
        Q_ASSERT(qApp);
        dm = new DeviceManagerPlatform(qApp);
        QObject::connect(dm, &DeviceManager::destroyed, [&] {
            dm = nullptr;
        });
    }
    return dm;
}

ScreenSaver *Platform::screenSaver()
{
    static ScreenSaverPlatform *ss = nullptr;
    if (ss == nullptr) {
        Q_ASSERT(qApp);
        ss = new ScreenSaverPlatform(qApp);
        QObject::connect(ss, &ScreenSaver::destroyed, [&] {
            ss = nullptr;
        });
    }
    return ss;
}

QString Platform::resourcesPath()
{
    if (Platform::isWindows)
        return QApplication::applicationDirPath();
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
        for (auto f : {':', '<', '>', '"', '/', '\\', '|', '?', '*'})
            fileName.replace(f, '.');
    else
        fileName.replace('/', '.');
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

    // These lists are not exhaustive
    QStringList tilers({ "awesome", "bspwm", "dwm", "i3", "larswm", "ion",
        "qtile", "ratpoison", "stumpwm", "wmii", "xmonad"});
    QStringList stackers({"plasma", "kde", "kwin", "gnome", "mutter", "xfce",
                          "mint", "mate", "cinnamon", "lxde", "lxqt",
                          "openbox"});

    QString desktop = qEnvironmentVariable("XDG_SESSION_DESKTOP").toLower();
    if (tilers.contains(desktop))
        return true;
    if (stackers.contains(desktop))
        return false;

    desktop = qEnvironmentVariable("XDG_DATA_DIRS").toLower();
    for (QString &wm : tilers)
        if (desktop.contains(wm))
            return true;
    for (QString &wm : stackers)
        if (desktop.contains(wm))
            return false;

    // Last resort, this will take some time (0.3s on my machine)
    Logger::log("platform", "did not quickly determine desktop type, "
                            "using pgrep and making a guess");
    for (QString &wm: tilers) {
        QProcess process;
        process.start("pgrep", QStringList({"-x", wm}));
        process.waitForFinished();
        if (!process.readAllStandardOutput().isEmpty())
            return true;
    }
    return false;
}
