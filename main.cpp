#include <QDebug>
#include <clocale>
#include <QApplication>
#include <QFileDialog>
#include <QDir>
#include <QStandardPaths>
#include <QUuid>
#include "main.h"
#include "storage.h"
#include "mainwindow.h"
#include "manager.h"
#include "settingswindow.h"
#include "mpvwidget.h"

int main(int argc, char *argv[])
{
    // When (un)docking windows, some widgets may get transformed into native
    // widgets, causing painting glitches.  Tell Qt that we prefer non-native.
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

    QCoreApplication::setOrganizationDomain("cmdrkotori.mpc-qt");
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/images/bitmaps/icon.png"));

    // Qt sets the locale in the QApplication constructor, but libmpv requires
    // the LC_NUMERIC category to be set to "C", so change it back.
    std::setlocale(LC_NUMERIC, "C");

    // Register the error code type so that signals/slots will work with it
    qRegisterMetaType<MpvErrorCode>("MpvErrorCode");

    Flow f;
    if (!f.hasPrevious())
        return f.run();
    else
        return 0;
}

Flow::Flow(QObject *owner) :
    QObject(owner), process(NULL), mainWindow(NULL), playbackManager(NULL),
    settingsWindow(NULL)
{
    process = new SingleProcess(this);
    if (process->hasPrevious(makePayload())) {
        hasPrevious_ = true;
        return;
    }
    connect(process, SIGNAL(payloadReceived(QStringList)),
            this, SLOT(process_payloadRecieved(QStringList)));

    mainWindow = new MainWindow();
    playbackManager = new PlaybackManager(this);
    playbackManager->setMpvWidget(mainWindow->mpvWidget(), true);
    playbackManager->setPlaylistWindow(mainWindow->playlistWindow());

    settingsWindow = new SettingsWindow();
    settingsWindow->setWindowModality(Qt::WindowModal);
    settings = storage.readVMap("settings");
    settingsWindow->takeSettings(settings);
    settingsWindow->setNnedi3Available(mainWindow->mpvWidget()->nnedi3Available());

    // mainwindow -> manager
    connect(mainWindow, SIGNAL(severalFilesOpened(QList<QUrl>)),
            playbackManager, SLOT(openSeveralFiles(QList<QUrl>)));
    connect(mainWindow, SIGNAL(fileOpened(QUrl)),
            playbackManager, SLOT(openFile(QUrl)));
    connect(mainWindow, &MainWindow::dvdbdOpened,
            playbackManager, &PlaybackManager::playDiscFiles);
    connect(mainWindow, &MainWindow::streamOpened,
            playbackManager, &PlaybackManager::playStream);
    connect(mainWindow, SIGNAL(paused()),
            playbackManager, SLOT(pausePlayer()));
    connect(mainWindow, SIGNAL(unpaused()),
            playbackManager, SLOT(unpausePlayer()));
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

    // mainwindow -> settings
    connect(mainWindow, &MainWindow::zoomPresetChanged,
            settingsWindow, &SettingsWindow::setZoomPreset);

    // settings -> mainwindow
    connect(settingsWindow, &SettingsWindow::zoomPreset,
            mainWindow, &MainWindow::setZoomPreset);

    // settings -> mpvwidget
    auto mpvw = mainWindow->mpvWidget();
    connect(settingsWindow, &SettingsWindow::logoSource,
            mpvw, &MpvWidget::setLogoUrl);
    connect(settingsWindow, &SettingsWindow::voCommandLine,
            mpvw, &MpvWidget::setVOCommandLine);
    connect(settingsWindow, &SettingsWindow::framedropMode,
            mpvw, &MpvWidget::setFramedropMode);
    connect(settingsWindow, &SettingsWindow::decoderDropMode,
            mpvw, &MpvWidget::setDecoderDropMode);
    connect(settingsWindow, &SettingsWindow::displaySyncMode,
            mpvw, &MpvWidget::setDisplaySyncMode);
    connect(settingsWindow, &SettingsWindow::audioDropSize,
            mpvw, &MpvWidget::setAudioDropSize);
    connect(settingsWindow, &SettingsWindow::maximumAudioChange,
            mpvw, &MpvWidget::setMaximumAudioChange);
    connect(settingsWindow, &SettingsWindow::maximumVideoChange,
            mpvw, &MpvWidget::setMaximumVideoChange);
    connect(settingsWindow, &SettingsWindow::subsAreGray,
            mpvw, &MpvWidget::setSubsAreGray);
    connect(settingsWindow, &SettingsWindow::screenshotFormat,
            mpvw, &MpvWidget::setScreenshotFormat);
    connect(settingsWindow, &SettingsWindow::screenshotJpegQuality,
            mpvw, &MpvWidget::setScreenshotJpegQuality);
    connect(settingsWindow, &SettingsWindow::screenshotJpegSmooth,
            mpvw, &MpvWidget::setScreenshotJpegSmooth);
    connect(settingsWindow, &SettingsWindow::screenshotJpegSourceChroma,
            mpvw, &MpvWidget::setScreenshotJpegSourceChroma);
    connect(settingsWindow, &SettingsWindow::screenshotPngCompression,
            mpvw, &MpvWidget::setScreenshotPngCompression);
    connect(settingsWindow, &SettingsWindow::screenshotPngFilter,
            mpvw, &MpvWidget::setScreenshotPngFilter);
    connect(settingsWindow, &SettingsWindow::screenshotPngColorspace,
            mpvw, &MpvWidget::setScreenshotPngColorspace);
    connect(settingsWindow, &SettingsWindow::clientDebuggingMessages,
            mpvw, &MpvWidget::setClientDebuggingMessages);
    connect(settingsWindow, &SettingsWindow::mpvLogLevel,
            mpvw, &MpvWidget::setMpvLogLevel);

    // settings -> playlistWindow
    connect(settingsWindow, &SettingsWindow::playlistFormat,
            mainWindow->playlistWindow(), &PlaylistWindow::setDisplayFormatSpecifier);

    // settings -> manager
    connect(settingsWindow, &SettingsWindow::playbackPlayTimes,
            playbackManager, &PlaybackManager::setPlaybackPlayTimes);

    // manager -> settings
    connect(playbackManager, &PlaybackManager::playerSettingsRequested,
            settingsWindow, &SettingsWindow::sendSignals);

    // mainwindow -> this
    connect(mainWindow, &MainWindow::takeImage,
            this, &Flow::mainwindow_takeImage);
    connect(mainWindow, &MainWindow::takeImageAutomatically,
            this, &Flow::mainwindow_takeImageAutomatically);
    connect(mainWindow, &MainWindow::optionsOpenRequested,
            this, &Flow::mainwindow_optionsOpenRequested);
    connect(mainWindow, &MainWindow::applicationShouldQuit,
            this, &Flow::mainwindow_applicationShouldQuit);

    // settings -> this
    connect(settingsWindow, &SettingsWindow::settingsData,
            this, &Flow::settingswindow_settingsData);
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

    // update player framework
    settingsWindow->sendSignals();
}

