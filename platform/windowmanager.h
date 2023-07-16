#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include <QList>
#include <QObject>
#include <QVariantMap>
#include <QWindow>
#include "mainwindow.h"

struct CliInfo {
    QPoint cliPos;
    QSize cliSize;
    bool validCliPos;
    bool validCliSize;
};

// TODO: investigate session management api save/restore
class WindowManager : public QObject
{
    Q_OBJECT
public:
    explicit WindowManager(QObject *parent = nullptr);

    void clearJson();
    void setJson(const QVariantMap &json);
    QVariantMap json();

    void saveAppWindow(MainWindow *window);
    void saveDocks(QMainWindow *dockHost);
    void saveWindow(QWidget *window);

    void restoreAppWindow(MainWindow *window, const CliInfo &cliInfo);
    void restoreDocks(QMainWindow *dockHost, QList<QDockWidget*> dockWidgets);
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
