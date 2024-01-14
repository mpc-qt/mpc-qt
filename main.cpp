#include <clocale>
#include <QApplication>
#include <QLocalSocket>
#include <QFileDialog>
#include <QDir>
#include <QStandardPaths>
#include <QUuid>
#include <QJsonDocument>
#include <QTimer>
#include <QCommandLineParser>
#include <QSurfaceFormat>
#include <QStyleFactory>
#include <QThread>
#include <QTranslator>
#include <QLibraryInfo>
#include "logger.h"
#include "main.h"
#include "storage.h"
#include "mainwindow.h"
#include "manager.h"
#include "settingswindow.h"
#include "mpvwidget.h"
#include "propertieswindow.h"
#include "ipc/mpris.h"
#include "platform/devicemanager.h"
#include "platform/unify.h"
#include "playlist.h"

//---------------------------------------------------------------------------

static const char keyCommand[] = "command";
static const char keyDirectory[] = "directory";
static const char keyFiles[] = "files";
static const char keyStreams[] = "streams";

static const char fileFavorites[] = "favorites";
static const char fileGeometryV2[] = "geometry_v2";
static const char fileKeys[] = "keys";
static const char filePlaylists[] = "playlists";
static const char filePlaylistsBackup[] = "playlists_backup";
static const char fileRecent[] = "recent";
static const char fileSettings[] = "settings";

//---------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    Flow::earlyPlatformOverride();

    QApplication a(argc, argv);
    Logger::singleton();
    a.setWindowIcon(QIcon(":/images/icon/mpc-qt.svg"));

    // The wayland plugin as of writing this (c. 2018-04) defaults
    // to a 16bit color surface, so ask for the standard 32bit one.
    if (a.platformName().contains("wayland")) {
        QSurfaceFormat sf(QSurfaceFormat::defaultFormat());
        sf.setBlueBufferSize(8);
        sf.setGreenBufferSize(8);
        sf.setRedBufferSize(8);
        sf.setAlphaBufferSize(8);
        QSurfaceFormat::setDefaultFormat(sf);
    }

    // Qt sets the locale in the QApplication constructor, but libmpv requires
    // the LC_NUMERIC category to be set to "C", so change it back.
    std::setlocale(LC_NUMERIC, "C");

    // Register the error code type et al so that events can serialize them.
    qRegisterMetaType<MpvController::PropertyList>("MpvController::PropertyList");
    qRegisterMetaType<MpvController::OptionList>("MpvController::OptionList");
    qRegisterMetaType<MpvErrorCode>("MpvErrorCode");
    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<uint16_t>("uint16_t");

    // Register the translations
    QLocale locale;
    QTranslator qtTranslator;
    if (qtTranslator.load(locale, "qt", "_", ":/i18n"))
        a.installTranslator(&qtTranslator);

    QTranslator aTranslator;
    if (aTranslator.load(locale, "mpc-qt", "_", ":/i18n"))
        a.installTranslator(&aTranslator);

#ifndef MPCQT_VERSION_STR
#define MPCQT_VERSION_STR MainWindow::tr("Development Build")
#endif
    QCoreApplication::setApplicationVersion(MPCQT_VERSION_STR);

    // Spin up the application
    Flow f;
    f.parseArgs();
    f.detectMode();
    if (f.earlyQuit())
        return 0;
    f.init();
    return f.run();
}

//---------------------------------------------------------------------------

bool Flow::settingsDisableWindowManagement = false;

Flow::Flow(QObject *owner) :
    QObject(owner)
{
    readConfig();
}

Flow::~Flow()
{
    if (server) {
        delete server;
        server = nullptr;
    }
    if (mpvServer) {
        delete mpvServer;
        mpvServer = nullptr;
    }
    if (mpcHcServer) {
        delete mpcHcServer;
        mpcHcServer = nullptr;
    }
    if (mainWindow) {
        // only write out the playlist if we're operating in the default mode
        // as the sole application.  Freestanding applications don't write any
        // settings, but they do inherit them.
        if (programMode == PrimaryMode) {
            storage.writeVList(filePlaylists, mainWindow->playlistWindow()->tabsToVList());
            storage.writeVList(filePlaylistsBackup, PlaylistCollection::getBackup()->toVList());
        }
        delete mainWindow;
        mainWindow = nullptr;
    }
    if (playbackManager) {
        delete playbackManager;
        playbackManager = nullptr;
    }
    if (settingsWindow) {
        delete settingsWindow;
        settingsWindow = nullptr;
    }
    if (propertiesWindow)  {
        delete propertiesWindow;
        propertiesWindow = nullptr;
    }
    if (favoritesWindow) {
        delete favoritesWindow;
        favoritesWindow = nullptr;
    }
    if (screenSaver) {
        screenSaver->uninhibitSaver();
        delete screenSaver;
        screenSaver = nullptr;
    }
    if (thumbnailerWindow) {
        delete thumbnailerWindow;
        thumbnailerWindow = nullptr;
    }
    if (logWindow) {
        delete logWindow;
        logWindow = nullptr;
    }
    if (logThread) {
        logThread->quit();
        logThread->wait();
        delete logThread;
        logThread = nullptr;
    }
    if (libraryWindow) {
        delete libraryWindow;
        libraryWindow = nullptr;
    }
}

