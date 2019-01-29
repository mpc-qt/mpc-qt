#include <clocale>
#include <QApplication>
#include <QDesktopWidget>
#include <QLocalSocket>
#include <QFileDialog>
#include <QDir>
#include <QStandardPaths>
#include <QUuid>
#include <QJsonDocument>
#include <QTimer>
#include <QCommandLineParser>
#include <QSurfaceFormat>
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
#include "ipcmpris.h"
#include "platform/unify.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationDomain("cmdrkotori.mpc-qt");
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

    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(),
       QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    a.installTranslator(&qtTranslator);

    QTranslator aTranslator;
    aTranslator.load("mpc-qt_" + QLocale::system().name(),
                     Platform::resourcesPath() + "/translations/");
    a.installTranslator(&aTranslator);

#ifndef MPCQT_VERSION_STR
#define MPCQT_VERSION_STR MainWindow::tr("Development Build")
#endif
    QCoreApplication::setApplicationVersion(MPCQT_VERSION_STR);

    Flow f;
    f.parseArgs();
    f.detectMode();
    if (f.earlyQuit())
        return 0;
    f.init();
    return f.run();
}

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
    if (mainWindow) {
        if (programMode == PrimaryMode)
            storage.writeVList("playlists", mainWindow->playlistWindow()->tabsToVList());
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

    logThread = new QThread();
    logThread->start();
    Logger *logger = Logger::singleton();
    logger->moveToThread(logThread);
    connect(logThread, &QThread::finished,
            logger, &QObject::deleteLater);

    mainWindow = new MainWindow();
    playbackManager = new PlaybackManager(this);
    playbackManager->setMpvObject(mainWindow->mpvObject(), true);
    playbackManager->setPlaylistWindow(mainWindow->playlistWindow());
    settingsWindow = new SettingsWindow();
    settingsWindow->setWindowModality(Qt::WindowModal);
    propertiesWindow = new PropertiesWindow();
    favoritesWindow = new FavoritesWindow();
    logWindow = new LogWindow();
    thumbnailerWindow = new ThumbnailerWindow();

    server = new MpcQtServer(mainWindow, playbackManager, this);
    server->setMainWindow(mainWindow);
    server->setPlaybackManger(playbackManager);

    mpvServer = new MpvServer(this);
    mpvServer->setPlaybackManger(playbackManager);
    mpvServer->setMpvObject(mainWindow->mpvObject());

    inhibitScreensaver = false;
    screenSaver = Platform::screenSaver();
    QSet<ScreenSaver::Ability> actualPowers = screenSaver->abilities();
    mainWindow->setScreensaverAbilities(actualPowers);
    QSet<ScreenSaver::Ability> desiredPowers;
    desiredPowers << ScreenSaver::Inhibit << ScreenSaver::Uninhibit;
    manipulateScreensaver = actualPowers.contains(desiredPowers);
    settingsWindow->setScreensaverDisablingEnabled(manipulateScreensaver);

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

    // manager -> settings
    connect(playbackManager, &PlaybackManager::playerSettingsRequested,
            settingsWindow, &SettingsWindow::sendSignals);

    if (programMode == PrimaryMode)
        setupMpris();

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

    if (programMode == PrimaryMode) {
        server->listen();
        mpvServer->listen();
    }
    settingsWindow->setServerName(server->fullServerName());

    mainWindow->setFreestanding(programMode == FreestandingMode);
    settingsWindow->setFreestanding(programMode == FreestandingMode);
}

int Flow::run()
{
    auto playlist = cliNoFiles ? QVariantList() : storage.readVList("playlists");
    auto geometry = cliNoConfig ? QVariantMap() : storage.readVMap("geometry");
    mainWindow->playlistWindow()->tabsFromVList(playlist);
    restoreWindows(geometry);
    return qApp->exec();
}

bool Flow::earlyQuit()
{
    return programMode == EarlyQuitMode;
}

void Flow::readConfig()
{
    if (!cliNoConfig) {
        settings = storage.readVMap("settings");
        keyMap = storage.readVMap("keys");
    }

    if (!cliNoFiles) {
        QVariantMap favoriteMap = storage.readVMap("favorites");
        favoriteFiles = TrackInfo::tracksFromVList(favoriteMap.value("files").toList());
        favoriteStreams = TrackInfo::tracksFromVList(favoriteMap.value("streams").toList());
        recentFiles = TrackInfo::tracksFromVList(storage.readVList("recent"));
    }
}

void Flow::writeConfig(bool onlySettings)
{
    if (programMode != PrimaryMode)
        return;

    storage.writeVMap("settings", settings);
    if (onlySettings)
        return;

    storage.writeVMap("keys", keyMap);
    storage.writeVList("recent", recentToVList());
    storage.writeVMap("favorites", favoritesToVMap());
    storage.writeVMap("geometry", windowsToVMap());
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
}

