#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include <QList>
#include <QObject>
#include <QVariantMap>
#include <QWindow>

class MainWindow;
class QMainWindow;
class QDockWidget;

struct CliInfo {
    QPoint cliPos;
    QSize cliSize;
    bool validCliPos;
    bool validCliSize;
};

struct FullscreenMemory {
    QString originalScreen;
    QRect normalGeometry;
    bool maximized;
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

    void saveAppWindow(MainWindow *window, bool rememberWindowGeometry, bool rememberPanels);
    void saveDocks(QMainWindow *dockHost);
    void saveWindow(QWidget *window);

    void restoreAppWindow(MainWindow *window, const CliInfo &cliInfo);
    void restoreDocks(QMainWindow *dockHost, QList<QDockWidget*> dockWidgets);
    void restoreWindow(QWidget *window);

    static FullscreenMemory makeFullscreen(QWidget *who, QString preferredScreen);
    static void restoreFullscreen(QWidget *who, const FullscreenMemory &fm);

signals:

private:
    QVariantMap json_;
};

#endif // WINDOWMANAGER_H