void Flow::parseArgs()
{
    QCommandLineParser parser;
    parser.setApplicationDescription(tr("Media Player Classic Qute Theater"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption freestandingOpt("freestanding", tr("Start a new process without saving data."));
    QCommandLineOption noConfigOpt("no-config", tr("Do not load any config files."));
    QCommandLineOption noFilesOpt("no-files", tr("Do not load file history, playlists, or favorites."));
    QCommandLineOption sizeOpt("size", tr("Main window size."), "w,h");
    QCommandLineOption posOpt("pos", tr("Main window position."), "x,y");

    parser.addOption(freestandingOpt);
    parser.addOption(noConfigOpt);
    parser.addOption(noFilesOpt);
    parser.addOption(sizeOpt);
    parser.addOption(posOpt);
    parser.addPositionalArgument("urls", tr("URLs to open, optionally."), "[urls...]");

    parser.process(QCoreApplication::arguments());

    programMode = parser.isSet(freestandingOpt) ? FreestandingMode : UnknownMode;
    cliNoConfig = parser.isSet(noConfigOpt);
    cliNoFiles = parser.isSet(noFilesOpt);
    validCliSize = parser.isSet(sizeOpt) && Helpers::sizeFromString(cliSize, parser.value(sizeOpt));
    validCliPos = parser.isSet(posOpt) && Helpers::pointFromString(cliPos, parser.value(posOpt));
    customFiles = parser.positionalArguments();
}

void Flow::detectMode() {
    if (programMode != UnknownMode)
        return;

    bool multiwinMode = settings.value("playerOpenNew", QVariant(false)).toBool();
    if (multiwinMode) {
        // In multiwin mode, we want to take over the main instance if it has quit,
        // so we switch to freestanding mode only if we find a previous instance.
        programMode = MpcQtServer::sendIdentify() ? FreestandingMode : PrimaryMode;
        return;
    }

    // Attempt to send our urls to a previous instance, and bail out if it works.
    bool alreadyAServer = JsonServer::sendPayload(makePayload(), MpcQtServer::defaultSocketName());
    programMode = alreadyAServer ? EarlyQuitMode : PrimaryMode;
}

void Flow::init() {
    Q_ASSERT(programMode != UnknownMode);

    // Start logging early
    logThread = new QThread();
    logThread->start();
    Logger *logger = Logger::singleton();
    logger->moveToThread(logThread);
    connect(logThread, &QThread::finished,
            logger, &QObject::deleteLater);

    // Create our windows
    mainWindow = new MainWindow();
    playbackManager = new PlaybackManager(this);
    playbackManager->setMpvObject(mainWindow->mpvObject(), true);
    playbackManager->setPlaylistWindow(mainWindow->playlistWindow());
    settingsWindow = new SettingsWindow();
    settingsWindow->setWindowModality(Qt::WindowModal);
    if (settingsDisableWindowManagement)
        settingsWindow->disableWindowManagment();
    propertiesWindow = new PropertiesWindow();
    favoritesWindow = new FavoritesWindow();
    logWindow = new LogWindow();
    libraryWindow = new LibraryWindow();
    thumbnailerWindow = new ThumbnailerWindow();

    // Start our servers
    server = new MpcQtServer(mainWindow, playbackManager, this);
    server->setMainWindow(mainWindow);
    server->setPlaybackManger(playbackManager);

    mpvServer = new MpvServer(this);
    mpvServer->setPlaybackManger(playbackManager);
    mpvServer->setMpvObject(mainWindow->mpvObject());

    mpcHcServer = new MpcHcServer(this);

    // Initialize the screensaver
    inhibitScreensaver = false;
    screenSaver = Platform::screenSaver();
    QSet<ScreenSaver::Ability> actualPowers = screenSaver->abilities();
    mainWindow->setScreensaverAbilities(actualPowers);
    QSet<ScreenSaver::Ability> desiredPowers;
    desiredPowers << ScreenSaver::Inhibit << ScreenSaver::Uninhibit;
    manipulateScreensaver = actualPowers.contains(desiredPowers);
    settingsWindow->setScreensaverDisablingEnabled(manipulateScreensaver);

    // Initialize the device manager if present
    if (Platform::deviceManager()->deviceAccessPossible()) {
        Logger::log("main", "device manager active");
    }

    // Connect the modules together, somewhat like a switchboard.
    // A connection method such as A->B is kept with B->A if possible.
    setupMainWindowConnections();
    setupManagerConnections();
    setupSettingsConnections();
    setupMpvObjectConnections();
    setupFlowConnections();

    // Setup more ipc and wire them up if we're in the primary mode
    if (programMode == PrimaryMode) {
        setupMpris();
        setupMpcHc();
    }

    // update player framework
    settingsWindow->takeActions(mainWindow->editableActions());
    mainWindow->setRecentDocuments(recentFiles);
    mainWindow->setFavoriteTracks(favoriteFiles, favoriteStreams);
    favoritesWindow->setFiles(favoriteFiles);
    favoritesWindow->setStreams(favoriteStreams);

    settingsWindow->setAudioDevices(mainWindow->mpvObject()->audioDevices());
    settingsWindow->takeSettings(settings);
    settingsWindow->setMouseMapDefaults(mainWindow->mouseMapDefaults());
    settingsWindow->takeKeyMap(keyMap);
    settingsWindow->sendSignals();
    settingsWindow->sendAcceptedSettings();

    // Turn our servers on in primary mode
    if (programMode == PrimaryMode) {
        server->listen();
        mpvServer->listen();
    }
    // and notify settings window of our server
    settingsWindow->setServerName(server->fullServerName());

    // Turn certain things off in freestanding mode
    mainWindow->setFreestanding(programMode == FreestandingMode);
    settingsWindow->setFreestanding(programMode == FreestandingMode);
}


int Flow::run()
{
    // Load our data
    auto playlist = cliNoFiles ? QVariantList() : storage.readVList(filePlaylists);
    auto backup = cliNoFiles ? QVariantList() : storage.readVList(filePlaylistsBackup);
    auto geometry = cliNoConfig ? QVariantMap() : storage.readVMap(fileGeometryV2);

    // Send data to the ui
    mainWindow->playlistWindow()->tabsFromVList(playlist);
    PlaylistCollection::getBackup()->fromVList(backup);
    libraryWindow->refreshLibrary();

    // Restore our window positions
    restoreWindows_v2(geometry);

    // Wait here until quit
    return qApp->exec();
}

bool Flow::earlyQuit()
{
    return programMode == EarlyQuitMode;
}

void Flow::earlyPlatformOverride()
{
    if (!Platform::isUnix)
        return;
    // Wayland doesn't support run-time centering and it doesn't look like
    // it'll support it any time soon.  I'll remove this code when it does.

    Storage s;
    QVariantMap settings = s.readVMap(fileSettings);
    if (!settings.value("tweaksPreferWayland", QVariant(false)).toBool()) {
        qputenv("QT_QPA_PLATFORM", "xcb");
    } else {
        settingsDisableWindowManagement = true;
    }
}

void Flow::readConfig()
{
    if (!cliNoConfig) {
        settings = storage.readVMap(fileSettings);
        keyMap = storage.readVMap(fileKeys);
    }

    if (!cliNoFiles) {
        QVariantMap favoriteMap = storage.readVMap(fileFavorites);
        favoriteFiles = TrackInfo::tracksFromVList(favoriteMap.value(keyFiles).toList());
        favoriteStreams = TrackInfo::tracksFromVList(favoriteMap.value(keyStreams).toList());
        recentFiles = TrackInfo::tracksFromVList(storage.readVList(fileRecent));
    }
}

// onlySettings - true if all you want to write is the settings, such as if
// you're calling from options dialog.
void Flow::writeConfig(bool onlySettings)
{
    if (programMode != PrimaryMode)
        return;

    storage.writeVMap(fileSettings, settings);
    if (onlySettings)
        return;

    storage.writeVMap(fileKeys, keyMap);
    storage.writeVList(fileRecent, recentToVList());
    storage.writeVMap(fileFavorites, favoritesToVMap());
    storage.writeVMap(fileGeometryV2, windowsToVMap_v2());

    // Note!  Playlists are written in the destructor.
}

void Flow::setupMainWindowConnections()
{
    // mainwindow -> manager
    connect(mainWindow, &MainWindow::severalFilesOpened,
            playbackManager, &PlaybackManager::openSeveralFiles);
    connect(mainWindow, &MainWindow::fileOpened,
            playbackManager, &PlaybackManager::openFile);
    connect(mainWindow, &MainWindow::dvdbdOpened,
            playbackManager, &PlaybackManager::playDiscFiles);
    connect(mainWindow, &MainWindow::streamOpened,
            playbackManager, &PlaybackManager::playStream);
    connect(mainWindow, &MainWindow::subtitlesLoaded,
            playbackManager, &PlaybackManager::loadSubtitle);
    connect(mainWindow, &MainWindow::paused,
            playbackManager, &PlaybackManager::pausePlayer);
    connect(mainWindow, &MainWindow::unpaused,
            playbackManager, &PlaybackManager::unpausePlayer);
    connect(mainWindow, &MainWindow::stopped,
            playbackManager, &PlaybackManager::stopPlayer);
    connect(mainWindow, &MainWindow::stepBackward,
            playbackManager, &PlaybackManager::stepBackward);
    connect(mainWindow, &MainWindow::stepForward,
            playbackManager, &PlaybackManager::stepForward);
    connect(mainWindow, &MainWindow::speedDown,
            playbackManager, &PlaybackManager::speedDown);
    connect(mainWindow, &MainWindow::speedUp,
            playbackManager, &PlaybackManager::speedUp);
    connect(mainWindow, &MainWindow::speedReset,
            playbackManager, &PlaybackManager::speedReset);
    connect(mainWindow, &MainWindow::relativeSeek,
            playbackManager, &PlaybackManager::relativeSeek);
    connect(mainWindow, &MainWindow::audioTrackSelected,
            playbackManager, &PlaybackManager::setAudioTrack);
    connect(mainWindow, &MainWindow::subtitleTrackSelected,
            playbackManager, &PlaybackManager::setSubtitleTrack);
    connect(mainWindow, &MainWindow::videoTrackSelected,
            playbackManager, &PlaybackManager::setVideoTrack);
    connect(mainWindow, &MainWindow::subtitlesEnabled,
            playbackManager, &PlaybackManager::setSubtitleEnabled);
    connect(mainWindow, &MainWindow::nextSubtitleSelected,
            playbackManager, &PlaybackManager::selectNextSubtitle);
    connect(mainWindow, &MainWindow::previousSubtitleSelected,
            playbackManager, &PlaybackManager::selectPrevSubtitle);
    connect(mainWindow, &MainWindow::volumeChanged,
            playbackManager, &PlaybackManager::setVolume);
    connect(mainWindow, &MainWindow::volumeMuteChanged,
            playbackManager, &PlaybackManager::setMute);
    connect(mainWindow, &MainWindow::afterPlaybackOnce,
            playbackManager, &PlaybackManager::setAfterPlaybackOnce);
    connect(mainWindow, &MainWindow::afterPlaybackAlways,
            playbackManager, &PlaybackManager::setAfterPlaybackAlways);
    connect(mainWindow, &MainWindow::chapterPrevious,
            playbackManager, &PlaybackManager::navigateToPrevChapter);
    connect(mainWindow, &MainWindow::chapterNext,
            playbackManager, &PlaybackManager::navigateToNextChapter);
    connect(mainWindow, &MainWindow::chapterSelected,
            playbackManager, &PlaybackManager::navigateToChapter);
    connect(mainWindow, &MainWindow::timeSelected,
            playbackManager, &PlaybackManager::navigateToTime);
    connect(mainWindow, &MainWindow::favoriteCurrentTrack,
            playbackManager, &PlaybackManager::sendCurrentTrackInfo);

    // playlistwindow -> mainwindow
    connect(mainWindow->playlistWindow(), &PlaylistWindow::viewActionChanged,
            mainWindow, &MainWindow::setPlaylistVisibleState);
    connect(mainWindow->playlistWindow(), &PlaylistWindow::quickQueueMode,
            mainWindow, &MainWindow::setPlaylistQuickQueueMode);
    connect(mainWindow->playlistWindow(), &PlaylistWindow::hideFullscreenChanged,
            mainWindow, &MainWindow::setFullscreenHidePanels);

    // mainwindow -> playlistwindow
    connect(mainWindow, &MainWindow::playCurrentItemRequested,
            mainWindow->playlistWindow(), &PlaylistWindow::playCurrentItem);
    connect(mainWindow, &MainWindow::severalFilesOpenedForPlaylist,
            mainWindow->playlistWindow(), &PlaylistWindow::addToPlaylist);

    // manager -> mainwindow
    connect(playbackManager, &PlaybackManager::timeChanged,
            mainWindow, &MainWindow::setTime);
    connect(playbackManager, &PlaybackManager::titleChanged,
            mainWindow, &MainWindow::setMediaTitle);
    connect(playbackManager, &PlaybackManager::chapterTitleChanged,
            mainWindow, &MainWindow::setChapterTitle);
    connect(playbackManager, &PlaybackManager::videoSizeChanged,
            mainWindow, &MainWindow::setVideoSize);
    connect(playbackManager, &PlaybackManager::stateChanged,
            mainWindow, &MainWindow::setPlaybackState);
    connect(playbackManager, &PlaybackManager::typeChanged,
            mainWindow, &MainWindow::setPlaybackType);
    connect(playbackManager, &PlaybackManager::chaptersAvailable,
            mainWindow, &MainWindow::setChapters);
    connect(playbackManager, &PlaybackManager::audioTracksAvailable,
            mainWindow, &MainWindow::setAudioTracks);
    connect(playbackManager, &PlaybackManager::videoTracksAvailable,
            mainWindow, &MainWindow::setVideoTracks);
    connect(playbackManager, &PlaybackManager::subtitleTracksAvailable,
            mainWindow, &MainWindow::setSubtitleTracks);
    connect(playbackManager, &PlaybackManager::fpsChanged,
            mainWindow, &MainWindow::setFps);
    connect(playbackManager, &PlaybackManager::avsyncChanged,
            mainWindow, &MainWindow::setAvsync);
    connect(playbackManager, &PlaybackManager::displayFramedropsChanged,
            mainWindow, &MainWindow::setDisplayFramedrops);
    connect(playbackManager, &PlaybackManager::decoderFramedropsChanged,
            mainWindow, &MainWindow::setDecoderFramedrops);
    connect(playbackManager, &PlaybackManager::audioBitrateChanged,
            mainWindow, &MainWindow::setAudioBitrate);
    connect(playbackManager, &PlaybackManager::videoBitrateChanged,
            mainWindow, &MainWindow::setVideoBitrate);
    connect(playbackManager, &PlaybackManager::afterPlaybackReset,
            mainWindow, &MainWindow::resetPlayAfterOnce);

    // mainwindow -> favorites
    connect(mainWindow, &MainWindow::organizeFavorites,
            favoritesWindow, &FavoritesWindow::show);

    // favorites -> mainwindow
    connect(favoritesWindow, &FavoritesWindow::favoriteTracks,
            mainWindow, &MainWindow::setFavoriteTracks);

    // mainwindow -> properties
    connect(mainWindow, &MainWindow::showFileProperties,
            propertiesWindow, &QWidget::show);

    // mainwindow -> log
    connect(mainWindow, &MainWindow::showLogWindow,
            logWindow, &LogWindow::show);
    connect(mainWindow, &MainWindow::hideLogWindow,
            logWindow, &LogWindow::close);
    connect(logWindow, &LogWindow::windowClosed,
            mainWindow, &MainWindow::logWindowClosed);

    // mainwindow -> library
    connect(mainWindow, &MainWindow::showLibraryWindow,
            libraryWindow, &LibraryWindow::show);
    connect(mainWindow, &MainWindow::hideLibraryWindow,
            libraryWindow, &LibraryWindow::hide);
    connect(libraryWindow, &LibraryWindow::windowClosed,
            mainWindow, &MainWindow::libraryWindowClosed);
    connect(libraryWindow, &LibraryWindow::playlistRestored,
            mainWindow->playlistWindow(), &PlaylistWindow::addPlaylistByUuid);
    connect(mainWindow->playlistWindow(), &PlaylistWindow::playlistMovedToBackup,
            libraryWindow, &LibraryWindow::refreshLibrary);
}

void Flow::setupManagerConnections()
{
    // manager -> favorites
    connect(playbackManager, &PlaybackManager::currentTrackInfo,
            favoritesWindow, &FavoritesWindow::addTrack);

    // manager -> settings
    connect(playbackManager, &PlaybackManager::playerSettingsRequested,
            settingsWindow, &SettingsWindow::sendSignals);
}

void Flow::setupSettingsConnections()
{

    // mainwindow -> settings
    connect(mainWindow, &MainWindow::volumeChanged,
            settingsWindow, &SettingsWindow::setVolume);
    connect(mainWindow, &MainWindow::zoomPresetChanged,
            settingsWindow, &SettingsWindow::setZoomPreset);

    // settings -> mainwindow
    connect(settingsWindow, &SettingsWindow::trayIcon,
            mainWindow, &MainWindow::setTrayIcon);
    connect(settingsWindow, &SettingsWindow::mouseWindowedMap,
            mainWindow, &MainWindow::setWindowedMouseMap);
    connect(settingsWindow, &SettingsWindow::mouseFullscreenMap,
            mainWindow, &MainWindow::setFullscreenMouseMap);
    connect(settingsWindow, &SettingsWindow::iconTheme,
            mainWindow, &MainWindow::setIconTheme);
    connect(settingsWindow, &SettingsWindow::highContrastWidgets,
            mainWindow, &MainWindow::setHighContrastWidgets);
    connect(settingsWindow, &SettingsWindow::infoStatsColors,
            mainWindow, &MainWindow::setInfoColors);
    connect(settingsWindow, &SettingsWindow::volume,
            mainWindow, &MainWindow::setVolume);
    connect(settingsWindow, &SettingsWindow::volumeStep,
            mainWindow, &MainWindow::setVolumeStep);
    connect(settingsWindow, &SettingsWindow::zoomPreset,
            mainWindow, &MainWindow::setZoomPreset);
    connect(settingsWindow, &SettingsWindow::zoomCenter,
            mainWindow, &MainWindow::setZoomCenter);
    connect(settingsWindow, &SettingsWindow::mouseHideTimeFullscreen,
            mainWindow, &MainWindow::setMouseHideTimeFullscreen);
    connect(settingsWindow, &SettingsWindow::mouseHideTimeWindowed,
            mainWindow, &MainWindow::setMouseHideTimeWindowed);
    connect(settingsWindow, &SettingsWindow::fullscreenScreen,
            mainWindow, &MainWindow::setFullscreenName);
    connect(settingsWindow, &SettingsWindow::fullscreenAtLaunch,
            mainWindow, &MainWindow::setFullscreenOnPlay);
    connect(settingsWindow, &SettingsWindow::fullscreenExitAtEnd,
            mainWindow, &MainWindow::setFullscreenExitOnEnd);
    connect(settingsWindow, &SettingsWindow::hideMethod,
            mainWindow, &MainWindow::setBottomAreaBehavior);
    connect(settingsWindow, &SettingsWindow::hideTime,
            mainWindow, &MainWindow::setBottomAreaHideTime);
    connect(settingsWindow, &SettingsWindow::hidePanels,
            mainWindow, &MainWindow::setFullscreenHidePanels);
    connect(settingsWindow, &SettingsWindow::volumeMax,
            mainWindow, &MainWindow::setVolumeMax);
    connect(settingsWindow, &SettingsWindow::timeShorten,
            mainWindow, &MainWindow::setTimeShortMode);
    connect(settingsWindow, &SettingsWindow::timeTooltip,
            mainWindow, &MainWindow::setTimeTooltip);

    // settings -> playlistWindow
    connect(settingsWindow, &SettingsWindow::iconTheme,
            mainWindow->playlistWindow(), &PlaylistWindow::setIconTheme);
    connect(settingsWindow, &SettingsWindow::hidePanels,
            mainWindow->playlistWindow(), &PlaylistWindow::setHideFullscreen);
    connect(settingsWindow, &SettingsWindow::playlistFormat,
            mainWindow->playlistWindow(), &PlaylistWindow::setDisplayFormatSpecifier);

    // playlistWindow -> settings
    connect(mainWindow->playlistWindow(), &PlaylistWindow::hideFullscreenChanged,
            settingsWindow, &SettingsWindow::setHidePanels);

    // settings -> manager
    connect(settingsWindow, &SettingsWindow::speedStep,
            playbackManager, &PlaybackManager::setSpeedStep);
    connect(settingsWindow, &SettingsWindow::speedStepAdditive,
            playbackManager, &PlaybackManager::setSpeedStepAdditive);
    connect(settingsWindow, &SettingsWindow::stepTimeLarge,
            playbackManager, &PlaybackManager::setStepTimeLarge);
    connect(settingsWindow, &SettingsWindow::stepTimeSmall,
            playbackManager, &PlaybackManager::setStepTimeSmall);
    connect(settingsWindow, &SettingsWindow::playbackForever,
            playbackManager, &PlaybackManager::setPlaybackForever);
    connect(settingsWindow, &SettingsWindow::playbackPlayTimes,
            playbackManager, &PlaybackManager::setPlaybackPlayTimes);
    connect(settingsWindow, &SettingsWindow::fallbackToFolder,
            playbackManager, &PlaybackManager::setFolderFallback);
    connect(settingsWindow, &SettingsWindow::subsPreferDefaultForced,
            playbackManager, &PlaybackManager::setSubtitlesPreferDefaultForced);
    connect(settingsWindow, &SettingsWindow::subsPreferExternal,
            playbackManager, &PlaybackManager::setSubtitlesPreferExternal);
    connect(settingsWindow, &SettingsWindow::subsIgnoreEmbeded,
            playbackManager, &PlaybackManager::setSubtitlesIgnoreEmbedded);
    connect(settingsWindow, &SettingsWindow::afterPlaybackDefault,
            playbackManager, &PlaybackManager::setAfterPlaybackAlways);
    connect(settingsWindow, &SettingsWindow::afterPlaybackDefault,
            mainWindow, &MainWindow::setPlayAfterAlways);

    // settings -> thumbnailer
    connect(settingsWindow, &SettingsWindow::screenshotDirectory,
            thumbnailerWindow, &ThumbnailerWindow::setScreenshotDirectory);
    connect(settingsWindow, &SettingsWindow::screenshotFormat,
            thumbnailerWindow, &ThumbnailerWindow::setScreenshotFormat);

    // settings -> application
    connect(settingsWindow, &SettingsWindow::applicationPalette,
            qApp, [](const QPalette &pal) { qApp->setPalette(pal); });
}

void Flow::setupMpvObjectConnections()
{
    // settings -> mpvwidget
    auto mpvObject = mainWindow->mpvObject();
    connect(settingsWindow, &SettingsWindow::videoColor,
            mpvObject, &MpvObject::setLogoBackground);
    connect(settingsWindow, &SettingsWindow::logoSource,
            mpvObject, &MpvObject::setLogoUrl);
    connect(settingsWindow, &SettingsWindow::volume,
            mpvObject, &MpvObject::setVolume);
    connect(settingsWindow, &SettingsWindow::option,
            mpvObject, &MpvObject::setCachedMpvOption);
    connect(settingsWindow, &SettingsWindow::clientDebuggingMessages,
            mpvObject, &MpvObject::setClientDebuggingMessages);
    connect(settingsWindow, &SettingsWindow::mpvLogLevel,
            mpvObject, &MpvObject::setMpvLogLevel);

    // mpvwidget -> settings
    connect(mpvObject, &MpvObject::audioDeviceList,
            settingsWindow, &SettingsWindow::setAudioDevices);

    // mpvwidget -> properties
    connect(mpvObject, &MpvObject::fileNameChanged,
            propertiesWindow, &PropertiesWindow::setFileName);
    connect(mpvObject, &MpvObject::fileFormatChanged,
            propertiesWindow, &PropertiesWindow::setFileFormat);
    connect(mpvObject, &MpvObject::fileSizeChanged,
            propertiesWindow, &PropertiesWindow::setFileSize);
    connect(mpvObject, &MpvObject::playLengthChanged,
            propertiesWindow, &PropertiesWindow::setMediaLength);
    connect(mpvObject, &MpvObject::videoSizeChanged,
            propertiesWindow, &PropertiesWindow::setVideoSize);
    connect(mpvObject, &MpvObject::fileCreationTimeChanged,
            propertiesWindow, &PropertiesWindow::setFileCreationTime);
    connect(mpvObject, &MpvObject::tracksChanged,
            propertiesWindow, &PropertiesWindow::setTracks);
    connect(mpvObject, &MpvObject::mediaTitleChanged,
            propertiesWindow, &PropertiesWindow::setMediaTitle);
    connect(mpvObject, &MpvObject::filePathChanged,
            propertiesWindow, &PropertiesWindow::setFilePath);
    connect(mpvObject, &MpvObject::metaDataChanged,
            propertiesWindow, &PropertiesWindow::setMetaData);
    connect(mpvObject, &MpvObject::chaptersChanged,
            propertiesWindow, &PropertiesWindow::setChapters);

    // settingswindow -> log
    auto logger = Logger::singleton();
    connect(settingsWindow, &SettingsWindow::loggingEnabled,
            logger, &Logger::setLoggingEnabled);
    connect(settingsWindow, &SettingsWindow::logFilePath,
            logger, &Logger::setLogFile);
    connect(settingsWindow, &SettingsWindow::logDelay,
            logger, &Logger::setFlushTime);
    connect(settingsWindow, &SettingsWindow::logHistory,
            logWindow, &LogWindow::setLogLimit);
}

void Flow::setupFlowConnections()
{
    // mainwindow -> this
    connect(mainWindow, &MainWindow::recentOpened,
            this, &Flow::mainwindow_recentOpened);
    connect(mainWindow, &MainWindow::recentClear,
            this, &Flow::mainwindow_recentClear);
    connect(mainWindow, &MainWindow::takeImage,
            this, &Flow::mainwindow_takeImage);
    connect(mainWindow, &MainWindow::takeImageAutomatically,
            this, &Flow::mainwindow_takeImageAutomatically);
    connect(mainWindow, &MainWindow::takeThumbnails,
            this, &Flow::mainwindow_takeThumbnails);
    connect(mainWindow, &MainWindow::optionsOpenRequested,
            this, &Flow::mainwindow_optionsOpenRequested);
    connect(mainWindow, &MainWindow::instanceShouldQuit,
            this, &Flow::mainwindow_instanceShouldQuit);

    // manager -> this
    connect(playbackManager, &PlaybackManager::nowPlayingChanged,
            this, &Flow::manager_nowPlayingChanged);
    connect(playbackManager, &PlaybackManager::stateChanged,
            this, &Flow::manager_stateChanged);
    connect(playbackManager, &PlaybackManager::instanceShouldClose,
            this, &Flow::mainwindow_instanceShouldQuit);
    connect(playbackManager, &PlaybackManager::subtitlesVisibile,
            this, &Flow::manager_subtitlesVisibile);
    connect(playbackManager, &PlaybackManager::hasNoSubtitles,
            this, &Flow::manager_hasNoSubtitles);

    // settings -> this
    connect(settingsWindow, &SettingsWindow::settingsData,
            this, &Flow::settingswindow_settingsData);
    connect(settingsWindow, &SettingsWindow::keyMapData,
            this, &Flow::settingswindow_keymapData);
    connect(settingsWindow, &SettingsWindow::inhibitScreensaver,
            this, &Flow::settingswindow_inhibitScreensaver);
    connect(settingsWindow, &SettingsWindow::rememberWindowGeometry,
            this, &Flow::settingswindow_rememberWindowGeometry);
    connect(settingsWindow, &SettingsWindow::mprisIpc,
            this, &Flow::settingswindow_mprisIpc);
    connect(settingsWindow, &SettingsWindow::stylesheetIsFusion,
            this, &Flow::settingswindow_stylesheetIsFusion);
    connect(settingsWindow, &SettingsWindow::stylesheetText,
            this, &Flow::settingswindow_stylesheetText);
    connect(settingsWindow, &SettingsWindow::screenshotDirectory,
            this, &Flow::settingswindow_screenshotDirectory);
    connect(settingsWindow, &SettingsWindow::encodeDirectory,
            this, &Flow::settingswindow_encodeDirectory);
    connect(settingsWindow, &SettingsWindow::screenshotTemplate,
            this, &Flow::settingswindow_screenshotTemplate);
    connect(settingsWindow, &SettingsWindow::encodeTemplate,
            this, &Flow::settingswindow_encodeTemplate);
    connect(settingsWindow, &SettingsWindow::screenshotFormat,
            this, &Flow::settingswindow_screenshotFormat);

    // playlistwindow -> this.storage
    connect(mainWindow->playlistWindow(), &PlaylistWindow::importPlaylist,
            this, &Flow::importPlaylist);
    connect(mainWindow->playlistWindow(), &PlaylistWindow::exportPlaylist,
            this, &Flow::exportPlaylist);

    // manager -> this.screensaver
    connect(playbackManager, &PlaybackManager::systemShouldHibernate,
            screenSaver, &ScreenSaver::hibernateSystem);
    connect(playbackManager, &PlaybackManager::systemShouldLock,
            screenSaver, &ScreenSaver::lockScreen);
    connect(playbackManager, &PlaybackManager::systemShouldLogOff,
            screenSaver, &ScreenSaver::logOff);
    connect(playbackManager, &PlaybackManager::systemShouldShutdown,
            screenSaver, &ScreenSaver::shutdownSystem);
    connect(playbackManager, &PlaybackManager::systemShouldStandby,
            screenSaver, &ScreenSaver::suspendSystem);

    // favorites -> this.favorite*
    connect(favoritesWindow, &FavoritesWindow::favoriteTracks,
            this, &Flow::favoriteswindow_favoriteTracks);

    // this.screensaver -> this
    connect(screenSaver, &ScreenSaver::systemShutdown,
            this, &Flow::endProgram);
    connect(screenSaver, &ScreenSaver::loggedOff,
            this, &Flow::endProgram);

    // this -> mainwindow
    connect(this, &Flow::recentFilesChanged,
            mainWindow, &MainWindow::setRecentDocuments);

    // this -> this
    connect(this, &Flow::windowsRestored,
            this, &Flow::self_windowsRestored);
}

void Flow::setupMpris()
{
    // Don't wire up if there's no dbus, such as on Windows and Mac.
#ifdef QT_DBUS_LIB
    mpris = new MprisInstance(this);
    connect(mainWindow, &MainWindow::volumeChanged,
            mpris, &MprisInstance::mainwindow_volumeChanged);
    connect(mainWindow, &MainWindow::fullscreenModeChanged,
            mpris, &MprisInstance::mainwindow_fullscreenModeChanged);
    connect(playbackManager, &PlaybackManager::timeChanged,
            mpris, &MprisInstance::manager_timeChanged);
    connect(playbackManager, &PlaybackManager::stateChanged,
            mpris, &MprisInstance::manager_stateChanged);
    connect(playbackManager, &PlaybackManager::nowPlayingChanged,
            mpris, &MprisInstance::manager_nowPlayingChanged);
    connect(mainWindow->mpvObject(), &MpvObject::mediaTitleChanged,
            mpris, &MprisInstance::mpvObject_mediaTitleChanged);
    connect(mainWindow->mpvObject(), &MpvObject::metaDataChanged,
            mpris, &MprisInstance::mpvObject_metaDataChanged);
    connect(mainWindow->playlistWindow(), &PlaylistWindow::currentPlaylistHasItems,
            mpris, &MprisInstance::playlistwindow_currentPlaylistHasItems);

    connect(mpris, &MprisInstance::fullscreenMode,
            mainWindow, &MainWindow::setFullscreenMode);
    connect(mpris, &MprisInstance::raiseWindow,
            mainWindow, &MainWindow::raise);
    connect(mpris, &MprisInstance::closeInstance,
            mainWindow, &MainWindow::instanceShouldQuit);
    connect(mpris, &MprisInstance::volumeChange,
            mainWindow, &MainWindow::setVolumeDouble);
    connect(mpris, &MprisInstance::playNextTrack,
            playbackManager, &PlaybackManager::playNext);
    connect(mpris, &MprisInstance::playPreviousTrack,
            playbackManager, &PlaybackManager::playPrev);
    connect(mpris, &MprisInstance::pause,
            playbackManager, &PlaybackManager::pausePlayer);
    connect(mpris, &MprisInstance::playpause,
            playbackManager, &PlaybackManager::playPausePlayer);
    connect(mpris, &MprisInstance::stop,
            playbackManager, &PlaybackManager::stopPlayer);
    connect(mpris, &MprisInstance::play,
            playbackManager, &PlaybackManager::playPlayer);
    connect(mpris, &MprisInstance::relativeSeek,
            mainWindow->mpvObject(), [this](double amount) {
        mainWindow->mpvObject()->seek(amount, false);
    });
    connect(mpris, &MprisInstance::absoluteSeek,
            mainWindow->mpvObject(), &MpvObject::setTime);

    mpris->setProtocolList(mainWindow->mpvObject()->supportedProtocols());
#endif
}

void Flow::setupMpcHc()
{
    // mpcHcServer -> flow
    connect(mpcHcServer, &MpcHcServer::fileSelected,
            this, &Flow::mpcHcServer_fileSelected);

    // settings -> mpcHcServer
    connect(settingsWindow, &SettingsWindow::webserverListening,
            mpcHcServer, &MpcHcServer::setEnabled);
    connect(settingsWindow, &SettingsWindow::webserverPort,
            mpcHcServer, &MpcHcServer::setTcpPort);
    connect(settingsWindow, &SettingsWindow::webserverLocalhost,
            mpcHcServer, &MpcHcServer::setLocalHostOnly);
    connect(settingsWindow, &SettingsWindow::webserverServePages,
            mpcHcServer, &MpcHcServer::setServeFiles);
    connect(settingsWindow, &SettingsWindow::webserverRoot,
            mpcHcServer, &MpcHcServer::setWebRoot);
    connect(settingsWindow, &SettingsWindow::webserverDefaultPage,
            mpcHcServer, &MpcHcServer::setDefaultPage);

    // mpcHcServer -> mainWindow
    connect(mpcHcServer, &MpcHcServer::quickOpenFile,
            mainWindow, &MainWindow::httpQuickOpenFile);
    connect(mpcHcServer, &MpcHcServer::openFileUrl,
            mainWindow, &MainWindow::httpOpenFileUrl);
    connect(mpcHcServer, &MpcHcServer::saveImage,
            mainWindow, &MainWindow::httpSaveImage);
    connect(mpcHcServer, &MpcHcServer::saveImageAuto,
            mainWindow, &MainWindow::httpSaveImageAuto);
    connect(mpcHcServer, &MpcHcServer::saveThumbnails,
            mainWindow, &MainWindow::httpSaveThumbnails);
    connect(mpcHcServer, &MpcHcServer::close,
            mainWindow, &MainWindow::httpClose);
    connect(mpcHcServer, &MpcHcServer::properties,
            mainWindow, &MainWindow::httpProperties);
    connect(mpcHcServer, &MpcHcServer::exit,
            mainWindow, &MainWindow::httpExit);
    connect(mpcHcServer, &MpcHcServer::play,
            mainWindow, &MainWindow::httpPlay);
    connect(mpcHcServer, &MpcHcServer::pause,
            mainWindow, &MainWindow::httpPause);
    connect(mpcHcServer, &MpcHcServer::stop,
            mainWindow, &MainWindow::httpStop);
    connect(mpcHcServer, &MpcHcServer::frameStep,
            mainWindow, &MainWindow::httpFrameStep);
    connect(mpcHcServer, &MpcHcServer::frameStepBack,
            mainWindow, &MainWindow::httpFrameStepBack);
    connect(mpcHcServer, &MpcHcServer::increaseRate,
            mainWindow, &MainWindow::httpIncreaseRate);
    connect(mpcHcServer, &MpcHcServer::decreaseRate,
            mainWindow, &MainWindow::httpDecreaseRate);
    connect(mpcHcServer, &MpcHcServer::quickAddFavorite,
            mainWindow, &MainWindow::httpQuickAddFavorite);
    connect(mpcHcServer, &MpcHcServer::organizeFavorites,
            mainWindow, &MainWindow::httpOrganizeFavories);
    connect(mpcHcServer, &MpcHcServer::toggleCaptionMenu,
            mainWindow, &MainWindow::httpToggleCaptionMenu);
    connect(mpcHcServer, &MpcHcServer::toggleSeekBar,
            mainWindow, &MainWindow::httpToggleSeekBar);
    connect(mpcHcServer, &MpcHcServer::toggleControls,
            mainWindow, &MainWindow::httpToogleControls);
    connect(mpcHcServer, &MpcHcServer::toggleInformation,
            mainWindow, &MainWindow::httpToggleInformation);
    connect(mpcHcServer, &MpcHcServer::toggleStatistics,
            mainWindow, &MainWindow::httpToggleStatistics);
    connect(mpcHcServer, &MpcHcServer::toggleStatus,
            mainWindow, &MainWindow::httpToggleStatus);
    connect(mpcHcServer, &MpcHcServer::togglePlaylistBar,
            mainWindow, &MainWindow::httpTogglePlaylistBar);
    connect(mpcHcServer, &MpcHcServer::viewMinimal,
            mainWindow, &MainWindow::httpViewMinimal);
    connect(mpcHcServer, &MpcHcServer::viewCompact,
            mainWindow, &MainWindow::httpViewCompact);
    connect(mpcHcServer, &MpcHcServer::viewNormal,
            mainWindow, &MainWindow::httpViewNormal);
    connect(mpcHcServer, &MpcHcServer::fullscreen,
            mainWindow, &MainWindow::httpFullscreen);
    connect(mpcHcServer, &MpcHcServer::zoom25,
            mainWindow, &MainWindow::httpZoom25);
    connect(mpcHcServer, &MpcHcServer::zoom50,
            mainWindow, &MainWindow::httpZoom50);
    connect(mpcHcServer, &MpcHcServer::zoom100,
            mainWindow, &MainWindow::httpZoom100);
    connect(mpcHcServer, &MpcHcServer::zoom200,
            mainWindow, &MainWindow::httpZoom200);
    connect(mpcHcServer, &MpcHcServer::zoomAutoFit,
            mainWindow, &MainWindow::httpZoomAutoFit);
    connect(mpcHcServer, &MpcHcServer::zoomAutoFitLarger,
            mainWindow, &MainWindow::httpZoomAutoFitLarger);
    connect(mpcHcServer, &MpcHcServer::volumeUp,
            mainWindow, &MainWindow::httpVolumeUp);
    connect(mpcHcServer, &MpcHcServer::volumeDown,
            mainWindow, &MainWindow::httpVolumeDown);
    connect(mpcHcServer, &MpcHcServer::volumeMute,
            mainWindow, &MainWindow::httpVolumeMute);
    connect(mpcHcServer, &MpcHcServer::nextSubtitleTrack,
            mainWindow, &MainWindow::httpNextSubtitle);
    connect(mpcHcServer, &MpcHcServer::previousSubtitleTrack,
            mainWindow, &MainWindow::httpPrevSubtitle);
    connect(mpcHcServer, &MpcHcServer::onOffSubtitles,
            mainWindow, &MainWindow::httpOnOffSubtitles);
    connect(mpcHcServer, &MpcHcServer::afterPlaybackDoNothing,
            mainWindow, &MainWindow::httpAfterPlaybackNothing);
    connect(mpcHcServer, &MpcHcServer::afterPlaybackPlayNext,
            mainWindow, &MainWindow::httpAfterPlaybackPlayNext);
    connect(mpcHcServer, &MpcHcServer::afterPlaybackExit,
            mainWindow, &MainWindow::httpAfterPlaybackExit);
    connect(mpcHcServer, &MpcHcServer::afterPlaybackStandBy,
            mainWindow, &MainWindow::httpAfterPlaybackStandBy);
    connect(mpcHcServer, &MpcHcServer::afterPlaybackHibernate,
            mainWindow, &MainWindow::httpAfterPlaybackHibernate);
    connect(mpcHcServer, &MpcHcServer::afterPlaybackShutdown,
            mainWindow, &MainWindow::httpAfterPlaybackShutdown);
    connect(mpcHcServer, &MpcHcServer::afterPlaybackLogOff,
            mainWindow, &MainWindow::httpAfterPlaybackLogOff);
    connect(mpcHcServer, &MpcHcServer::afterPlaybackLock,
            mainWindow, &MainWindow::httpAfterPlaybackLock);

    // mpcHcServer -> playbackManager
    connect(mpcHcServer, &MpcHcServer::nextFile,
            playbackManager, &PlaybackManager::playNext);
    connect(mpcHcServer, &MpcHcServer::previousFile,
            playbackManager, &PlaybackManager::playPrev);

    // mainWindow -> mpcHcServer
    connect(mainWindow, &MainWindow::volumeChanged,
            mpcHcServer, &MpcHcServer::setVolume);
    connect(mainWindow, &MainWindow::volumeMuteChanged,
            mpcHcServer, &MpcHcServer::setVolumeMuted);

    // mpvObject -> mpcHcServer
    MpvObject *mpvObject = mainWindow->mpvObject();
    connect(mpvObject, &MpvObject::fileSizeChanged,
            mpcHcServer, &MpcHcServer::setFileSize);

    // playbackManager -> mpcHcServer
    connect(playbackManager, &PlaybackManager::timeChanged,
            mpcHcServer, &MpcHcServer::setFileTime);
    connect(playbackManager, &PlaybackManager::titleChanged,
            mpcHcServer, &MpcHcServer::setMediaTitle);
    connect(playbackManager, &PlaybackManager::nowPlayingChanged,
            mpcHcServer, [this](QUrl itemUrl, QUuid listUuid, QUuid itemUuid) {
            Q_UNUSED(listUuid);
            Q_UNUSED(itemUuid);
            mpcHcServer->setNowPlaying(itemUrl); });
    connect(playbackManager, &PlaybackManager::playbackSpeedChanged,
            mpcHcServer, &MpcHcServer::setPlaybackRate);
    connect(playbackManager, &PlaybackManager::stateChanged,
            mpcHcServer, &MpcHcServer::setPlaybackState);
}

QByteArray Flow::makePayload() const
{
    // Create a JSON document containing a file open request suitable for
    // sending to our mpc-qt socket.
    QVariantMap map({
        {keyCommand, QVariant("playFiles")},
        {keyDirectory, QVariant(QDir::currentPath())},
        {keyFiles, QVariant(customFiles)}
    });
    return QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
}

QString Flow::pictureTemplate(Helpers::DisabledTrack tracks, Helpers::Subtitles subs) const
{
    double playTime = mainWindow->mpvObject()->playTime();
    QUrl nowPlaying = playbackManager->nowPlaying();
    QString basename = QFileInfo(nowPlaying.toDisplayString().split('/').last())
                       .completeBaseName();

    // FIXME: Use parseFormatEx?
    QString fileName = Helpers::parseFormat(screenshotTemplate, basename, tracks, subs, playTime, 0, 0);

    // Filesystems typically support 255 characters for filename length
    int maxLength = 255 - (screenshotFormat.length() + 1);
    if (fileName.length() >= maxLength) {
        basename.truncate(basename.length() - (fileName.length() - maxLength));
        fileName = Helpers::parseFormat(screenshotTemplate, basename, tracks, subs, playTime, 0, 0);
    }

    QString filePath = screenshotDirectory;
    if (filePath.isEmpty()) {
        if (nowPlaying.isLocalFile())
            filePath = QFileInfo(nowPlaying.toLocalFile()).path();
        else
            filePath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    }
    QDir().mkpath(filePath);
    fileName = Platform::sanitizedFilename(fileName);
    return filePath + "/" + fileName + "." + screenshotFormat;
}

QVariantList Flow::recentToVList() const
{
    return TrackInfo::tracksToVList(recentFiles);
}

QVariantMap Flow::favoritesToVMap() const
{
    return QVariantMap {
        { keyFiles, TrackInfo::tracksToVList(favoriteFiles) },
        { keyStreams, TrackInfo::tracksToVList(favoriteStreams) }
    };
}

QVariantMap Flow::windowsToVMap_v2()
{
    if (!rememberWindowGeometry)
        return QVariantMap();

    windowManager.clearJson();
    windowManager.saveDocks(mainWindow->mpvHost());
    windowManager.saveWindow(settingsWindow);
    windowManager.saveWindow(propertiesWindow);
    windowManager.saveWindow(logWindow);
    windowManager.saveWindow(libraryWindow);
    windowManager.saveAppWindow(mainWindow);
    return windowManager.json();
}

void Flow::restoreWindows_v2(const QVariantMap &geometryMap)
{
    CliInfo cliInfo { cliPos, cliSize, validCliPos, validCliSize };

    windowManager.setJson(geometryMap);
    windowManager.restoreDocks(mainWindow->mpvHost(), { mainWindow->playlistWindow() });
    windowManager.restoreWindow(settingsWindow);
    windowManager.restoreWindow(propertiesWindow);
    windowManager.restoreWindow(logWindow);
    windowManager.restoreWindow(libraryWindow);
    windowManager.restoreAppWindow(mainWindow, cliInfo);

    // Tell the main window it can process size requests et al now
    mainWindow->unfreezeWindow();
    // In 50 msec time when the ui is in a reliable state, call
    // self_windowsRestored for further processing (2022-03: such as opening
    // files for playback)
    QTimer::singleShot(50, this, &Flow::windowsRestored);
}

void Flow::self_windowsRestored()
{
    server->fakePayload(makePayload());
}

void Flow::mainwindow_instanceShouldQuit()
{
    endProgram();
}

void Flow::mainwindow_recentOpened(const TrackInfo &track)
{
    // attempt to play the playlist item if possible, otherwise act like it
    // is a new file
    QUrl old = mainWindow->playlistWindow()->getUrlOf(track.list, track.item);
    if (!old.isEmpty())
        playbackManager->playItem(track.list, track.item);
    else
        playbackManager->openFile(track.url);

    // Navigate to a particular position if set, such as if this is from the
    // favorites menu
    if (track.position > 0 && track.url.isLocalFile())
        playbackManager->navigateToTime(track.position);
}

void Flow::mainwindow_recentClear()
{
    recentFiles.clear();
    mainWindow->setRecentDocuments(recentFiles);
}

void Flow::mainwindow_takeImage(Helpers::ScreenshotRender render)
{
    // Take a screenshot and save it to a temporary file
    QString fmt("%1/mpc-qt_%2.%3");
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString tempFile = fmt.arg(tempDir, QUuid::createUuid().toString(), screenshotFormat);
    mainWindow->mpvObject()->screenshot(tempFile, render);

    // Do some quick logic based on track presence
    Helpers::Subtitles subRender;
    if (nowPlayingNoSubtitleTracks)
        subRender = Helpers::NoSubtitles;
    else if (render == Helpers::VideoRender)
        subRender = Helpers::SubtitlesDisabled;
    else if (!nowPlayingDisplayingSubtitles)
        subRender = Helpers::SubtitlesDisabled;
    else
        subRender = Helpers::SubtitlesPresent;

    QString fileName = pictureTemplate(Helpers::DisabledAudio, subRender);

    QString picFile;
    picFile = QFileDialog::getSaveFileName(this->mainWindow, tr("Save Image"),
                                           fileName);

    // Copy the temp file to the desired location, then delete it
    QFile tf(tempFile);
    if (!picFile.isEmpty()) {
        QFile pf(picFile);
        if (pf.exists())
            pf.remove();
        tf.copy(picFile);
    }
    tf.remove();
}

void Flow::mainwindow_takeImageAutomatically(Helpers::ScreenshotRender render)
{
    // Like mainwindow_takeImage, but we do the screenshot knowing where to
    // put it
    Helpers::Subtitles subRender;
    if (nowPlayingNoSubtitleTracks)
        subRender = Helpers::NoSubtitles;
    else if (render == Helpers::VideoRender)
        subRender = Helpers::SubtitlesDisabled;
    else if (!nowPlayingDisplayingSubtitles)
        subRender = Helpers::SubtitlesDisabled;
    else
        subRender = Helpers::SubtitlesPresent;
    QString fileName = pictureTemplate(Helpers::DisabledAudio, subRender);
    mainWindow->mpvObject()->screenshot(fileName, render);
}

void Flow::mainwindow_takeThumbnails()
{
    thumbnailerWindow->open(playbackManager->nowPlaying());
}

void Flow::mainwindow_optionsOpenRequested()
{
    // Load the settings window with data and show it
    settingsWindow->takeSettings(settings);
    settingsWindow->takeKeyMap(keyMap);
    settingsWindow->show();
    settingsWindow->raise();
}

void Flow::manager_nowPlayingChanged(QUrl url, QUuid listUuid, QUuid itemUuid)
{
    // Insert playing track as the most recent item
    TrackInfo track(url, listUuid, itemUuid, QString(), 0, 0);
    if (recentFiles.contains(track)) {
        // Remove all prior mention of it
        recentFiles.removeAll(track);
    }
    recentFiles.insert(0, track);

    // Trim the recent file list
    if (recentFiles.size() > 10)
        recentFiles.removeLast();

    // Notify (2022-03: the main window) that the recents have changed
    emit recentFilesChanged(recentFiles);
}

void Flow::manager_stateChanged(PlaybackManager::PlaybackState state)
{
    // Do nothing if we don't have to
    if (!manipulateScreensaver)
        return;

    // If inhibiting the screensaver is switched off, or we just entered the
    // stopped state, unihibit the screensaver
    if (!inhibitScreensaver || state == PlaybackManager::StoppedState) {
        screenSaver->uninhibitSaver();
        return;
    }

    // Else: inhibit the screensaver because we're not stopped
    screenSaver->inhibitSaver(tr("Playing Media"));
}

void Flow::manager_subtitlesVisibile(bool visible)
{
    // Remember that subtitles are displayed (used for saving screenshots)
    // or not
    nowPlayingDisplayingSubtitles = visible;
}

void Flow::manager_hasNoSubtitles(bool none)
{
    // Remember that there's no subtitle tracks (used for saving screenshots)
    // or not
    nowPlayingNoSubtitleTracks = none;
}

void Flow::mpcHcServer_fileSelected(QString fileName)
{
    // Send the file open request to our playback manager
    QList<QUrl> urls;
    urls << QUrl::fromLocalFile(fileName);
    playbackManager->openSeveralFiles(urls, true);
}

void Flow::settingswindow_settingsData(const QVariantMap &settings)
{
    // The selected options have changed, so write them to disk
    this->settings = settings;
    writeConfig(true);
}

void Flow::settingswindow_inhibitScreensaver(bool yes)
{
    // Remember inhibit screensaver and renotify ourselves of the current
    // playback state (2022-03: which optionally re/uninhibits the
    // screensaver)
    this->inhibitScreensaver = yes;
    manager_stateChanged(playbackManager->playbackState());
}

void Flow::settingswindow_rememberWindowGeometry(bool yes)
{
    // Remember our preference to process the window geometry when we exit
    this->rememberWindowGeometry = yes;
}

void Flow::settingswindow_keymapData(const QVariantMap &keyMap)
{
    // Remember the keymap (shortcuts) data
    this->keyMap = keyMap;
}

void Flow::settingswindow_mprisIpc(bool enabled)
{
#ifdef QT_DBUS_LIB
    // Do nothing if we don't have to
    if (!mpris)
        return;

    // Going off: turn it off only if it were on
    if (!enabled && mpris->registered()) {
        mpris->unregisterDBus();
    }
    // Going on: turn it on only if it were off
    if (enabled && !mpris->registered()) {
        mpris->registerDBus();
    }
#else
    Q_UNUSED(enabled)
#endif
}

void Flow::settingswindow_stylesheetIsFusion(bool yes)
{
    static QString originalApplicationStyle;
    if (originalApplicationStyle.isNull()) {
        originalApplicationStyle = qApp->style()->name();
    }

    bool wasFusion = qApp->style()->name() == "Fusion";
    if (!yes && wasFusion)
        qApp->setStyle(originalApplicationStyle);
    if (yes && !wasFusion)
        qApp->setStyle(QStyleFactory::create("Fusion"));
}

void Flow::settingswindow_stylesheetText(QString text)
{
    QString oldSheet = qApp->styleSheet();
    if (oldSheet == text)
        return;

    qApp->setStyleSheet(text);
}

void Flow::settingswindow_screenshotDirectory(const QString &where)
{
    this->screenshotDirectory = where;
}

void Flow::settingswindow_encodeDirectory(const QString &where)
{
    this->encodeDirectory = where;
}

void Flow::settingswindow_screenshotTemplate(const QString &fmt)
{
    this->screenshotTemplate = fmt;
}

void Flow::settingswindow_encodeTemplate(const QString &fmt)
{
    this->encodeTemplate = fmt;
}

void Flow::settingswindow_screenshotFormat(const QString &fmt)
{
    this->screenshotFormat = fmt;
}

void Flow::favoriteswindow_favoriteTracks(const QList<TrackInfo> &files, const QList<TrackInfo> &streams)
{
    // Remember our favorite files and streams for later
    favoriteFiles = files;
    favoriteStreams = streams;
}

void Flow::endProgram()
{
    // We're about to quit, so write out our config
    writeConfig();
    QMetaObject::invokeMethod(qApp, "exit", Qt::QueuedConnection);
}

void Flow::importPlaylist(QString fname)
{
    //CHECKME: addSimplePlaylist should be a slot?
    mainWindow->playlistWindow()->addSimplePlaylist(storage.readM3U(fname));
}

void Flow::exportPlaylist(QString fname, QStringList items)
{
    storage.writeM3U(fname, items);
}

