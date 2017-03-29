#ifndef MAIN_H
#define MAIN_H
#include <QHash>
#include <QMetaMethod>
#include "ipc.h"
#include "helpers.h"
#include "mainwindow.h"
#include "manager.h"
#include "storage.h"
#include "settingswindow.h"
#include "qscreensaver.h"

// a simple class to control program exection and own application objects
class Flow : public QObject {
    Q_OBJECT
public:
    explicit Flow(QObject *owner = 0);
    ~Flow();

    void parseArgs();
    void init();
    int run();
    bool hasPrevious();

signals:
    void recentFilesChanged(QList<TrackInfo> urls);
    void windowsRestored();

private:
    QByteArray makePayload() const;
    QString pictureTemplate(Helpers::DisabledTrack tracks, Helpers::Subtitles subs) const;
    QVariantList recentToVList() const;
    void recentFromVList(const QVariantList &list);
    QVariantMap saveWindows();
    void restoreWindows(const QVariantMap &map);
    void showWindows(const QVariantMap &mainWindowMap);

private slots:
    void self_windowsRestored();
    void mainwindow_applicationShouldQuit();
    void mainwindow_recentOpened(const TrackInfo &track);
    void mainwindow_recentClear();
    void mainwindow_takeImage(bool subs);
    void mainwindow_takeImageAutomatically(bool subs);
    void mainwindow_optionsOpenRequested();
    void manager_nowPlayingChanged(QUrl url, QUuid listUuid, QUuid itemUuid);
    void manager_stateChanged(PlaybackManager::PlaybackState state);
    void settingswindow_settingsData(const QVariantMap &settings);
    void settingswindow_inhibitScreensaver(bool yes);
    void settingswindow_rememberWindowGeometry(bool yes);
    void settingswindow_keymapData(const QVariantMap &keyMap);
    void settingswindow_screenshotDirectory(const QString &where);
    void settingswindow_encodeDirectory(const QString &where);
    void settingswindow_screenshotTemplate(const QString &fmt);
    void settingswindow_encodeTemplate(const QString &fmt);
    void settingswindow_screenshotFormat(const QString &fmt);
    void importPlaylist(QString fname);
    void exportPlaylist(QString fname, QStringList items);

private:
    MpcQtServer *server;
    MpvServer *mpvServer;
    MainWindow *mainWindow;
    PlaybackManager *playbackManager;
    SettingsWindow *settingsWindow;
    Storage storage;
    QVariantMap settings;
    QVariantMap keyMap;
    QList<TrackInfo> recentFiles;
    QScreenSaver screenSaver;

    QSize customSize;
    QPoint customPos;
    bool validCustomSize;
    bool validCustomPos;
    QStringList customFiles;

    bool inhibitScreensaver;
    bool manipulateScreensaver;
    bool rememberWindowGeometry;
    QString screenshotDirectory;
    QString encodeDirectory;
    QString screenshotTemplate;
    QString encodeTemplate;
    QString screenshotFormat;
    bool hasPrevious_;
};

#endif // MAIN_H
