#include <QDebug>
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
#include <QTranslator>
#include <QLibraryInfo>
#include "main.h"
#include "storage.h"
#include "mainwindow.h"
#include "manager.h"
#include "settingswindow.h"
#include "mpvwidget.h"
#include "propertieswindow.h"
#include "platform/resources_paths.h"
#include "platform/unify.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationDomain("cmdrkotori.mpc-qt");
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/images/icon/logo.svg"));

    // Qt sets the locale in the QApplication constructor, but libmpv requires
    // the LC_NUMERIC category to be set to "C", so change it back.
    std::setlocale(LC_NUMERIC, "C");

    // Register the error code type so that signals/slots will work with it
    qRegisterMetaType<MpvErrorCode>("MpvErrorCode");
    qRegisterMetaType<uint64_t>("uint64_t");

    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(),
       QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    a.installTranslator(&qtTranslator);

    QTranslator aTranslator;
    aTranslator.load("mpc-qt_" + QLocale::system().name(), APP_LANG_PATH);
    a.installTranslator(&aTranslator);

    Flow f;
    f.parseArgs();
    f.init();
    if (!f.hasPrevious())
        return f.run();
    else
        return 0;
}

Flow::Flow(QObject *owner) :
    QObject(owner), server(NULL), mpvServer(NULL), mainWindow(NULL),
    playbackManager(NULL), settingsWindow(NULL), propertiesWindow(NULL)
{
}

Flow::~Flow()
{
    if (server) {
        delete server;
        server = NULL;
    }
    if (mpvServer) {
        delete mpvServer;
        mpvServer = NULL;
    }
    if (mainWindow) {
        storage.writeVList("playlists", mainWindow->playlistWindow()->tabsToVList());
        delete mainWindow;
        mainWindow = NULL;
    }
    if (playbackManager) {
        delete playbackManager;
        playbackManager = NULL;
    }
    if (settingsWindow) {
        delete settingsWindow;
        settingsWindow = NULL;
    }
    if (propertiesWindow)  {
        delete propertiesWindow;
        propertiesWindow = NULL;
    }
    screenSaver.uninhibitSaver();
}

