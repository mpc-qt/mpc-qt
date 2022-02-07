#ifndef MAIN_H
#define MAIN_H
#include <QHash>
#include <QMetaMethod>
#include "ipc/http.h"
#include "ipc/json.h"
#include "helpers.h"
#include "librarywindow.h"
#include "logwindow.h"
#include "mainwindow.h"
#include "manager.h"
#include "storage.h"
#include "settingswindow.h"
#include "propertieswindow.h"
#include "favoriteswindow.h"
#include "thumbnailerwindow.h"
#include "platform/screensaver.h"
#include "platform/devicemanager.h"

class MprisInstance;
class QThread;

// a simple class to control program exection and own application objects
class Flow : public QObject {
    Q_OBJECT
    enum ProgramMode { UnknownMode, EarlyQuitMode, PrimaryMode, FreestandingMode, };

public:
    explicit Flow(QObject *owner = nullptr);
    ~Flow();

    void parseArgs();
    void detectMode();
    void init();
    int run();
    bool earlyQuit();

signals:
    void recentFilesChanged(QList<TrackInfo> urls);
    void windowsRestored();

private:
    void readConfig();
    void writeConfig(bool onlySettings = false);
    void setupMainWindowConnections();
    void setupManagerConnections();
    void setupSettingsConnections();
    void setupMpvObjectConnections();
    void setupFlowConnections();
    void setupMpris();
    void setupMpcHc();
    QByteArray makePayload() const;
    QString pictureTemplate(Helpers::DisabledTrack tracks, Helpers::Subtitles subs) const;
    QVariantList recentToVList() const;
    QVariantMap favoritesToVMap() const;
    QVariantMap windowsToVMap();
    void restoreWindows(const QVariantMap &geometryMap);
    void showWindows(const QVariantMap &mainWindowMap);

private slots:
    void self_windowsRestored();
    void mainwindow_instanceShouldQuit();
    void mainwindow_recentOpened(const TrackInfo &track);
    void mainwindow_recentClear();
    void mainwindow_takeImage(Helpers::ScreenshotRender render);
    void mainwindow_takeImageAutomatically(Helpers::ScreenshotRender render);
    void mainwindow_takeThumbnails();
    void mainwindow_optionsOpenRequested();
    void manager_nowPlayingChanged(QUrl url, QUuid listUuid, QUuid itemUuid);
    void manager_stateChanged(PlaybackManager::PlaybackState state);
    void manager_subtitlesVisibile(bool visible);
    void manager_hasNoSubtitles(bool none);
    void mpcHcServer_fileSelected(QString fileName);
    void settingswindow_settingsData(const QVariantMap &settings);
    void settingswindow_inhibitScreensaver(bool yes);
    void settingswindow_rememberWindowGeometry(bool yes);
    void settingswindow_keymapData(const QVariantMap &keyMap);
    void settingswindow_mprisIpc(bool enabled);
    void settingswindow_stylesheetIsFusion(bool yes);
    void settingswindow_stylesheetText(QString text);
    void settingswindow_screenshotDirectory(const QString &where);
    void settingswindow_encodeDirectory(const QString &where);
    void settingswindow_screenshotTemplate(const QString &fmt);
    void settingswindow_encodeTemplate(const QString &fmt);
    void settingswindow_screenshotFormat(const QString &fmt);
    void favoriteswindow_favoriteTracks(const QList<TrackInfo> &files, const QList<TrackInfo> &streams);

    void endProgram();
    void importPlaylist(QString fname);
    void exportPlaylist(QString fname, QStringList items);

private:
    MpcQtServer *server = nullptr;
    MpvServer *mpvServer = nullptr;
    MpcHcServer *mpcHcServer = nullptr;
    MprisInstance *mpris = nullptr;
    ScreenSaver *screenSaver = nullptr;
    MainWindow *mainWindow = nullptr;
    PlaybackManager *playbackManager = nullptr;
    SettingsWindow *settingsWindow = nullptr;
    PropertiesWindow *propertiesWindow = nullptr;
    FavoritesWindow *favoritesWindow = nullptr;
    LogWindow *logWindow = nullptr;
    LibraryWindow *libraryWindow = nullptr;
    ThumbnailerWindow *thumbnailerWindow = nullptr;
    QThread *logThread = nullptr;
    Storage storage;
    QVariantMap settings;
    QVariantMap keyMap;
    QList<TrackInfo> recentFiles;
    QList<TrackInfo> favoriteFiles;
    QList<TrackInfo> favoriteStreams;

    ProgramMode programMode = UnknownMode;
    bool cliNoConfig = false;
    bool cliNoFiles = false;
    QSize cliSize;
    QPoint cliPos;
    bool validCliSize = false;
    bool validCliPos = false;
    QStringList customFiles;

    bool inhibitScreensaver = false;
    bool manipulateScreensaver = false;
    bool rememberWindowGeometry = false;
    bool nowPlayingDisplayingSubtitles = true;
    bool nowPlayingNoSubtitleTracks = false;
    QString screenshotDirectory;
    QString encodeDirectory;
    QString screenshotTemplate;
    QString encodeTemplate;
    QString screenshotFormat;
};

#endif // MAIN_H
