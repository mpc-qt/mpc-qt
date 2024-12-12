#include <QDockWidget>
#include <QMainWindow>
#include <QMetaMethod>
#include <QStyle>
#include <QWidget>
#include "helpers.h"
#include "mainwindow.h"
#include "windowmanager.h"

constexpr char keyGeometry[] = "geometry";
constexpr char keyMaximized[] = "maximized";
constexpr char keyQtState[] = "qtState";
constexpr char keyState[] = "state";

WindowManager::WindowManager(QObject *parent)
    : QObject{parent}
{

}

void WindowManager::clearJson()
{
    json_.clear();
}

void WindowManager::setJson(const QVariantMap &json)
{
    json_ = json;
}

QVariantMap WindowManager::json()
{
    return json_;
}

void WindowManager::saveAppWindow(MainWindow *window, bool rememberWindowGeometry, bool rememberPanels)
{
    QVariantMap geometry;
    QVariantMap panels;
    if (rememberWindowGeometry)
        geometry = Helpers::rectToVmap(QRect(window->geometry().topLeft(),
                                            window->size()));
    if (rememberPanels)
        panels = window->state();
    QVariantMap data = {
        { keyGeometry, geometry },
        { keyState, panels },
        { keyMaximized, window->isMaximized() }
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

    if (data.value(keyMaximized, false).toBool())
        window->showMaximized();
    else
        window->show();
    window->raise();
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
    for (auto widget : dockWidgets) {
        dockHost->restoreDockWidget(widget);
        const QMetaObject *dockMetaObject = widget->metaObject();
        int index = dockMetaObject->indexOfSlot("dockLocationMaybeChanged(void)");
        if (index == -1)
            continue;
        dockMetaObject->method(index).invoke(widget);
    }
}

void WindowManager::restoreWindow(QWidget *window)
{
    QVariantMap data = json_[window->objectName()].toMap();
    QRect geometry = Helpers::vmapToRect(data[keyGeometry].toMap());
    if (geometry.isEmpty()) {
        QRect available = Helpers::availableGeometryFromPoint(QCursor::pos());
        geometry = QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, window->size(), available);
    }
    window->setGeometry(geometry);
}

FullscreenMemory WindowManager::makeFullscreen(QWidget *who, QString preferredScreen)
{
    FullscreenMemory fm;
    fm.maximized = who->isMaximized();
    fm.originalScreen = who->screen()->name();

    // Doesn't work unless we start off as a normal window
    if (fm.maximized)
        who->showNormal();

    fm.normalGeometry = who->geometry();

    QScreen *target = Helpers::findScreenByName(preferredScreen);
    if (target) {
        who->setScreen(target);
        who->setGeometry(target->geometry());
    }
    who->showFullScreen();
    return fm;
}

void WindowManager::restoreFullscreen(QWidget *who, const FullscreenMemory &fm)
{
    who->showNormal();
    who->setGeometry(fm.normalGeometry);

    QScreen *target = Helpers::findScreenByName(fm.originalScreen);
    if (target) {
        who->setScreen(target);
    }
    if (fm.maximized)
        who->showMaximized();
}