void Flow::parseArgs()
{
    QCommandLineParser parser;
    parser.setApplicationDescription(tr("Media Player Classic Qute Theater"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption sizeOpt("size", tr("Main window size."), "w,h");
    QCommandLineOption posOpt("pos", tr("Main window position."), "x,y");

    parser.addOption(sizeOpt);
    parser.addOption(posOpt);
    parser.addPositionalArgument("urls", tr("URLs to open, optionally."), "[urls...]");

    parser.process(QCoreApplication::arguments());

    validCliSize = parser.isSet(sizeOpt) && Helpers::sizeFromString(cliSize, parser.value(sizeOpt));
    validCliPos = parser.isSet(posOpt) && Helpers::pointFromString(cliPos, parser.value(posOpt));

    customFiles = parser.positionalArguments();
}

void Flow::init() {
    mainWindow = new MainWindow();
    playbackManager = new PlaybackManager(this);
    playbackManager->setMpvWidget(mainWindow->mpvWidget(), true);
    playbackManager->setPlaylistWindow(mainWindow->playlistWindow());
    settingsWindow = new SettingsWindow();
    settingsWindow->setWindowModality(Qt::WindowModal);
    propertiesWindow = new PropertiesWindow();

    server = new MpcQtServer(mainWindow, playbackManager, this);
    hasPrevious_ = server->sendPayload(makePayload());
    if (hasPrevious_)
        return;
    mpvServer = new MpvServer(playbackManager, mainWindow->mpvWidget(), this);

    inhibitScreensaver = false;
    QSet<QScreenSaver::Ability> actualPowers = screenSaver.abilities();
    mainWindow->setScreensaverAbilities(actualPowers);
    QSet<QScreenSaver::Ability> desiredPowers;
    desiredPowers << QScreenSaver::Inhibit << QScreenSaver::Uninhibit;
    manipulateScreensaver = actualPowers.contains(desiredPowers);
    settingsWindow->setScreensaverDisablingEnabled(manipulateScreensaver);

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

    // playlistwindow -> mainwindow
    connect(mainWindow->playlistWindow(), &PlaylistWindow::visibilityChanged,
            mainWindow, &MainWindow::setPlaylistVisibleState);
    connect(mainWindow->playlistWindow(), &PlaylistWindow::quickQueueMode,
            mainWindow, &MainWindow::setPlaylistQuickQueueMode);

    // mainwindow -> playlistwindow
    connect(mainWindow, &MainWindow::playCurrentItemRequested,
            mainWindow->playlistWindow(), &PlaylistWindow::playCurrentItem);

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
    connect(settingsWindow, &SettingsWindow::timeTooltip,
            mainWindow, &MainWindow::setTimeTooltip);

    // settings -> mpvwidget
    auto mpvw = mainWindow->mpvWidget();
    connect(settingsWindow, &SettingsWindow::logoSource,
            mpvw, &MpvWidget::setLogoUrl);
    connect(settingsWindow, &SettingsWindow::volume,
            mpvw, &MpvWidget::setVolume);
    connect(settingsWindow, &SettingsWindow::option,
            mpvw, &MpvWidget::setCachedMpvOption);
    connect(settingsWindow, &SettingsWindow::clientDebuggingMessages,
            mpvw, &MpvWidget::setClientDebuggingMessages);
    connect(settingsWindow, &SettingsWindow::mpvLogLevel,
            mpvw, &MpvWidget::setMpvLogLevel);

    // mpvwidget -> settings
    connect(mpvw, &MpvWidget::audioDeviceList,
            settingsWindow, &SettingsWindow::setAudioDevices);

    // mpvwidget -> properties
    connect(mpvw, &MpvWidget::fileNameChanged,
            propertiesWindow, &PropertiesWindow::setFileName);
    connect(mpvw, &MpvWidget::fileFormatChanged,
            propertiesWindow, &PropertiesWindow::setFileFormat);
    connect(mpvw, &MpvWidget::fileSizeChanged,
            propertiesWindow, &PropertiesWindow::setFileSize);
    connect(mpvw, &MpvWidget::playLengthChanged,
            propertiesWindow, &PropertiesWindow::setMediaLength);
    connect(mpvw, &MpvWidget::videoSizeChanged,
            propertiesWindow, &PropertiesWindow::setVideoSize);
    connect(mpvw, &MpvWidget::fileCreationTimeChanged,
            propertiesWindow, &PropertiesWindow::setFileCreationTime);
    connect(mpvw, &MpvWidget::tracksChanged,
            propertiesWindow, &PropertiesWindow::setTracks);
    connect(mpvw, &MpvWidget::mediaTitleChanged,
            propertiesWindow, &PropertiesWindow::setMediaTitle);
    connect(mpvw, &MpvWidget::filePathChanged,
            propertiesWindow, &PropertiesWindow::setFilePath);
    connect(mpvw, &MpvWidget::metaDataChanged,
            propertiesWindow, &PropertiesWindow::setMetaData);
    connect(mpvw, &MpvWidget::chaptersChanged,
            propertiesWindow, &PropertiesWindow::setChapters);

    // settings -> playlistWindow
    connect(settingsWindow, &SettingsWindow::playlistFormat,
            mainWindow->playlistWindow(), &PlaylistWindow::setDisplayFormatSpecifier);

    // settings -> manager
    connect(settingsWindow, &SettingsWindow::playbackForever,
            playbackManager, &PlaybackManager::setPlaybackForever);
    connect(settingsWindow, &SettingsWindow::playbackPlayTimes,
            playbackManager, &PlaybackManager::setPlaybackPlayTimes);

    // manager -> settings
    connect(playbackManager, &PlaybackManager::playerSettingsRequested,
            settingsWindow, &SettingsWindow::sendSignals);

    // mainwindow -> properties
    connect(mainWindow, &MainWindow::showFileProperties,
            propertiesWindow, &QWidget::show);

    // mainwindow -> this
    connect(mainWindow, &MainWindow::recentOpened,
            this, &Flow::mainwindow_recentOpened);
    connect(mainWindow, &MainWindow::recentClear,
            this, &Flow::mainwindow_recentClear);
    connect(mainWindow, &MainWindow::takeImage,
            this, &Flow::mainwindow_takeImage);
    connect(mainWindow, &MainWindow::takeImageAutomatically,
            this, &Flow::mainwindow_takeImageAutomatically);
    connect(mainWindow, &MainWindow::optionsOpenRequested,
            this, &Flow::mainwindow_optionsOpenRequested);
    connect(mainWindow, &MainWindow::applicationShouldQuit,
            this, &Flow::mainwindow_applicationShouldQuit);

    // manager -> this
    connect(playbackManager, &PlaybackManager::nowPlayingChanged,
            this, &Flow::manager_nowPlayingChanged);
    connect(playbackManager, &PlaybackManager::stateChanged,
            this, &Flow::manager_stateChanged);
    connect(playbackManager, &PlaybackManager::instanceShouldClose,
            this, &Flow::mainwindow_applicationShouldQuit);

    // settings -> this
    connect(settingsWindow, &SettingsWindow::settingsData,
            this, &Flow::settingswindow_settingsData);
    connect(settingsWindow, &SettingsWindow::keyMapData,
            this, &Flow::settingswindow_keymapData);
    connect(settingsWindow, &SettingsWindow::inhibitScreensaver,
            this, &Flow::settingswindow_inhibitScreensaver);
    connect(settingsWindow, &SettingsWindow::rememberWindowGeometry,
            this, &Flow::settingswindow_rememberWindowGeometry);
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
            &screenSaver, &QScreenSaver::hibernateSystem);
    connect(playbackManager, &PlaybackManager::systemShouldLock,
            &screenSaver, &QScreenSaver::lockScreen);
    connect(playbackManager, &PlaybackManager::systemShouldLogOff,
            &screenSaver, &QScreenSaver::logOff);
    connect(playbackManager, &PlaybackManager::systemShouldShutdown,
            &screenSaver, &QScreenSaver::shutdownSystem);
    connect(playbackManager, &PlaybackManager::systemShouldStandby,
            &screenSaver, &QScreenSaver::suspendSystem);

    // this -> mainwindow
    connect(this, &Flow::recentFilesChanged,
            mainWindow, &MainWindow::setRecentDocuments);

    // this -> this
    connect(this, &Flow::windowsRestored,
            this, &Flow::self_windowsRestored);

    // update player framework
    settingsWindow->takeActions(mainWindow->editableActions());
    recentFromVList(storage.readVList("recent"));
    mainWindow->setRecentDocuments(recentFiles);
    settings = storage.readVMap("settings");
    keyMap = storage.readVMap("keys");
    settingsWindow->setAudioDevices(mpvw->audioDevices());
    settingsWindow->takeSettings(settings);
    settingsWindow->setMouseMapDefaults(mainWindow->mouseMapDefaults());
    settingsWindow->takeKeyMap(keyMap);
    settingsWindow->setServerName(server->fullServerName());
    settingsWindow->sendSignals();

    // Push all our windows on to the same (moused) screen... similar code is
    // in mainwindow.cpp.  Prevents jumping screens on multiple screens for
    // whatever underlying reason.
    QDesktopWidget *desktop = qApp->desktop();
    QRect available = desktop->availableGeometry(desktop->screenNumber(QCursor::pos()));
    settingsWindow->setGeometry(QStyle::alignedRect(Qt::LeftToRight,
                                                    Qt::AlignCenter,
                                                    settingsWindow->size(),
                                                    available));
    mainWindow->setGeometry(QStyle::alignedRect(Qt::LeftToRight,
                                                    Qt::AlignCenter,
                                                    mainWindow->size(),
                                                    available));
}

int Flow::run()
{
    mainWindow->playlistWindow()->tabsFromVList(storage.readVList("playlists"));
    QTimer::singleShot(50, this, [this]() {
        // wait for the internal geometry to update, then perform a resize
        restoreWindows(storage.readVMap("geometry"));
    });
    return qApp->exec();
}

bool Flow::hasPrevious()
{
    return hasPrevious_;
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
    double playTime = mainWindow->mpvWidget()->playTime();
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
    QVariantList l;
    for (const TrackInfo &track : recentFiles)
        l.append(track.toVMap());
    return l;
}

void Flow::recentFromVList(const QVariantList &list)
{
    recentFiles.clear();
    for (const QVariant &item : list) {
        TrackInfo t;
        t.fromVMap(item.toMap());
        recentFiles.append(t);
    }
}

QVariantMap Flow::saveWindows()
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
        }
    };
}

