#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include <QObject>
#include <QVariantMap>
#include <QWindow>
#include "mainwindow.h"

// TODO: investigate session management api save/restore
class WindowManager : public QObject
{
    Q_OBJECT
public:
    explicit WindowManager(QObject *parent = nullptr);

    void setJson(const QVariantMap &json);
    QVariantMap json();

    void saveAppWindow(MainWindow *window);
    void saveDockHost(QMainWindow *dockWindow);
    void saveDockWindow(QDockWidget *window);
    void saveWindow(QWidget *window);

    void restoreAppWindow(MainWindow *window);
    void restoreDockHost(QMainWindow *dockHost);
    void restoreDockWindow(QDockWidget *window);
    void restoreWindow(QWidget *window);

    QSize calculateParentSize(QWidget *parent, QWidget *child, const QSize &childSize);
    void centerWindowAndClip(QWidget *who, const QSize &newSize);
    QVariantMap makeFullscreen(QWidget *who, QString preferredScreen);
    void restoreFullscreen(QWidget *who, const QVariantMap &data);

signals:

private:
    QVariantMap json_;
};

#endif // WINDOWMANAGER_H
