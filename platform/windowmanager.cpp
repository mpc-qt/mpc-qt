#include <QWidget>
#include "helpers.h"
#include "windowmanager.h"

static const char keyCommand[] = "command";
static const char keyDirectory[] = "directory";
static const char keyFiles[] = "files";
static const char keyFloating[] = "floating";
static const char keyGeometry[] = "geometry";
static const char keyLibraryWindow[] = "libraryWindow";
static const char keyLogWindow[] = "logWindow";
static const char keyMainWindow[] = "mainWindow";
static const char keyMaximized[] = "maximized";
static const char keyMinimized[] = "minimized";
static const char keyMpvHost[] = "mpvHost";
static const char keyPlaylistWindow[] = "playlistWindow";
static const char keyPropertiesWindow[] = "propertiesWindow";
static const char keyQtState[] = "qtState";
static const char keySettingsWindow[] = "settingsWindow";
static const char keyState[] = "state";
static const char keyStreams[] = "streams";

WindowManager::WindowManager(QObject *parent)
    : QObject{parent}
{

}

void WindowManager::setJson(const QVariantMap &json)
{
    json_ = json;
}

QVariantMap WindowManager::json()
{
    return json_;
}

void WindowManager::saveAppWindow(MainWindow *window)
{
    QVariantMap data = {
        { keyGeometry, Helpers::rectToVmap(QRect(window->geometry().topLeft(),
                                            window->size())) },
        { keyState, window->state() },
        { keyMaximized, window->isMaximized() },
        { keyMinimized, window->isMinimized() }
    };
    json_.insert(window->objectName(), data);
}

void WindowManager::saveDockHost(QMainWindow *dockWindow)
{
    QVariantMap data = {
        { keyQtState, QString(dockWindow->saveState().toBase64()) }
    };
    json_.insert(dockWindow->objectName(), data);
}

void WindowManager::saveDockWindow(QDockWidget *window)
{
    QVariantMap data = {
        { keyGeometry, Helpers::rectToVmap(window->window()->geometry()) },
        { keyFloating, window->isFloating() }
    };
    json_.insert(window->objectName(), data);
}

void WindowManager::saveWindow(QWidget *window)
{
    QVariantMap data = {
        { keyGeometry, Helpers::rectToVmap(window->geometry()) }
    };
    json_.insert(window->objectName(), data);
}

void WindowManager::restoreAppWindow(MainWindow *window)
{
    if (!json_.contains(window->objectName()))
        return;
}


void WindowManager::restoreDockHost(QMainWindow *dockHost)
{
    if (!json_.contains(dockHost->objectName()))
        return;

}

void WindowManager::restoreDockWindow(QDockWidget *window)
{
    if (!json_.contains(window->objectName()))
        return;

}

void WindowManager::restoreWindow(QWidget *window)
{
    if (!json_.contains(window->objectName()))
        return;

}

QSize WindowManager::calculateParentSize(QWidget *parent, QWidget *child, const QSize &childSize)
{
    //FIXME
    return QSize();
}

void WindowManager::centerWindowAndClip(QWidget *who, const QSize &newSize)
{
    //FIXME
}

QVariantMap WindowManager::makeFullscreen(QWidget *who, QString preferredScreen)
{
    //FIXME
    return QVariantMap();
}

void WindowManager::restoreFullscreen(QWidget *who, const QVariantMap &data)
{

}