void Flow::restoreWindows(const QVariantMap &geometryMap)
{
    // fetch defaults and overwrite them with saved state
    QVariantMap map = saveWindows();
    QMapIterator<QString, QVariant> i(geometryMap);
    while (i.hasNext()) {
        i.next();
        map.insert(i.key(), i.value());
    }

    QVariantMap mainMap = map["mainWindow"].toMap();
    QVariantMap mpvHostMap = map["mpvHost"].toMap();
    QVariantMap playlistMap = map["playlistWindow"].toMap();
    QVariantMap settingsMap = map["settingsWindow"].toMap();
    QVariantMap propertiesMap = map["propertiesWindow"].toMap();
    QRect geometry;
    bool restoreGeometry = rememberWindowGeometry
            && mainMap.contains("geometry")
            && playlistMap.contains("geometry")
            && settingsMap.contains("geometry")
            && propertiesMap.contains("geometry");

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
    mainWindow->unfreezeWindow();
    showWindows(mainMap);

    // restore settings and properties window
    geometry = Helpers::vmapToRect(settingsMap["geometry"].toMap());
    settingsWindow->setGeometry(geometry);
    geometry = Helpers::vmapToRect(propertiesMap["geometry"].toMap());
    propertiesWindow->setGeometry(geometry);
}

