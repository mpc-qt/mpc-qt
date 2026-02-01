#include <clocale>
#include <csignal>
#include <cstring>
#include <QApplication>
#include <QFileDialog>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QTimer>
#include <QCommandLineParser>
#include <QStyleFactory>
#include <QLockFile>
#include <QThread>
#include <QTranslator>
#ifdef Boost_FOUND
#include <boost/stacktrace.hpp>
#endif
#include "logger.h"
#include "main.h"
#include "qprocess.h"
#include "storage.h"
#include "mainwindow.h"
#include "manager.h"
#include "settingswindow.h"
#include "mpvwidget.h"
#include "propertieswindow.h"
#include "version.h"
#include "ipc/mpris.h"
#include "platform/devicemanager.h"
#include "platform/unify.h"
#include "playlist.h"

//---------------------------------------------------------------------------

static constexpr char logModule[] =  "main";

static constexpr char desktopFile[] = "io.github.mpc_qt.mpc-qt";

static constexpr char keyCommand[] = "command";
static constexpr char keyDirectory[] = "directory";
static constexpr char keyFiles[] = "files";
static constexpr char keyStreams[] = "streams";

static constexpr char optConsoleLog[] = "log-to-console";
static constexpr char optConsoleLogEx[] = "--log-to-console";

//---------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName("MPC-QT");
    Logger::log(logModule, "starting logging");
    #if !defined(Q_OS_WIN)
    std::signal(SIGHUP, signalHandler);
    #endif
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGABRT, signalHandler);
    std::signal(SIGSEGV, signalHandler);

    Flow::earlyPlatformOverride();

    QApplication a(argc, argv);
    bool foundLoggingOpt = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], optConsoleLogEx)) {
            foundLoggingOpt = true;
            break;
        }
    }
    Logger::setConsoleLogging(foundLoggingOpt);
    Logger::singleton();
    QApplication::setWindowIcon(QIcon(":/images/icon/mpc-qt.svg"));

    // Qt sets the locale in the QApplication constructor, but libmpv requires
    // the LC_NUMERIC category to be set to "C", so change it back.
    std::setlocale(LC_NUMERIC, "C");

    // Register the error code type et al so that events can serialize them.
    qRegisterMetaType<MpvController::PropertyList>("MpvController::PropertyList");
    qRegisterMetaType<MpvController::OptionList>("MpvController::OptionList");
    qRegisterMetaType<MpvErrorCode>("MpvErrorCode");
    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<uint16_t>("uint16_t");

    QTranslator qtTranslator;
    QTranslator appTranslator;

    Flow::setTranslation(&qtTranslator, &appTranslator);

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
    try {
        return f.run();
    }
    catch (const std::exception &e) {
        Logger::log(logModule, QString("Uncaught exception: %1").arg(e.what()));
        throw;
    }
}

void signalHandler(int signal) {
    if (signal == SIGSEGV || SIGABRT) {
        std::ostringstream stacktrace;
#ifdef Boost_FOUND
        stacktrace << boost::stacktrace::stacktrace();
        Logger::log(logModule, "Stack trace:");
        Logger::log(logModule, QString::fromStdString(stacktrace.str()));
#endif
        if (signal == SIGSEGV)
            Logger::log(logModule, "Segmentation fault!");
        Logger::log(logModule, "Please report this error.");
        QMetaObject::invokeMethod(Logger::singleton(), "flushMessages", Qt::BlockingQueuedConnection);
        if (signal == SIGSEGV) {
            std::signal(SIGSEGV, SIG_DFL);
            std::raise(SIGSEGV);
        }
    }
    QMetaObject::invokeMethod(qApp, "exit", Qt::QueuedConnection);
}

//---------------------------------------------------------------------------

bool Flow::isWayland = false;
bool Flow::isWaylandMode = false;

Flow::Flow(QObject *owner) :
    QObject(owner)
{
    // Start logging early
    logThread = new QThread();
    logThread->start();
    Logger *logger = Logger::singleton();
    logger->moveToThread(logThread);
    connect(logThread, &QThread::finished,
            logger, &QObject::deleteLater);
    connect(this, &Flow::flushLog,
            logger, &Logger::flushMessages,
            Qt::BlockingQueuedConnection);

    readConfig();
    Logger::log(logModule, "finished reading config");
}