void Flow::setupManagerConnections()
{
    // manager -> favorites
    connect(playbackManager, &PlaybackManager::currentTrackInfo,
            favoritesWindow, &FavoritesWindow::addTrack);
}

void Flow::setupSettingsConnections()
{

    // mainwindow -> settings
    connect(mainWindow, &MainWindow::volumeChanged,
            settingsWindow, &SettingsWindow::setVolume);
    connect(mainWindow, &MainWindow::zoomPresetChanged,
            settingsWindow, &SettingsWindow::setZoomPreset);

    // settings -> mainwindow
    connect(settingsWindow, &SettingsWindow::mouseWindowedMap,
            mainWindow, &MainWindow::setWindowedMouseMap);
    connect(settingsWindow, &SettingsWindow::mouseFullscreenMap,
            mainWindow, &MainWindow::setFullscreenMouseMap);
    connect(settingsWindow, &SettingsWindow::iconTheme,
            mainWindow, &MainWindow::setIconTheme);
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

QByteArray Flow::makePayload() const
{
    QVariantMap map({
        {"command", QVariant("playFiles")},
        {"directory", QVariant(QDir::currentPath())},
        {"files", QVariant(customFiles)}
    });
    return QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
}

QString Flow::pictureTemplate(Helpers::DisabledTrack tracks, Helpers::Subtitles subs) const
{
    double playTime = mainWindow->mpvObject()->playTime();
    QUrl nowPlaying = playbackManager->nowPlaying();
    QString basename = QFileInfo(nowPlaying.toDisplayString().split('/').last())
                       .completeBaseName();

    QString fileName = Helpers::parseFormat(screenshotTemplate, basename,
                                            tracks,
                                            subs,
                                            playTime, 0, 0);
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
        { "files", TrackInfo::tracksToVList(favoriteFiles) },
        { "streams", TrackInfo::tracksToVList(favoriteStreams) }
    };
}

QVariantMap Flow::windowsToVMap()
{
    return QVariantMap {
        {
            "mainWindow", QVariantMap {
                { "geometry", Helpers::rectToVmap(QRect(mainWindow->geometry().topLeft(),
                                                        mainWindow->size())) },
                { "state", mainWindow->state() }
            }
        },
        {
            "mpvHost", QVariantMap {
                { "qtState", QString(mainWindow->mpvHost()->saveState().toBase64()) }
            }
        },
        {
            "playlistWindow", QVariantMap {
                { "geometry", Helpers::rectToVmap(mainWindow->playlistWindow()->window()->geometry()) },
                { "floating", mainWindow->playlistWindow()->isFloating() }
            }
        },
        {
            "settingsWindow", QVariantMap {
                { "geometry", Helpers::rectToVmap(settingsWindow->geometry()) }
            }
        },
        {
            "propertiesWindow", QVariantMap {
                { "geometry", Helpers::rectToVmap(propertiesWindow->geometry()) }
            }
        },
        {
            "logWindow", QVariantMap {
                { "geometry", Helpers::rectToVmap(logWindow->geometry()) }
            }
        }
    };
}

void Flow::restoreWindows(const QVariantMap &geometryMap)
{
    QVariantMap mainMap = geometryMap["mainWindow"].toMap();
    QVariantMap mpvHostMap = geometryMap["mpvHost"].toMap();
    QVariantMap playlistMap = geometryMap["playlistWindow"].toMap();
    QVariantMap settingsMap = geometryMap["settingsWindow"].toMap();
    QVariantMap propertiesMap = geometryMap["propertiesWindow"].toMap();
    QVariantMap logMap = geometryMap["logWindow"].toMap();
    QRect geometry;
    QDesktopWidget desktop;
    bool restoreGeometry = rememberWindowGeometry
            && mainMap.contains("geometry")
            && playlistMap.contains("geometry")
            && settingsMap.contains("geometry")
            && propertiesMap.contains("geometry")
            && logMap.contains("geometry");

    if (restoreGeometry && playlistMap["floating"].toBool()) {
        // the playlist window starts off floating, so restore it
        mainWindow->playlistWindow()->setFloating(true);
        geometry = Helpers::vmapToRect(playlistMap["geometry"].toMap());
        mainWindow->playlistWindow()->window()->setGeometry(geometry);
    } else if (restoreGeometry && mpvHostMap.contains("qtState")) {
        // the playlist window is docked, so we place it back where it was
        QByteArray encoded = mpvHostMap["qtState"].toString().toLocal8Bit();
        QByteArray state = QByteArray::fromBase64(encoded);
        mainWindow->mpvHost()->restoreState(state);
        mainWindow->mpvHost()->restoreDockWidget(mainWindow->playlistWindow());
    }

    // restore main window geometry and override it if requested
    geometry = Helpers::vmapToRect(mainMap["geometry"].toMap());
    QPoint desiredPlace = geometry.topLeft();
    QSize desiredSize = geometry.size();
    bool checkMainWindow = !geometryMap.contains("mainWindow")
            || geometry.isEmpty() || !restoreGeometry;

    if (checkMainWindow)
        desiredSize = mainWindow->desirableSize(true);
    if (validCliSize)
        desiredSize = cliSize;

    if (checkMainWindow)
        desiredPlace = mainWindow->desirablePosition(desiredSize, true);
    if (validCliPos)
        desiredPlace = cliPos;

    mainWindow->setGeometry(QRect(desiredPlace, desiredSize));

    // helper: fetch geometry from map and center if not exists
    auto applyVariantToWindow = [&](QWidget *window, const QVariantMap &map) {
        geometry = Helpers::vmapToRect(map["geometry"].toMap());
        if (geometry.isEmpty()) {
            int mouseScreenNumber = desktop.screenNumber(QCursor::pos());
            QRect available = desktop.availableGeometry(mouseScreenNumber);
            geometry = QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter,
                                           window->size(), available);
        }
        window->setGeometry(geometry);
    };
    // restore settings and properties window
    applyVariantToWindow(settingsWindow, settingsMap);
    applyVariantToWindow(propertiesWindow, propertiesMap);
    applyVariantToWindow(logWindow, logMap);
    showWindows(mainMap);
}