void Flow::showWindows(const QVariantMap &mainWindowMap)
{
    mainWindow->show();
    mainWindow->raise();
    if (mainWindowMap.contains("state"))
        mainWindow->setState(mainWindowMap["state"].toMap());

    QTimer::singleShot(50, this, &Flow::windowsRestored);
}

void Flow::self_windowsRestored()
{
    if (!hasPrevious_)
        server->fakePayload(makePayload());
}

void Flow::mainwindow_applicationShouldQuit()
{
    storage.writeVMap("settings", settings);
    storage.writeVMap("keys", keyMap);
    storage.writeVList("recent", recentToVList());
    storage.writeVMap("geometry", saveWindows());
    qApp->quit();
}

void Flow::mainwindow_recentOpened(const TrackInfo &track)
{
    // attempt to play the old one if possible, otherwise pretend it's new
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
    QString fmt("%1/mpc-qt_%2");
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString tempFile = QString(fmt).arg(tempDir, QUuid::createUuid().toString());
    mainWindow->mpvWidget()->screenshot(tempFile, render);
    tempFile += "." + screenshotFormat;

    bool subsVisible = render != Helpers::VideoRender;
    QString fileName = pictureTemplate(Helpers::DisabledAudio,
                                       subsVisible ? Helpers::SubtitlesPresent
                                                   : Helpers::SubtitlesDisabled);
    QString picFile;
    picFile = QFileDialog::getSaveFileName(this->mainWindow, tr("Save Image"),
                                           fileName);
    if (picFile.isEmpty()) {
        QFile(tempFile).remove();
        return;
    }
    QFileInfo qfi(picFile);
    QString dest = qfi.absolutePath() + "/" + qfi.completeBaseName();
    QFile(tempFile).copy(dest);
}

void Flow::mainwindow_takeImageAutomatically(Helpers::ScreenshotRender render)
{
    bool subsVisible = render != Helpers::VideoRender;
    QString fileName = pictureTemplate(Helpers::DisabledAudio,
                                       subsVisible ? Helpers::SubtitlesPresent
                                                   : Helpers::SubtitlesDisabled);
    mainWindow->mpvWidget()->screenshot(fileName, render);
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
    TrackInfo track(url, listUuid, itemUuid);
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
        screenSaver.uninhibitSaver();
        return;
    }
    screenSaver.inhibitSaver(tr("Playing Media"));
}

void Flow::settingswindow_settingsData(const QVariantMap &settings)
{
    this->settings = settings;
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

void Flow::importPlaylist(QString fname)
{
    //CHECKME: addSimplePlaylist should be a slot?
    mainWindow->playlistWindow()->addSimplePlaylist(storage.readM3U(fname));
}

void Flow::exportPlaylist(QString fname, QStringList items)
{
    storage.writeM3U(fname, items);
}