Flow::~Flow()
{
    Logger::log(logModule, "~Flow");
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
            updateRecentPosition(false);
            settings = settingsWindow->settings();
            writeConfig();
            storage.writeVList(filePlaylists,
                               mainWindow->playlistWindow()->tabsToVList(rememberQuickPlaylist));
            if (playlistsBackupLoaded)
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
    if (gotoWindow) {
        delete gotoWindow;
        gotoWindow = nullptr;
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
    if (libraryWindow) {
        delete libraryWindow;
        libraryWindow = nullptr;
    }
    if (logWindow) {
        delete logWindow;
        logWindow = nullptr;
    }
    if (logThread) {
        Logger::log(logModule, "flushing log before closing it");
        emit flushLog();
        logThread->quit();
        logThread->wait();
        delete logThread;
        logThread = nullptr;
    }
}

void Flow::parseArgs()
{
    Logger::log(logModule, "parsing arguments");
    QCommandLineParser parser;
    parser.setApplicationDescription(tr("Media Player Classic Qute Theater"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption freestandingOpt("freestanding", tr("Start a new process without saving data."));
    QCommandLineOption noConfigOpt("no-config", tr("Do not load any config files."));
    QCommandLineOption noFilesOpt("no-files", tr("Do not load file history, playlists, or favorites."));
    QCommandLineOption sizeOpt("size", tr("Main window size."), "w,h");
    QCommandLineOption posOpt("pos", tr("Main window position."), "x,y");
    QCommandLineOption loggingOpt(optConsoleLog, tr("Also write logging messages to console."));

    parser.addOption(freestandingOpt);
    parser.addOption(noConfigOpt);
    parser.addOption(noFilesOpt);
    parser.addOption(sizeOpt);
    parser.addOption(posOpt);
    parser.addOption(loggingOpt);
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
    Logger::log(logModule, "determining program mode");

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

    if (programMode == PrimaryMode) {
        QString lockFilePath =
            Storage::fetchConfigPath() + QLatin1Char('/') + QLatin1String("mpc-qt.lock");
        std::unique_ptr<QLockFile> lockFile = std::make_unique<QLockFile>(lockFilePath);
        lockFile->setStaleLockTime(10000);
        while (!lockFile->tryLock()) {
            alreadyAServer = JsonServer::sendPayload(makePayload(), MpcQtServer::defaultSocketName());
            programMode = alreadyAServer ? EarlyQuitMode : PrimaryMode;
            if (programMode == EarlyQuitMode)
                break;
        }
        if (lockFile->isLocked() || lockFile->tryLock())
            lockFile_ = std::move(lockFile);
    }
}

void Flow::init() {
    Q_ASSERT(programMode != UnknownMode);

    Logger::log(logModule, "starting init");

    // Create our windows
    Logger::log(logModule, "creating main window");
    mainWindow = new MainWindow();
    Logger::log(logModule, "creating playback manager");
    playbackManager = new PlaybackManager(this);
    playbackManager->setMpvObject(mainWindow->mpvObject(), true);
    playbackManager->setPlaylistWindow(mainWindow->playlistWindow());
    Logger::log(logModule, "creating settings window");
    settingsWindow = new SettingsWindow();
    settingsWindow->setWindowModality(Qt::WindowModal);
    settingsWindow->setWaylandOptions(isWayland, isWaylandMode);
    Logger::log(logModule, "creating properties window");
    propertiesWindow = new PropertiesWindow();
    Logger::log(logModule, "creating favorites window");
    favoritesWindow = new FavoritesWindow();
    Logger::log(logModule, "creating goto window");
    gotoWindow = new GoToWindow();
    Logger::log(logModule, "creating log window");
    logWindow = new LogWindow();
    Logger::log(logModule, "creating library window");
    libraryWindow = new LibraryWindow();
    Logger::log(logModule, "creating thumbnailer window");
    thumbnailerWindow = new ThumbnailerWindow();

    Logger::log(logModule, "finished creating windows");

    // Start our servers
    server = new MpcQtServer(mainWindow, playbackManager, this);
    server->setMainWindow(mainWindow);
    server->setPlaybackManger(playbackManager);

    mpvServer = new MpvServer(this);
    mpvServer->setPlaybackManger(playbackManager);
    mpvServer->setMpvObject(mainWindow->mpvObject());

    mpcHcServer = new MpcHcServer(this);

    Logger::log(logModule, "finished creating servers");

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
        Logger::log(logModule, "device manager active");
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
        Logger::log(logModule, "completed setting up primary servers");
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

    Logger::log(logModule, "finished initialization");
    showVersionInfo();
}


int Flow::run()
{
    // Load our data
    auto playlist = cliNoFiles ? QVariantList() : storage.readVList(filePlaylists);
    auto geometry = cliNoConfig ? QVariantMap() : storage.readVMap(fileGeometryV2);

    // Send data to the ui
    mainWindow->playlistWindow()->tabsFromVList(playlist);

    // Restore our window positions
    restoreWindows_v2(geometry);

    // Wait here until quit
    Logger::log(logModule, "telling the program to run");
    return QApplication::exec();
}

bool Flow::earlyQuit()
{
    return programMode == EarlyQuitMode;
}

void Flow::earlyPlatformOverride()
{
    if (!Platform::isUnix)
        return;

    QGuiApplication::setDesktopFileName(desktopFile);

    // Wayland doesn't support run-time centering and it doesn't look like
    // it'll support it any time soon.  I'll remove this code when it does.
    if (qEnvironmentVariable("XDG_SESSION_TYPE") == "wayland") {
        isWayland = true;
        Storage s;
        QVariantMap settings = s.readVMap(fileSettings);
        if (!settings.value("tweaksPreferWayland", QVariant(false)).toBool())
            qputenv("QT_QPA_PLATFORM", "xcb");
        else if (!qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY"))
            isWaylandMode = true;
    }
    // The Nvidia drivers don't work well with EGL
    if (!Flow::isNvidiaGPU())
        qputenv("QT_XCB_GL_INTEGRATION", "xcb_egl");
}

// Register the translations
void Flow::setTranslation(QTranslator *qtTranslator, QTranslator *appTranslator)
{
    QLocale locale;
    Storage s;
    QVariantMap settings = s.readVMap(fileSettings);
    int localeSetting = settings.value("playerLanguageComboBox_v2", 0).toInt();
    if (localeSetting != 0)
        locale = QLocale("en");

    if (qtTranslator->load(locale, "qtbase", "_", ":/i18n"))
        QApplication::installTranslator(qtTranslator);

    if (appTranslator->load(locale, "mpc-qt", "_", ":/i18n"))
        QApplication::installTranslator(appTranslator);
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
    connect(mainWindow, &MainWindow::nextAudioTrackSelected,
            playbackManager, &PlaybackManager::selectNextAudioTrack);
    connect(mainWindow, &MainWindow::previousAudioTrackSelected,
            playbackManager, &PlaybackManager::selectPrevAudioTrack);
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
    connect(mainWindow, &MainWindow::filePrevious,
            playbackManager, &PlaybackManager::playPrev);
    connect(mainWindow, &MainWindow::fileNext,
            playbackManager, &PlaybackManager::playNext);
    connect(mainWindow, &MainWindow::moveToRecycleBin,
            playbackManager, &PlaybackManager::moveToRecycleBin);
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

    // propertieswindow -> mainwindow
    connect(propertiesWindow, &PropertiesWindow::artistAndTitleChanged,
            mainWindow, &MainWindow::setMediaTitleWithFilename);

    // mainwindow -> playlistwindow
    connect(mainWindow, &MainWindow::playCurrentItemRequested,
            mainWindow->playlistWindow(), &PlaylistWindow::playCurrentItem);
    connect(mainWindow, &MainWindow::severalFilesOpenedForPlaylist,
            mainWindow->playlistWindow(), &PlaylistWindow::addToPlaylist);
    connect(mainWindow, &MainWindow::removeSelectedPlaylistItem,
            mainWindow->playlistWindow(), &PlaylistWindow::playlist_removeItemRequested);

    // manager -> mainwindow
    connect(playbackManager, &PlaybackManager::timeChanged,
            mainWindow, &MainWindow::setTime);
    connect(playbackManager, &PlaybackManager::titleChangedWithFilename,
            mainWindow, &MainWindow::setMediaTitleWithFilename);
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
    connect(playbackManager, &PlaybackManager::nowPlayingChanged,
            mainWindow, &MainWindow::setVideoPreviewItem);
    connect(playbackManager, &PlaybackManager::isVideo,
            mainWindow, &MainWindow::setIsVideo);

    // manager -> playlistwindow
    connect(playbackManager, &PlaybackManager::removePlaylistItemRequested,
            mainWindow->playlistWindow(), &PlaylistWindow::removePlaylistItem);

    // mainwindow -> favorites
    connect(mainWindow, &MainWindow::organizeFavorites,
            favoritesWindow, &FavoritesWindow::show);

    // mainwindow -> goto
    connect(mainWindow, &MainWindow::showGoToWindow,
            gotoWindow, &GoToWindow::init);

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

    // manager -> properties
    connect(playbackManager, &PlaybackManager::aboutToStartPlayingFile,
            propertiesWindow, &PropertiesWindow::setFileModifiedTime);

    // goto -> manager
    connect(gotoWindow, &GoToWindow::goTo,
            playbackManager, &PlaybackManager::navigateToTime);
}

void Flow::setupSettingsConnections()
{

    // mainwindow -> settings
    connect(mainWindow, &MainWindow::zoomPresetChanged,
            settingsWindow, &SettingsWindow::setZoomPreset);
    connect(mainWindow, &MainWindow::audioFilter,
            settingsWindow, &SettingsWindow::setAudioFilter);
    connect(mainWindow, &MainWindow::videoFilter,
            settingsWindow, &SettingsWindow::setVideoFilter);

    // settings -> mainwindow
    connect(settingsWindow, &SettingsWindow::trayIcon,
            mainWindow, &MainWindow::setTrayIcon);
    connect(settingsWindow, &SettingsWindow::titleBarFormat,
            mainWindow, &MainWindow::setTitleBarFormat);
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
    connect(settingsWindow, &SettingsWindow::fullscreenHideControls,
            mainWindow, &MainWindow::setControlsInFullscreen);
    connect(settingsWindow, &SettingsWindow::hidePanels,
            mainWindow, &MainWindow::setFullscreenHidePanels);
    connect(settingsWindow, &SettingsWindow::volumeMax,
            mainWindow, &MainWindow::setVolumeMax);
    connect(settingsWindow, &SettingsWindow::subtitlesDelayStep,
            mainWindow, &MainWindow::setSubtitlesDelayStep);
    connect(settingsWindow, &SettingsWindow::videoPreview,
            mainWindow, &MainWindow::setVideoPreview);
    connect(settingsWindow, &SettingsWindow::timeTooltip,
            mainWindow, &MainWindow::setTimeTooltip);
    connect(settingsWindow, &SettingsWindow::osdTimerOnSeek,
            mainWindow, &MainWindow::setOsdTimerOnSeek);

    // settings -> playlistWindow
    connect(settingsWindow, &SettingsWindow::iconTheme,
            mainWindow->playlistWindow(), &PlaylistWindow::setIconTheme);
    connect(settingsWindow, &SettingsWindow::hidePanels,
            mainWindow->playlistWindow(), &PlaylistWindow::setHideFullscreen);
    connect(settingsWindow, &SettingsWindow::playlistFormat,
            mainWindow->playlistWindow(), &PlaylistWindow::setDisplayFormatSpecifier);
    connect(settingsWindow, &SettingsWindow::rememberSelectedPlaylist,
            mainWindow->playlistWindow(), &PlaylistWindow::setRememberSelectedPlaylist);

    // playlistWindow -> settings
    connect(mainWindow->playlistWindow(), &PlaylistWindow::hideFullscreenChanged,
            settingsWindow, &SettingsWindow::setHidePanels);

    // settings -> manager
    connect(settingsWindow, &SettingsWindow::appendToQuickPlaylist,
            playbackManager, &PlaybackManager::setAppendToQuickPlaylist);
    connect(settingsWindow, &SettingsWindow::speedStep,
            playbackManager, &PlaybackManager::setSpeedStep);
    connect(settingsWindow, &SettingsWindow::speedStepAdditive,
            playbackManager, &PlaybackManager::setSpeedStepAdditive);
    connect(settingsWindow, &SettingsWindow::stepTimeNormal,
            playbackManager, &PlaybackManager::setStepTimeNormal);
    connect(settingsWindow, &SettingsWindow::stepTimeLarge,
            playbackManager, &PlaybackManager::setStepTimeLarge);
    connect(settingsWindow, &SettingsWindow::trackSubtitlePreference,
            playbackManager, &PlaybackManager::setSubtitleTrackPreference);
    connect(settingsWindow, &SettingsWindow::trackAudioPreference,
            playbackManager, &PlaybackManager::setAudioTrackPreference);
    connect(settingsWindow, &SettingsWindow::playbackForever,
            playbackManager, &PlaybackManager::setPlaybackForever);
    connect(settingsWindow, &SettingsWindow::playbackPlayTimes,
            playbackManager, &PlaybackManager::setPlaybackPlayTimes);
    connect(settingsWindow, &SettingsWindow::fastSeek,
            playbackManager, &PlaybackManager::setFastSeek);
    connect(settingsWindow, &SettingsWindow::fallbackToFolder,
            playbackManager, &PlaybackManager::setFolderFallback);
    connect(settingsWindow, &SettingsWindow::subsPreferDefaultForced,
            playbackManager, &PlaybackManager::setSubtitlesPreferDefaultForced);
    connect(settingsWindow, &SettingsWindow::subsPreferExternal,
            playbackManager, &PlaybackManager::setSubtitlesPreferExternal);
    connect(settingsWindow, &SettingsWindow::subsIgnoreEmbeded,
            playbackManager, &PlaybackManager::setSubtitlesIgnoreEmbedded);
    connect(settingsWindow, &SettingsWindow::afterPlaybackDefault,
            playbackManager, &PlaybackManager::setAfterPlaybackAlwaysDefault);
    connect(settingsWindow, &SettingsWindow::afterPlaybackDefault,
            mainWindow, &MainWindow::setPlayAfterAlwaysDefault);

    // settings -> thumbnailer
    connect(settingsWindow, &SettingsWindow::screenshotDirectory,
            thumbnailerWindow, &ThumbnailerWindow::setScreenshotDirectory);
    connect(settingsWindow, &SettingsWindow::screenshotFormat,
            thumbnailerWindow, &ThumbnailerWindow::setScreenshotFormat);

    // settings -> application
    connect(settingsWindow, &SettingsWindow::applicationPalette,
            qApp, [](const QPalette &pal) { QApplication::setPalette(pal); });
}

void Flow::setupMpvObjectConnections()
{
    // settings -> mpvwidget
    auto mpvObject = mainWindow->mpvObject();
    connect(settingsWindow, &SettingsWindow::videoColor,
            mpvObject, &MpvObject::setLogoBackground);
    connect(settingsWindow, &SettingsWindow::logoSource,
            mpvObject, &MpvObject::setLogoUrl);
    connect(settingsWindow, &SettingsWindow::option,
            mpvObject, &MpvObject::setCachedMpvOption);
    connect(settingsWindow, &SettingsWindow::optionUncached,
            mpvObject, &MpvObject::setUncachedMpvOption);
    connect(settingsWindow, &SettingsWindow::audioFilters,
            mpvObject, &MpvObject::setAudioFilters);
    connect(settingsWindow, &SettingsWindow::videoFilters,
            mpvObject, &MpvObject::setVideoFilters);
    connect(settingsWindow, &SettingsWindow::clientDebuggingMessages,
            mpvObject, &MpvObject::setClientDebuggingMessages);
    connect(settingsWindow, &SettingsWindow::mpvLogLevel,
            mpvObject, &MpvObject::setMpvLogLevel);
    connect(settingsWindow, &SettingsWindow::mpvMouseEvents,
            mpvObject, &MpvObject::setSendMouseEvents);
    connect(settingsWindow, &SettingsWindow::mpvKeyEvents,
            mpvObject, &MpvObject::setSendKeyEvents);

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

    // mpvwidget -> mainwindow
    connect(mpvObject, &MpvObject::audioTrackSet,
            mainWindow, &MainWindow::audioTrackSet);
    connect(mpvObject, &MpvObject::subtitleTrackSet,
            mainWindow, &MainWindow::subtitleTrackSet);
    connect(mpvObject, &MpvObject::videoTrackSet,
            mainWindow, &MainWindow::videoTrackSet);
    connect(mpvObject, &MpvObject::chapterTitleChanged,
            mainWindow, &MainWindow::setChapterTitle);
    connect(mpvObject, &MpvObject::aspectNameChanged,
            mainWindow, &MainWindow::setAspectName);
    connect(mpvObject, &MpvObject::mouseReleased,
            mainWindow, &MainWindow::mpvObject_mouseReleased);

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
    connect(mainWindow, &MainWindow::showLibraryWindow,
            this, &Flow::loadPlaylistsBackup);
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
    connect(mainWindow, &MainWindow::fullscreenHideControls,
            this, &Flow::mainwindow_fullscreenHideControls);
    connect(mainWindow, &MainWindow::repeatAfter,
            this, &Flow::mainwindow_repeatAfter);

    // manager -> this
    connect(playbackManager, &PlaybackManager::playLengthChanged,
            this, &Flow::manager_playLengthChanged);
    connect(playbackManager, &PlaybackManager::openingNewFile,
            this, &Flow::manager_openingNewFile);
    connect(playbackManager, &PlaybackManager::aboutToStartPlayingFile,
            this, &Flow::manager_aboutToStartPlayingFile);
    connect(playbackManager, &PlaybackManager::startedPlayingFile,
            this, &Flow::manager_startedPlayingFile);
    connect(playbackManager, &PlaybackManager::stoppedPlaying,
            this, &Flow::manager_stoppedPlaying);
    connect(playbackManager, &PlaybackManager::stateChanged,
            this, &Flow::manager_stateChanged);
    connect(playbackManager, &PlaybackManager::fileClosed,
            this, &Flow::manager_fileClosed);
    connect(playbackManager, &PlaybackManager::instanceShouldClose,
            this, &Flow::mainwindow_instanceShouldQuit);
    connect(playbackManager, &PlaybackManager::subtitlesVisible,
            this, &Flow::manager_subtitlesVisible);
    connect(playbackManager, &PlaybackManager::hasNoSubtitles,
            this, &Flow::manager_hasNoSubtitles);
    connect(playbackManager, &PlaybackManager::playingNextFile,
            this, &Flow::manager_playingNextFile);

    // settings -> this
    connect(settingsWindow, &SettingsWindow::settingsData,
            this, &Flow::settingswindow_settingsData);
    connect(settingsWindow, &SettingsWindow::keyMapData,
            this, &Flow::settingswindow_keymapData);
    connect(settingsWindow, &SettingsWindow::inhibitScreensaver,
            this, &Flow::settingswindow_inhibitScreensaver);
    connect(settingsWindow, &SettingsWindow::rememberHistory,
            this, &Flow::settingswindow_rememberHistory);
    connect(settingsWindow, &SettingsWindow::rememberFilePosition,
            this, &Flow::settingswindow_rememberFilePosition);
    connect(settingsWindow, &SettingsWindow::rememberQuickPlaylist,
            this, &Flow::settingswindow_rememberQuickPlaylist);
    connect(settingsWindow, &SettingsWindow::rememberWindowGeometry,
            this, &Flow::settingswindow_rememberWindowGeometry);
    connect(settingsWindow, &SettingsWindow::rememberPanels,
            this, &Flow::settingswindow_rememberPanels);
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
    connect(mainWindow->playlistWindow(), &PlaylistWindow::playlistsBackupRequested,
            this, &Flow::loadPlaylistsBackup);

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
    connect(favoritesWindow, &FavoritesWindow::favoriteTracksCancel,
            this, &Flow::favoriteswindow_favoriteTracksCancel);

    // this.screensaver -> this
    connect(screenSaver, &ScreenSaver::systemShutdown,
            this, &Flow::endProgram);
    connect(screenSaver, &ScreenSaver::loggedOff,
            this, &Flow::endProgram);

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
    connect(mpcHcServer, &MpcHcServer::nextAudioTrack,
            mainWindow, &MainWindow::httpNextAudio);
    connect(mpcHcServer, &MpcHcServer::previousAudioTrack,
            mainWindow, &MainWindow::httpPrevAudio);
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

    // mpcHcServer -> playlistWindow
    connect(mpcHcServer, &MpcHcServer::removeSelectedPlaylistItem,
            mainWindow->playlistWindow(), &PlaylistWindow::playlist_removeItemRequested);
            
    // mpcHcServer -> playbackManager
    connect(mpcHcServer, &MpcHcServer::nextFile,
            playbackManager, &PlaybackManager::playNext);
    connect(mpcHcServer, &MpcHcServer::previousFile,
            playbackManager, &PlaybackManager::playPrev);
    connect(mpcHcServer, &MpcHcServer::moveToRecycleBin,
            playbackManager, &PlaybackManager::moveToRecycleBin);

    // mainWindow -> mpcHcServer
    connect(mainWindow, &MainWindow::volumeChanged,
            mpcHcServer, &MpcHcServer::setVolume);
    connect(mainWindow, &MainWindow::volumeMuteChanged,
            mpcHcServer, &MpcHcServer::setVolumeMuted);

    // mpvObject -> mpcHcServer
    const MpvObject *mpvObject = mainWindow->mpvObject();
    connect(mpvObject, &MpvObject::fileSizeChanged,
            mpcHcServer, &MpcHcServer::setFileSize);

    // playbackManager -> mpcHcServer
    connect(playbackManager, &PlaybackManager::timeChanged,
            mpcHcServer, &MpcHcServer::setFileTime);
    connect(playbackManager, &PlaybackManager::titleChanged,
            mpcHcServer, &MpcHcServer::setMediaTitle);
    connect(playbackManager, &PlaybackManager::nowPlayingChanged,
            mpcHcServer, [this](QUrl itemUrl, QUuid listUuid, QUuid itemUuid,
                                              bool clickedInPlaylist) {
            Q_UNUSED(listUuid);
            Q_UNUSED(itemUuid);
            Q_UNUSED(clickedInPlaylist);
            mpcHcServer->setNowPlaying(itemUrl); });
    connect(playbackManager, &PlaybackManager::playbackSpeedChanged,
            mpcHcServer, &MpcHcServer::setPlaybackRate);
    connect(playbackManager, &PlaybackManager::stateChanged,
            mpcHcServer, &MpcHcServer::setPlaybackState);
}

bool Flow::isNvidiaGPU()
{
    bool foundNvidia = false;
    QProcess process;
    Logger::log(logModule, "Display devices:");
    process.start("lspci", QStringList() << "-v" << "-d" << "::0300");
    process.waitForFinished(1000);
    QString result = process.readAllStandardOutput();
    for (const QString &line : result.split('\n'))
        LogStream(logModule) << line;

    if (result.contains("nvidia", Qt::CaseSensitivity::CaseInsensitive)) {
        foundNvidia = true;
    }
    return foundNvidia;
}

void Flow::showVersionInfo()
{
    QString package = "Native build";
    if (qEnvironmentVariableIsSet("FLATPAK_ID")) {
        package = "Flatpak";
        mainWindow->setRemoveFileNotRecycle();
    }
    else if (qEnvironmentVariableIsSet("APPIMAGE"))
        package = "AppImage";
    else if (qEnvironmentVariableIsSet("SNAP"))
        package = "Snap";

    static constexpr char spaces[] = "               ";
    QString logMessages = "Version information:\n";
    logMessages += (QString) spaces + "OS: " + QSysInfo::productType() + " " + QSysInfo::productVersion() + "\n";
    logMessages += (QString) spaces + "Qt: " + (QString) QT_VERSION_STR + "\n";
    logMessages += (QString) spaces + mainWindow->mpvObject()->mpvVersion() + "\n";
    logMessages += (QString) spaces + "ffmpeg " + mainWindow->mpvObject()->ffmpegVersion() + "\n";
    logMessages += (QString) spaces + "mpc-qt: " + (QString) MPCQT_VERSION_STR;
    logMessages += " " + (QString) __DATE__ + " " + __TIME__  + "\n";
    logMessages += (QString) spaces + "Package: " + package;
    LogStream(logModule) << logMessages;
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
    QString basename;
    if (nowPlaying.isLocalFile())
        basename = QFileInfo(nowPlaying.fileName()).completeBaseName();
    else
        basename = Platform::sanitizedFilename(playbackManager->nowPlayingTitle());

    // FIXME: Use parseFormatEx?
    QString fileName = Helpers::parseFormat(screenshotTemplate, basename, tracks, subs, playTime, 0, 0);

    // Filesystems typically support 255 characters for filename length
    int maxLength = 255 - (screenshotFormat.length() + 1);
    if (fileName.length() >= maxLength) {
        basename.truncate(basename.length() - (fileName.length() - maxLength));
        fileName = Helpers::parseFormat(screenshotTemplate, basename, tracks, subs, playTime, 0, 0);
    }
    QString filePath = lastScreenshotDir;
    if (filePath.isEmpty())
        filePath = screenshotDirectory;
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
    windowManager.clearJson();
    if (rememberPanels)
        windowManager.saveDocks(mainWindow->dockHost());
    if (rememberWindowGeometry) {
        windowManager.saveWindow(settingsWindow);
        windowManager.saveWindow(propertiesWindow);
        windowManager.saveWindow(logWindow);
        windowManager.saveWindow(libraryWindow);
    }
    windowManager.saveAppWindow(mainWindow, rememberWindowGeometry, rememberPanels);
    return windowManager.json();
}

void Flow::restoreWindows_v2(const QVariantMap &geometryMap)
{
    CliInfo cliInfo { cliPos, cliSize, validCliPos, validCliSize };

    windowManager.setJson(geometryMap);
    windowManager.restoreDocks(mainWindow->dockHost(), { mainWindow->playlistWindow() });
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

void Flow::loadPlaylistsBackup()
{
    if (playlistsBackupLoaded)
        return;
    auto backup = cliNoFiles ? QVariantList() : storage.readVList(filePlaylistsBackup);
    PlaylistCollection::getBackup()->fromVList(backup);
    libraryWindow->refreshLibrary();
    playlistsBackupLoaded = true;
}

void Flow::mainwindow_instanceShouldQuit()
{
    endProgram();
}

void Flow::mainwindow_fullscreenHideControls(bool hide)
{
    this->settings["fullscreenHideControls"] = hide;
    mainWindow->setControlsInFullscreen(hide, this->settings.value("fullscreenShowWhen").toInt(), \
        this->settings.value("fullscreenShowWhenDuration").toInt(), false);
}

void Flow::mainwindow_repeatAfter()
{
    repeatAfter = true;
}

void Flow::mainwindow_recentOpened(const TrackInfo &track)
{
    updateRecentPosition(false);
    if (repeatAfter)
        mainWindow->setActionPlayLoopUse();
    // attempt to play the playlist item if possible, otherwise act like it
    // is a new file
    QUrl old = mainWindow->playlistWindow()->getUrlOf(track.list, track.item);
    if (!old.isEmpty())
        playbackManager->playItem(track.list, track.item);
    else
        playbackManager->openFile(track.url);
}

void Flow::mainwindow_recentClear()
{
    recentFiles.clear();
    mainWindow->setRecentDocuments(recentFiles);
}

void Flow::mainwindow_takeImage(Helpers::ScreenshotRender render)
{
    static auto options = QFileDialog::Options();
#ifdef Q_OS_MAC
    options = QFileDialog::DontUseNativeDialog;
#endif
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

    QString picFile = pictureTemplate(Helpers::DisabledAudio, subRender);
    if (!lastScreenshotDir.isEmpty())
        picFile = lastScreenshotDir + "/" +
            QFileInfo(QUrl::fromLocalFile(picFile).toLocalFile()).fileName();

    picFile = QFileDialog::getSaveFileName(this->mainWindow, tr("Save Image"),
                                           picFile, "", nullptr, options);

    // Only remember the screenshot dir if the user picked a different one from the default
    if (!lastScreenshotDir.isEmpty() ||
            QFileInfo(QUrl::fromLocalFile(picFile).toLocalFile()).path() != lastScreenshotDir)
        lastScreenshotDir = QFileInfo(QUrl::fromLocalFile(picFile).toLocalFile()).path();

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

void Flow::manager_stateChanged(PlaybackManager::PlaybackState state)
{
    // Do nothing if we don't have to
    if (!manipulateScreensaver)
        return;

    // If inhibiting the screensaver is switched off, or we just entered the
    // stopped or paused state, unihibit the screensaver
    if (!inhibitScreensaver || state == PlaybackManager::StoppedState
                            || state == PlaybackManager::PausedState) {
        screenSaver->uninhibitSaver();
        return;
    }

    // Else: inhibit the screensaver because we're not stopped
    screenSaver->inhibitSaver(tr("Playing Media"));
}

void Flow::manager_fileClosed()
{
    mainWindow->disableChaptersMenus();
}

void Flow::manager_subtitlesVisible(bool visible)
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

void Flow::manager_playingNextFile()
{
    // Save the position just before opening the next file
    Logger::log(logModule, "manager_playingNextFile");
    updateRecentPosition(false);
}

void Flow::manager_playLengthChanged() {
    Logger::log(logModule, "manager_playLengthChanged");
    updateRecentPosition(false);
    if (repeatAfter)
        mainWindow->setActionPlayLoopUse();
}

void Flow::manager_openingNewFile()
{
    updateRecentPosition(false);
    if (repeatAfter)
        mainWindow->setActionPlayLoopUse();
}

void Flow::manager_aboutToStartPlayingFile()
{
    if (firstFile) {
        firstFile = false;
        mainWindow->fixMpvwSize();
    }
    if (rememberFilePosition)
        updateRecentPosition(false);
}

void Flow::manager_startedPlayingFile(QUrl url)
{
    Logger::log(logModule, "manager_startedPlayingFile");
    if (rememberFilePosition) {
        // Check if there's a position saved in recents for this file
        foreach (TrackInfo track, recentFiles) {
            if (track.url == url) {
                playbackManager->navigateToTime(track.position);
                playbackManager->restoreVideoTrack(track.videoTrack);
                playbackManager->restoreAudioTrack(track.audioTrack);
                playbackManager->restoreSubtitleTrack(track.subtitleTrack);
                break;
            }
        }
    }
}
void Flow::manager_stoppedPlaying()
{
    // Reset the position on stop
    updateRecentPosition(true);
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
    Logger::log(logModule, "settingswindow_settingsData");
}

void Flow::settingswindow_inhibitScreensaver(bool yes)
{
    // Remember inhibit screensaver and renotify ourselves of the current
    // playback state (2022-03: which optionally re/uninhibits the
    // screensaver)
    this->inhibitScreensaver = yes;
    manager_stateChanged(playbackManager->playbackState());
}

void Flow::settingswindow_rememberHistory(bool yes, bool onlyVideos)
{
    // Remember our preference to the list of recent files
    this->rememberHistory = yes;
    this->rememberHistoryOnlyForVideos = onlyVideos;
    if (!yes) {
        recentFiles = QList<TrackInfo>();
        mainWindow->setRecentDocuments(recentFiles);
    }
}

void Flow::settingswindow_rememberFilePosition(bool yes)
{
    // Remember our preference to restore the position when opening a file
    this->rememberFilePosition = yes;
}

void Flow::settingswindow_rememberQuickPlaylist(bool yes)
{
    // Remember our preference to restore the content of the Quick Playlist
    this->rememberQuickPlaylist = yes;
}

void Flow::settingswindow_rememberWindowGeometry(bool yes)
{
    // Remember our preference to process the window geometry when we exit
    this->rememberWindowGeometry = yes;
}

void Flow::settingswindow_rememberPanels(bool yes)
{
    // Remember our preference to save the panels state when we exit
    this->rememberPanels = yes;
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
        originalApplicationStyle = QApplication::style()->name();
    }

    bool wasFusion = QApplication::style()->name() == "Fusion";
    if (!yes && wasFusion)
        QApplication::setStyle(originalApplicationStyle);
    if (yes && !wasFusion)
        QApplication::setStyle(QStyleFactory::create("Fusion"));
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

void Flow::favoriteswindow_favoriteTracksCancel()
{
    // Reset the favoritesWindow files and stream to previous ones
    favoritesWindow->setFiles(favoriteFiles);
    favoritesWindow->setStreams(favoriteStreams);
}

// Update the position of the current file
void Flow::updateRecentPosition(bool resetPosition)
{
    Logger::log(logModule, "updateRecentPosition");
    if (!rememberHistory)
        return;
    QUrl url;
    QUuid listUuid;
    QUuid itemUuid;
    QString title;
    double length;
    double position;
    int64_t videoTrack;
    int64_t audioTrack;
    int64_t subtitleTrack;
    bool hasVideo;
    if (playbackManager->eofReached())
        resetPosition = true;
    playbackManager->getCurrentTrackInfo(url, listUuid, itemUuid, title, length, position,
                                         videoTrack, audioTrack, subtitleTrack, hasVideo);
    if (!itemUuid.isNull() && !url.isEmpty() && (hasVideo || !rememberHistoryOnlyForVideos))
        updateRecents(url, listUuid, itemUuid, title, length, resetPosition ? 0 : position,
                      videoTrack, audioTrack, subtitleTrack);
}

// Update the Recents list
void Flow::updateRecents(QUrl url, QUuid listUuid, QUuid itemUuid, QString title, double length,
                         double position, int64_t videoTrack, int64_t audioTrack, int64_t subtitleTrack)
{
    // Insert playing track as the most recent item
    TrackInfo track(url, listUuid, itemUuid, title, length, position,
                    videoTrack, audioTrack, subtitleTrack);
    if (recentFiles.contains(track)) {
        // Remove all prior mention of it
        recentFiles.removeAll(track);
    }
    recentFiles.insert(0, track);

    // Trim the recent file list
    while (recentFiles.size() > (rememberFilePosition ? 1000 : 20))
        recentFiles.removeLast();

    // Notify (2022-03: the main window) that the recents have changed
    mainWindow->setRecentDocuments(recentFiles);
}

void Flow::endProgram()
{
    Logger::log(logModule, "endProgram");
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

