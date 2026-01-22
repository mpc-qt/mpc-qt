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
#include "gotowindow.h"
#include "thumbnailerwindow.h"
#include "platform/screensaver.h"
#include "platform/windowmanager.h"

class MprisInstance;
class QLockFile;
class QThread;

void signalHandler(int signal);

// a simple class to control program execution and own application objects
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
    static void earlyPlatformOverride();
    static void setTranslation(QTranslator *qtTranslator, QTranslator *appTranslator);
    static bool isNvidiaGPU();

signals:
    void windowsRestored();
    void flushLog();

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
    void updateRecentPosition(bool resetPosition);
    void updateRecents(QUrl url, QUuid listUuid, QUuid itemUuid, QString title, double length,
                       double position, int64_t videoTrack, int64_t audioTrack, int64_t subtitleTrack);
    QByteArray makePayload() const;
    void showVersionInfo();
    QString pictureTemplate(Helpers::DisabledTrack tracks, Helpers::Subtitles subs) const;
    QVariantList recentToVList() const;
    QVariantMap favoritesToVMap() const;
    QVariantMap windowsToVMap_v2();
    void restoreWindows_v2(const QVariantMap &geometryMap);

private slots:
    void self_windowsRestored();
    void loadPlaylistsBackup();
    void mainwindow_instanceShouldQuit();
    void mainwindow_fullscreenHideControls(bool hide);
    void mainwindow_repeatAfter();
    void mainwindow_recentOpened(const TrackInfo &track);
    void mainwindow_recentClear();
    void mainwindow_takeImage(Helpers::ScreenshotRender render);
    void mainwindow_takeImageAutomatically(Helpers::ScreenshotRender render);
    void mainwindow_takeThumbnails();
    void mainwindow_optionsOpenRequested();
    void manager_playLengthChanged();
    void manager_stateChanged(PlaybackManager::PlaybackState state);
    void manager_fileClosed();
    void manager_subtitlesVisible(bool visible);
    void manager_hasNoSubtitles(bool none);
    void manager_playingNextFile();
    void manager_openingNewFile();
    void manager_aboutToStartPlayingFile();
    void manager_startedPlayingFile(QUrl url);
    void manager_stoppedPlaying();
    void mpcHcServer_fileSelected(QString fileName);
    void settingswindow_settingsData(const QVariantMap &settings);
    void settingswindow_inhibitScreensaver(bool yes);
    void settingswindow_rememberHistory(bool yes, bool onlyVideos);
    void settingswindow_rememberFilePosition(bool yes);
    void settingswindow_rememberQuickPlaylist(bool yes);
    void settingswindow_rememberWindowGeometry(bool yes);
    void settingswindow_rememberPanels(bool yes);
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
    void favoriteswindow_favoriteTracksCancel();

    void endProgram();
    void importPlaylist(QString fname);
    void exportPlaylist(QString fname, QStringList items);

private:
    std::unique_ptr<QLockFile> lockFile_;
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
    GoToWindow *gotoWindow = nullptr;
    LogWindow *logWindow = nullptr;
    LibraryWindow *libraryWindow = nullptr;
    ThumbnailerWindow *thumbnailerWindow = nullptr;
    QThread *logThread = nullptr;
    WindowManager windowManager;
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

    static bool isWayland;
    static bool isWaylandMode;
    bool firstFile = true;
    bool playlistsBackupLoaded = false;
    bool inhibitScreensaver = false;
    bool manipulateScreensaver = false;
    bool rememberHistory = true;
    bool rememberHistoryOnlyForVideos = true;
    bool rememberFilePosition = false;
    bool rememberQuickPlaylist = true;
    bool rememberWindowGeometry = false;
    bool rememberPanels = false;
    bool nowPlayingDisplayingSubtitles = true;
    bool nowPlayingNoSubtitleTracks = false;
    bool repeatAfter = false;
    QString screenshotDirectory;
    QString lastScreenshotDir;
    QString encodeDirectory;
    QString screenshotTemplate;
    QString encodeTemplate;
    QString screenshotFormat;
};

#endif // MAIN_H
