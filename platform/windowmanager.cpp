#include <QStyle>
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

static void applyGeometryVariant(QWidget *widget, const QVariant &v) {

}

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

void WindowManager::saveDocks(QMainWindow *dockHost)
{
    QVariantMap data = {
        { keyQtState, QString(dockHost->saveState().toBase64()) }
    };
    json_.insert(dockHost->objectName(), data);
}

void WindowManager::saveWindow(QWidget *window)
{
    QVariantMap data = {
        { keyGeometry, Helpers::rectToVmap(window->geometry()) }
    };
    json_.insert(window->objectName(), data);
}

void WindowManager::restoreAppWindow(MainWindow *window, const CliInfo &cliInfo)
{
    // Unlike restoreWindow, we do not bail out -- this function MUST succeed
    // in some way for the window to show
    QVariantMap data = json_[window->objectName()].toMap();

    // restore main window geometry and override it if requested
    QRect geometry = Helpers::vmapToRect(data[keyGeometry].toMap());
    QPoint desiredPlace = geometry.topLeft();
    QSize desiredSize = geometry.size();
    bool checkMainWindow = data.isEmpty() || geometry.isEmpty();

    if (checkMainWindow)
        desiredSize = window->desirableSize(true);
    if (cliInfo.validCliSize)
        desiredSize = cliInfo.cliSize;

    if (checkMainWindow)
        desiredPlace = window->desirablePosition(desiredSize, true);
    if (cliInfo.validCliPos)
        desiredPlace = cliInfo.cliPos;

    window->setGeometry(QRect(desiredPlace, desiredSize));

    if (data.value(keyMinimized, false).toBool()) {
        window->showMinimized();
    } else {
        if (data.value(keyMaximized, false).toBool())
            window->showMaximized();
        else
            window->show();
        window->raise();
    }
    if (data.contains(keyState))
        window->setState(data[keyState].toMap());
}

void WindowManager::restoreDocks(QMainWindow *dockHost, QList<QDockWidget *> dockWidgets)
{
    if (!json_.contains(dockHost->objectName()))
        return;
    QVariantMap data = json_[dockHost->objectName()].toMap();
    if (!data.contains(keyQtState))
        return;
    QByteArray encoded = data[keyQtState].toString().toLocal8Bit();
    dockHost->restoreState(QByteArray::fromBase64(encoded));
    for (auto widget : dockWidgets)
        dockHost->restoreDockWidget(widget);
}

void WindowManager::restoreWindow(QWidget *window)
{
    if (!json_.contains(window->objectName()))
        return;
    QVariantMap data = json_[window->objectName()].toMap();
    if (!data.contains(keyGeometry))
        return;

    QRect geometry = Helpers::vmapToRect(data[keyGeometry].toMap());
    if (geometry.isEmpty()) {
        QRect available = Helpers::availableGeometryFromPoint(QCursor::pos());
        geometry = QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, window->size(), available);
    }
    window->setGeometry(geometry);
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