void Flow::showWindows(const QVariantMap &mainWindowMap)
{
    mainWindow->show();
    mainWindow->raise();
    if (mainWindowMap.contains("state"))
        mainWindow->setState(mainWindowMap["state"].toMap());
    mainWindow->unfreezeWindow();
    QTimer::singleShot(50, this, &Flow::windowsRestored);
}

void Flow::self_windowsRestored()
{
    server->fakePayload(makePayload());
}

void Flow::mainwindow_instanceShouldQuit()
{
    // this should eventually be different from endProgram():
    //windows.remove(window);
    //if (windows.isEmpty())
    endProgram();
}

void Flow::mainwindow_recentOpened(const TrackInfo &track)
{
    // attempt to play the old one if possible, otherwise pretend it's new
    QUrl old = mainWindow->playlistWindow()->getUrlOf(track.list, track.item);
    if (!old.isEmpty())
        playbackManager->playItem(track.list, track.item);
    else
        playbackManager->openFile(track.url);
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
    QString fmt("%1/mpc-qt_%2.%3");
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString tempFile = fmt.arg(tempDir, QUuid::createUuid().toString(), screenshotFormat);
    mainWindow->mpvObject()->screenshot(tempFile, render);

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
    settingsWindow->takeSettings(settings);
    settingsWindow->takeKeyMap(keyMap);
    settingsWindow->show();
    settingsWindow->raise();
}

void Flow::manager_nowPlayingChanged(QUrl url, QUuid listUuid, QUuid itemUuid)
{
    TrackInfo track(url, listUuid, itemUuid, QString(), 0, 0);
    if (recentFiles.contains(track)) {
        recentFiles.removeAll(track);
    }
    recentFiles.insert(0, track);
    if (recentFiles.size() > 10)
        recentFiles.removeLast();

    emit recentFilesChanged(recentFiles);
}

void Flow::manager_stateChanged(PlaybackManager::PlaybackState state)
{
    if (!manipulateScreensaver)
        return;

    if (!inhibitScreensaver || state == PlaybackManager::StoppedState) {
        screenSaver->uninhibitSaver();
        return;
    }
    screenSaver->inhibitSaver(tr("Playing Media"));
}

void Flow::manager_subtitlesVisibile(bool visible)
{
    nowPlayingDisplayingSubtitles = visible;
}

void Flow::manager_hasNoSubtitles(bool none)
{
    nowPlayingNoSubtitleTracks = none;
}

void Flow::settingswindow_settingsData(const QVariantMap &settings)
{
    this->settings = settings;
    writeConfig(true);
}

void Flow::settingswindow_inhibitScreensaver(bool yes)
{
    this->inhibitScreensaver = yes;
    manager_stateChanged(playbackManager->playbackState());
}

void Flow::settingswindow_rememberWindowGeometry(bool yes)
{
    this->rememberWindowGeometry = yes;
}

void Flow::settingswindow_keymapData(const QVariantMap &keyMap)
{
    this->keyMap = keyMap;
}

void Flow::settingswindow_mprisIpc(bool enabled)
{
#ifdef QT_DBUS_LIB
    if (!mpris)
        return;

    if (!enabled && mpris->registered()) {
        mpris->unregisterDBus();
    }
    if (enabled && !mpris->registered()) {
        mpris->registerDBus();
    }
#else
    Q_UNUSED(enabled)
#endif
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
    favoriteFiles = files;
    favoriteStreams = streams;
}

void Flow::endProgram()
{
    writeConfig();
    qApp->quit();
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