Flow::~Flow()
{
    if (process) {
        delete process;
        process = NULL;
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
    storage.writeVMap("settings", settings);
}

int Flow::run()
{
    mainWindow->playlistWindow()->tabsFromVList(storage.readVList("playlists"));
    mainWindow->show();
    if (!hasPrevious_)
        process_payloadRecieved(makePayload());
    return qApp->exec();
}

bool Flow::hasPrevious()
{
    return hasPrevious_;
}

QStringList Flow::makePayload() const
{
    return QStringList() << QDir::currentPath()
                         << QCoreApplication::arguments().mid(1);
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
    return filePath + "/" + fileName;
}

void Flow::mainwindow_applicationShouldQuit()
{
    qApp->quit();
}

void Flow::mainwindow_takeImage()
{
    QString tempFile = QString("/dev/shm/mpc-qt_%1").arg(QUuid::createUuid().toString());
    mainWindow->mpvWidget()->screenshot(tempFile, true);
    tempFile += "." + screenshotFormat;

    QString fileName = pictureTemplate(Helpers::DisabledAudio, Helpers::SubtitlesPresent);
    QString picFile;
    picFile = QFileDialog::getSaveFileName(this->mainWindow, tr("Save Image"),
                                           fileName);
    if (picFile.isEmpty()) {
        QFile(tempFile).remove();
        return;
    }
    QFileInfo qfi(picFile);
    QString dest = qfi.absolutePath() + "/" + qfi.completeBaseName() + "." + screenshotFormat;
    QFile(tempFile).copy(dest);
}

void Flow::mainwindow_takeImageAutomatically()
{
    mainWindow->mpvWidget()->screenshot(pictureTemplate(Helpers::DisabledAudio, Helpers::SubtitlesPresent), true);
}

void Flow::mainwindow_optionsOpenRequested()
{
    settingsWindow->takeSettings(settings);
    settingsWindow->show();
    settingsWindow->raise();
}

void Flow::process_payloadRecieved(const QStringList &payload)
{
    QList<QUrl> files;
    QUrl url;
    foreach (QString s, payload.mid(1)) {
        files << QUrl::fromUserInput(s, payload.first());
    }
    if (!files.empty()) {
        playbackManager->openSeveralFiles(files, true);
    }
}

void Flow::settingswindow_settingsData(const QVariantMap &settings)
{
    this->settings = settings;
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
    emit mainWindow->playlistWindow()->addSimplePlaylist(storage.readM3U(fname));
}

void Flow::exportPlaylist(QString fname, QStringList items)
{
    storage.writeM3U(fname, items);
}

