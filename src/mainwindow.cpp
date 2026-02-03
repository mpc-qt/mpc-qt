#include <cmath>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "openfiledialog.h"
#include "helpers.h"
#include "logger.h"
#include "version.h"
#include "platform/unify.h"
#include "platform/devicemanager.h"
#include <QActionGroup>
#include <QClipboard>
#include <QLocale>
#include <QMenuBar>
#include <QSizeGrip>
#include <QMimeData>
#include <QFileDialog>
#include <QInputDialog>
#include <QTime>
#include <QDesktopServices>
#include <QMessageBox>
#include <QScreen>
#include <QStyle>
#include <QSvgRenderer>
#include <QPainter>

using namespace Helpers;
static constexpr char logModule[] =  "mainwindow";
static constexpr char SKIPACTION[] = "Skip";
static constexpr char textWindowTitle[] = "Media Player Classic Qute Theater";
static constexpr char mpcQtIconPath[] = ":/images/icon/mpc-qt.svg";
static constexpr char tinyIconPath[] = ":/images/icon/tinyicon.svg";


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    Logger::log(logModule, "creating ui");
    ui->setupUi(this);
    Logger::log(logModule, "finished creating ui");
    setupMenu();
    setupContextMenu();
    setupTrayIcon();
    setupActionGroups();
    setupPositionSlider();
    setupVolumeSlider();
    setupMpvHost();
    setupMpvObject();
    setupPlaylist();
    setupStatus();
    setupSizing();
    setupBottomArea();
    setupHideTimer();
    setupIconThemer();

    if (mpvw)
        mpvw->installEventFilter(this);
    playlistWindow_->installEventFilter(this);
    ui->bottomArea->installEventFilter(this);

    connectActionsToSignals();
    connectActionsToSlots();
    connectButtonsToActions();
    connectPlaylistWindowToActions();
    globalizeAllActions();
    setUiEnabledState(false);
    setDiscState(false);

    // Sync with X11
    if (Platform::isUnix) {
        setAttribute(Qt::WA_DontShowOnScreen, true);
        show();
        QApplication::processEvents(QEventLoop::AllEvents |
                            QEventLoop::WaitForMoreEvents,
                            50);
        hide();
        setAttribute(Qt::WA_DontShowOnScreen, false);
    }
}

MainWindow::~MainWindow()
{
    Logger::log(logModule, "~MainWindow");
    mpvObject_->setWidgetType(Helpers::NullWidget);
    delete ui;
    ui = nullptr;
}

MpvObject *MainWindow::mpvObject()
{
    return mpvObject_;
}

QMainWindow *MainWindow::dockHost()
{
    return mpvHost_;
}

PlaylistWindow *MainWindow::playlistWindow()
{
    return playlistWindow_;
}

static void actionsToList(QList<QAction*> &actionList, const QList<QAction*> &actions) {
    for (QAction *a : actions) {
        if (a->data().toString() == SKIPACTION)
            continue;
        if (a->menu())
            actionsToList(actionList, a->menu()->actions());
        else if (!a->isSeparator())
            actionList.append(a);
    }
}

QList<QAction *> MainWindow::editableActions()
{
    QList<QAction*> actionList;
    actionsToList(actionList, actions());

    // Reorder actions so that seek keys aren't at the bottom
    qsizetype indexOfActionPlayFrameForward = actionList.indexOf(ui->actionPlayFrameForward);
    actionList.move(actionList.indexOf(ui->actionPlaySeekForwardsLarge),
        indexOfActionPlayFrameForward + 1);
    actionList.move(actionList.indexOf(ui->actionPlaySeekBackwardsLarge),
        indexOfActionPlayFrameForward + 1);
    actionList.move(actionList.indexOf(ui->actionPlaySeekForwardsNormal),
        indexOfActionPlayFrameForward + 1);
    actionList.move(actionList.indexOf(ui->actionPlaySeekBackwardsNormal),
        indexOfActionPlayFrameForward + 1);

    actionList.remove(actionList.indexOf(ui->actionVideoAspectName));

    return actionList;
}

QVariantMap MainWindow::mouseMapDefaults()
{
    QVariantMap commandMap;
    auto actionToMap = [&commandMap](QAction *a, const MouseState &m) {
        Command c;
        c.fromAction(a);
        c.mouseWindowed = m;
        c.mouseFullscreen = m;
        commandMap.insert(a->objectName(), c.toVMap());
    };
    actionToMap(ui->actionViewFullscreen, {MouseState::Left,0,MouseState::PressTwice});
    actionToMap(ui->actionPlayPause, {MouseState::Left,0,MouseState::MouseUp});
    actionToMap(ui->actionNavigateChaptersNext, {MouseState::Forward,0,MouseState::MouseDown});
    actionToMap(ui->actionNavigateChaptersPrevious, {MouseState::Back,0,MouseState::MouseDown});
    actionToMap(ui->actionPlayVolumeDown, {MouseState::Wheel,0,MouseState::MouseDown});
    actionToMap(ui->actionPlayVolumeUp, {MouseState::Wheel,0,MouseState::MouseUp});
    return commandMap;
}

QMap<int, QAction *> MainWindow::wmCommandMap() const
{
    QMap<int, QAction *> wmCommands = {

    };
    return wmCommands;
}

QVariantMap MainWindow::state()
{
#define WRAP(a) \
    a->objectName(), a->isChecked()
    return QVariantMap {
        { "decorationState", int(decorationState_) },
        { WRAP(ui->actionViewHideSeekbar) },
        { WRAP(ui->actionViewHideControls) },
        { WRAP(ui->actionViewHideInformation) },
        { WRAP(ui->actionViewHideStatistics) },
        { WRAP(ui->actionViewHideStatus) },
        { WRAP(ui->actionViewHideSubresync) },
        { WRAP(ui->actionViewHidePlaylist) },
        { WRAP(ui->actionViewHideCapture) },
        { WRAP(ui->actionViewHideNavigation) },
        { WRAP(ui->actionViewHideLog) },
        { WRAP(ui->actionViewHideLibrary) },
        { WRAP(ui->actionViewHideControlsInFullscreen) },
        { WRAP(ui->actionViewMusicMode) },
        { WRAP(ui->actionViewOntopDefault) },
        { WRAP(ui->actionViewOntopAlways) },
        { WRAP(ui->actionViewOntopPlaying) },
        { WRAP(ui->actionViewOntopVideo) },
        { WRAP(ui->actionViewFullscreen) },
        { WRAP(ui->actionAudioFilterExtrastereo) },
        { WRAP(ui->actionAudioFilterAcompressor) },
        { WRAP(ui->actionAudioFilterCrossfeed) },
        { WRAP(ui->actionPlaySubtitlesEnabled) },
        { WRAP(ui->actionDisableVideoAspect) },
        { WRAP(ui->actionPlayVolumeMute) },
        { "volume", volumeSlider_->value() },
        { "shownStatsPage", mpvObject_->selectedStatsPage() },
        { "timeShortMode", timeShortMode },
        { "timeRemainingMode", timeRemainingMode },
        { "timePercentageMode", timePercentageMode }
    };
#undef WRAP
}

void MainWindow::setState(const QVariantMap &map)
{
#define UNWRAP(a,b) \
    a->setChecked(map.value(a->objectName(), b).toBool())

    setUiDecorationState(DecorationState(map["decorationState"].toInt()));
    UNWRAP(ui->actionViewHideSeekbar, true);
    UNWRAP(ui->actionViewHideControls, true);
    UNWRAP(ui->actionViewHideInformation, false);
    UNWRAP(ui->actionViewHideStatistics, false);
    UNWRAP(ui->actionViewHideStatus, true);
    UNWRAP(ui->actionViewHideSubresync, false);
    UNWRAP(ui->actionViewHidePlaylist, true);
    UNWRAP(ui->actionViewHideCapture, false);
    UNWRAP(ui->actionViewHideNavigation, false);
    UNWRAP(ui->actionViewHideLog, false);
    UNWRAP(ui->actionViewHideLibrary, false);
    UNWRAP(ui->actionViewHideControlsInFullscreen, false);
    UNWRAP(ui->actionViewMusicMode, false);
    UNWRAP(ui->actionViewOntopDefault, true);
    UNWRAP(ui->actionViewOntopAlways, false);
    UNWRAP(ui->actionViewOntopPlaying, false);
    UNWRAP(ui->actionViewOntopVideo, false);
    UNWRAP(ui->actionViewFullscreen, false);
    UNWRAP(ui->actionAudioFilterExtrastereo, false);
    UNWRAP(ui->actionAudioFilterAcompressor, false);
    UNWRAP(ui->actionAudioFilterCrossfeed, false);
    UNWRAP(ui->actionPlaySubtitlesEnabled, true);
    UNWRAP(ui->actionDisableVideoAspect, false);
    UNWRAP(ui->actionPlayVolumeMute, false);

    ui->actionAudioFilterExtrastereo->isChecked() ? on_actionAudioFilterExtrastereo_triggered(true) : qt_noop();
    ui->actionAudioFilterAcompressor->isChecked() ? on_actionAudioFilterAcompressor_triggered(true) : qt_noop();
    ui->actionAudioFilterCrossfeed->isChecked() ? on_actionAudioFilterCrossfeed_triggered(true) : qt_noop();
    setSubtitlesEnabled(ui->actionPlaySubtitlesEnabled->isChecked(), true);
    setVolumeMuteState(ui->actionPlayVolumeMute->isChecked(), true);
    setVolume(map.value("volume", 100).toInt(), true);
    setOSDPage(map.value("shownStatsPage",0).toInt());
    setTimeShortMode(map.value("timeShortMode", true).toBool());
    setTimeRemainingMode(map.value("timeRemainingMode", false).toBool());
    setTimePercentageMode(map.value("timePercentageMode", false).toBool());
    updateOnTop();

#undef UNWRAP
}

void MainWindow::setScreensaverAbilities(QSet<ScreenSaver::Ability> ab)
{
    ui->actionPlayAfterOnceStandby->setVisible(ab.contains(ScreenSaver::Suspend));
    ui->actionPlayAfterOnceHibernate->setVisible(ab.contains(ScreenSaver::Hibernate));
    ui->actionPlayAfterOnceShutdown->setVisible(ab.contains(ScreenSaver::Shutdown));
    ui->actionPlayAfterOnceLogoff->setVisible(ab.contains(ScreenSaver::LogOff));
    ui->actionPlayAfterOnceLock->setVisible(ab.contains(ScreenSaver::LockScreen));
}

QSize MainWindow::desirableSize(bool first_run)
{
    if (zoomMode == FitToWindow)
        return size();

    // Grab device pixel ratio (2=HiDPI screen)
    qreal ratio = devicePixelRatioF();

    // calculate available area for the window
    QRect available = first_run ? Helpers::availableGeometryFromPoint(QCursor::pos())
                                : screen()->availableGeometry();
    QSize frameDiff = this->frameGeometry().size() - this->geometry().size();
    available.adjust(0, 0, -frameDiff.width(), -frameDiff.height());

    // calculate player size
    QSize player = mpvObject_->videoSize() / ratio;
    double factor = sizeFactor();
    if (!isPlaying || player.isEmpty()) {
        player = noVideoSize_;
        factor = std::max(1.0, sizeFactor());
    }

    // calculate the amount taken by widgets outside the video frame
    QSize mpvSize = mpvw && !first_run ? mpvw->size() : noVideoSize_;
    QSize fudgeFactor = size() - mpvSize;

    // calculate desired client size, depending upon the zoom mode
    QSize wanted;
    if (zoomMode == RegularZoom) {
        wanted = QSize(int(player.width()*factor + 0.5),
                       int(player.height()*factor + 0.5));
    } else {
        wanted = (available.size() - fudgeFactor) * fitFactor_;
        if (zoomMode == Autofit) {
            wanted = player.scaled(wanted.width(), wanted.height(),
                                   Qt::KeepAspectRatio);
        } else if (zoomMode == AutofitLarger) {
            if (wanted.height() > player.height()
                    || wanted.width() > player.width()) {
                // window is larger than player size, so set to player size
                wanted = player;
            } else {
                // window is smaller than player size, so limit new window
                // size to contain a player rectangle
                wanted = player.scaled(wanted.width(), wanted.height(),
                                       Qt::KeepAspectRatio);
            }
        } else { // zoomMode == AutofitSmaller
            if (player.height() < wanted.height()
                    || player.width() < wanted.width()) {
                // player size is smaller than wanted window, so make it larger
                wanted = player.scaled(wanted.width(), wanted.height(),
                                       Qt::KeepAspectRatioByExpanding);
            } else {
                // player size is larger than wanted, so use the player size
                wanted = player;
            }
        }
    }
    return wanted + fudgeFactor;
}

QPoint MainWindow::desirablePosition(QSize &size, bool first_run) const
{
    QRect available = first_run ? Helpers::availableGeometryFromPoint(QCursor::pos())
                                : screen()->availableGeometry();
    QSize frameDiff = this->frameGeometry().size() - this->geometry().size();
    available.adjust(0, 0, -frameDiff.width(), -frameDiff.height());
    if (size.height() > available.height())
        size.setHeight(available.height());
    if (size.width() > available.width())
        size.setWidth(available.width());

    QPoint clientOffset = geometry().topLeft() - pos();
    return QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter,
                               size, available).topLeft() + clientOffset;
}

void MainWindow::unfreezeWindow()
{
    frozenWindow = false;
}

// REMOVEME: work around bug on Wayland where video doesn't fit window
void MainWindow::fixMpvwSize()
{
    QSize size = mpvw->size();
    mpvw->resize(noVideoSize_);
    mpvw->resize(size);
}

void MainWindow::setActionPlayLoopUse()
{
    ui->actionPlayLoopUse->setChecked(false);
    ui->actionPlayLoopUse->setChecked(true);
}

void MainWindow::setRemoveFileNotRecycle()
{
    ui->actionFileMoveToRecycleBin->setText(tr("Re&move File"));
}

void MainWindow::resizePlaylistToFit()
{
    if (ui->actionViewMusicMode->isChecked() && !playlistWindow_->isFloating()) {
        int otherWidgetsHeight = ui->menubar->height() + ui->bottomArea->height();
        playlistWindow_->setMinimumSize(QSize(this->width(), this->height() - otherWidgetsHeight));
        playlistWindow_->setMaximumSize(QSize(this->width(), this->height() - otherWidgetsHeight));
        dockHost()->resizeDocks({playlistWindow_}, {this->width()}, Qt::Horizontal);
        dockHost()->resizeDocks({playlistWindow_}, {this->height()}, Qt::Vertical);
        QApplication::processEvents();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    updateBottomAreaGeometry();
    checkBottomArea(QCursor::pos());
    resizePlaylistToFit();
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    bool insideMpv = mpvw ? object == mpvw : false;
    if ((insideMpv || object == playlistWindow_) && event->type() == QEvent::MouseMove) {
        this->mouseMoveEvent(static_cast<QMouseEvent*>(event));
    }
    if (object == ui->bottomArea && event->type() == QEvent::Leave)
        this->leaveBottomArea();

    if (event->type() == QEvent::ApplicationPaletteChange)
        positionSlider_->applicationPaletteChanged();

    if (Platform::isWindows && event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        switch (ke->key()) {
        case Qt::Key_MediaPlay:
        case Qt::Key_MediaPause:
        case Qt::Key_MediaTogglePlayPause:
            ke->accept();
            on_play_clicked();
            return true;
        case Qt::Key_MediaStop:
            ke->accept();
            if (ui->actionPlayStop->isEnabled())
                ui->actionPlayStop->activate(QAction::Trigger);
            return true;
        case Qt::Key_MediaPrevious:
            ke->accept();
            if (ui->actionNavigateChaptersPrevious->isEnabled())
                ui->actionNavigateChaptersPrevious->activate(QAction::Trigger);
            return true;
        case Qt::Key_MediaNext:
            ke->accept();
            if (ui->actionNavigateChaptersNext->isEnabled())
                ui->actionNavigateChaptersNext->activate(QAction::Trigger);
            return true;
        }
    }
    return QMainWindow::eventFilter(object, event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Logger::log(logModule, "closeEvent");
    bool showPlaylist = ui->actionViewHidePlaylist->isChecked();
    playlistWindow_->close();
    ui->actionViewHidePlaylist->setChecked(showPlaylist);
    mainwindowIsClosing = true;
    event->accept();
    emit instanceShouldQuit();
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (fullscreenMode_)
        checkBottomArea(event->globalPosition());
    if (mousePressedInsideStatustime) {
        mousePressedInsideStatustime = false;
        QWindow *parentWindow = this->window()->windowHandle();
        parentWindow->startSystemMove();
    }
}

static bool insideWidget(QPoint p, QWidget const *widget) {
    if (widget == nullptr)
        return false;
    QRect rc(widget->mapToGlobal(QPoint()), widget->size());
    return rc.contains(p);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    QPoint pos = event->globalPosition().toPoint();
    bool isInMpvw = mpvw ? insideWidget(pos, mpvw) : false;
    bool isInBottomArea = ui->bottomArea->isVisible() ? insideWidget(pos, ui->bottomArea) : false;
    mousePressedInsideStatustime = insideWidget(pos, statusTime);
    if (isInMpvw && mouseStateEvent(MouseState::fromMouseEvent(event, MouseState::MouseDown)))
        event->accept();
    else if (isInBottomArea && !mousePressedInsideStatustime)  {
        QWindow *parentWindow = this->window()->windowHandle();
        parentWindow->startSystemMove();
    }
    else
        QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    QPoint pos = event->globalPosition().toPoint();
    bool ok = mpvw ? insideWidget(pos, mpvw) : false;
    if (ok && mouseStateEvent(MouseState::fromMouseEvent(event, MouseState::PressTwice)))
        event->accept();
    mousePressEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    QPoint pos = event->globalPosition().toPoint();
    bool ok = mpvw ? insideWidget(pos, mpvw) : false;
    if (mousePressedInsideStatustime && insideWidget(pos, statusTime)) {
        if (event->button() == Qt::MouseButton::LeftButton)
            setTimeRemainingMode(!timeRemainingMode);
    }
    else {
        if (ok && mouseStateEvent(MouseState::fromMouseEvent(event, MouseState::MouseUp)))
            event->accept();
        else
            QMainWindow::mouseReleaseEvent(event);
    }
    mousePressedInsideStatustime = false;
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    if (mouseStateEvent(MouseState::fromWheelEvent(event)))
        event->accept();
    else
        QMainWindow::wheelEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->accept();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasUrls())
        return;

    // intercept single subtitle files
    if (event->mimeData()->urls().count() == 1) {
        QUrl url = event->mimeData()->urls()[0];
        QFileInfo info(url.toLocalFile());
        if (isPlaying && Helpers::subsExtensions.contains(info.suffix())) {
            // load subtitle, but only when playing media
            emit subtitlesLoaded(url);
            return;
        }
        // fallthrough to opening as files
    }
    emit severalFilesOpened(event->mimeData()->urls(), true);
}

bool MainWindow::mouseStateEvent(const MouseState &state)
{
    MouseStateMap &mouseMap = fullscreenMode_ ? mouseMapFullscreen
                                              : mouseMapWindowed;

    if (mouseMap.contains(state)) {
        QAction *action = mouseMap[state];
        if (!action->isEnabled())
            return true;
        if (action->isCheckable())
            action->setChecked(!action->isChecked());
        emit action->triggered(action->isChecked());
        return true;
    }
    return false;
}

MediaSlider *MainWindow::positionSlider()
{
    return positionSlider_;
}

VolumeSlider *MainWindow::volumeSlider()
{
    return volumeSlider_;
}

MainWindow::DecorationState MainWindow::decorationState() const
{
    return decorationState_;
}

bool MainWindow::fullscreenMode() const
{
    return fullscreenMode_;
}

QSize MainWindow::noVideoSize() const
{
    return noVideoSize_;
}

double MainWindow::sizeFactor() const
{
    return sizeFactor_;
}

void MainWindow::setFullscreenMode(bool fullscreenMode)
{
    if (fullscreenMode_ == fullscreenMode)
        return;
    fullscreenMode_ = fullscreenMode;

    if (fullscreenMode)
        fullscreenMemory = WindowManager::makeFullscreen(this, fullscreenName);
    else {
        WindowManager::restoreFullscreen(this, fullscreenMemory);
        if (videoPreview)
            videoPreview->hide();
        if (tooltip)
            tooltip->hide();
    }

    ui->actionViewFullscreenEscape->setEnabled(fullscreenMode);
    updateMouseHideTime();

    //REMOVEME: work around OpenGL blackness bug after fullscreen
    QTimer::singleShot(50, this, [this]() {
        positionSlider_->update();
        volumeSlider_->update();
        statusTime->update();
    });
    emit fullscreenModeChanged(fullscreenMode_);
}

void MainWindow::setDiscState(bool playingADisc)
{
    ui->actionNavigateMenuTitle->setEnabled(playingADisc);
    ui->actionNavigateMenuRoot->setEnabled(playingADisc);
    ui->actionNavigateMenuSubtitle->setEnabled(playingADisc);
    ui->actionNavigateMenuAudio->setEnabled(playingADisc);
    ui->actionNavigateMenuAngle->setEnabled(playingADisc);
    ui->actionNavigateMenuChapter->setEnabled(playingADisc);
}

void MainWindow::setupMenu()
{
    // Hide certain items that aren't going to be implemented
    ui->actionViewHideSubresync->setVisible(false);
    ui->actionViewHideCapture->setVisible(false);
    ui->actionViewHideNavigation->setVisible(false);

    ui->infoStats->setVisible(false);

    connect(Platform::deviceManager(), &DeviceManager::deviceListChanged,
            this, [this]() { updateDiscList(); });
}

void MainWindow::setupContextMenu()
{
    contextMenu = new QMenu(this);

    contextMenu->addMenu(ui->menuFile);
    contextMenu->addSeparator();
    contextMenu->addAction(ui->actionPlayPause);
    contextMenu->addAction(ui->actionPlayStop);
    contextMenu->addSeparator();
    contextMenu->addAction(ui->actionViewFullscreen);
    contextMenu->addMenu(ui->menuNavigate);
    contextMenu->addMenu(ui->menuFavorites);
    contextMenu->addSeparator();
    contextMenu->addMenu(ui->menuPlayAudio);
    contextMenu->addMenu(ui->menuPlaySubtitles);
    contextMenu->addMenu(ui->menuPlayVideo);
    contextMenu->addSeparator();
    contextMenu->addMenu(ui->menuPlayVolume);
    contextMenu->addMenu(ui->menuPlayAfter);
    contextMenu->addSeparator();

    QMenu *contextView = new QMenu(tr("View"), this);
    contextView->addMenu(ui->menuViewOntop);
    contextView->addSeparator();
    contextView->addAction(ui->actionViewHideMenu);
    contextView->addAction(ui->actionViewHideSeekbar);
    contextView->addAction(ui->actionViewHideControls);
    contextView->addAction(ui->actionViewHideInformation);
    contextView->addAction(ui->actionViewHideStatistics);
    contextView->addAction(ui->actionViewHideStatus);
    contextView->addAction(ui->actionViewHideSubresync);
    contextView->addAction(ui->actionViewHidePlaylist);
    contextView->addAction(ui->actionViewHideControlsInFullscreen);
    contextView->addAction(ui->actionViewMusicMode);
    contextView->addAction(ui->actionViewHideCapture);
    contextView->addAction(ui->actionViewHideNavigation);
    contextView->addMenu(ui->menuViewPresets);

    contextMenu->addMenu(contextView);
    contextMenu->addAction(ui->actionFileProperties);
    contextMenu->addAction(ui->actionViewOptions);
    contextMenu->addSeparator();
    contextMenu->addAction(ui->actionFileExit);
}

void MainWindow::setupTrayIcon()
{
    trayMenu = new QMenu(this);
    trayMenu->addMenu(ui->menuFile);
    trayMenu->addMenu(ui->menuPlaylist);
    trayMenu->addMenu(ui->menuView);
    trayMenu->addMenu(ui->menuPlay);
    trayMenu->addMenu(ui->menuNavigate);
    trayMenu->addMenu(ui->menuFavorites);
    trayMenu->addMenu(ui->menuHelp);
    trayMenu->addSeparator();
    // TODO: add filter menu
    // TODO: add shader menu
    // add seperator
    trayMenu->addMenu(ui->menuPlayAudio);
    trayMenu->addMenu(ui->menuPlaySubtitles);
    trayMenu->addMenu(ui->menuPlayVideo);
    trayMenu->addSeparator();
    trayMenu->addMenu(ui->menuPlayVolume);
    trayMenu->addMenu(ui->menuPlayAfter);
    trayMenu->addSeparator();
    // TODO: add renderer settings
    trayMenu->addAction(ui->actionFileProperties);
    trayMenu->addAction(ui->actionViewOptions);
    trayMenu->addSeparator();
    trayMenu->addAction(ui->actionFileExit);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayMenu);
    trayIcon->setToolTip(textWindowTitle);
    Logger::log(logModule, "rendering trayIcon sizes");
    trayIcon->setIcon(createIconFromSvg(mpcQtIconPath, 64));
    Logger::log(logModule, "rendering trayIcon sizes done");
}

void MainWindow::setupActionGroups()
{
    QActionGroup *ag;

    ag = new QActionGroup(this);
    ag->addAction(ui->actionViewOSDNone);
    ag->addAction(ui->actionViewOSDMessages);
    ag->addAction(ui->actionViewOSDStatistics);
    ag->addAction(ui->actionViewOSDInputCacheStats);

    ag = new QActionGroup(this);
    ag->addAction(ui->actionViewZoom025);
    ag->addAction(ui->actionViewZoom050);
    ag->addAction(ui->actionViewZoom075);
    ag->addAction(ui->actionViewZoom100);
    ag->addAction(ui->actionViewZoom150);
    ag->addAction(ui->actionViewZoom200);
    ag->addAction(ui->actionViewZoom300);
    ag->addAction(ui->actionViewZoom400);
    ag->addAction(ui->actionViewZoomAutofit);
    ag->addAction(ui->actionViewZoomAutofitLarger);
    ag->addAction(ui->actionViewZoomAutofitSmaller);
    ag->addAction(ui->actionViewZoomDisable);

    ag = new QActionGroup(this);
    ag->addAction(ui->actionViewOntopDefault);
    ag->addAction(ui->actionViewOntopAlways);
    ag->addAction(ui->actionViewOntopPlaying);
    ag->addAction(ui->actionViewOntopVideo);

    ag = new QActionGroup(this);
    ag->addAction(ui->actionPlayAfterOnceExit);
    ag->addAction(ui->actionPlayAfterOnceHibernate);
    ag->addAction(ui->actionPlayAfterOnceLock);
    ag->addAction(ui->actionPlayAfterOnceLogoff);
    ag->addAction(ui->actionPlayAfterOnceNothing);
    ag->addAction(ui->actionPlayAfterOnceShutdown);
    ag->addAction(ui->actionPlayAfterOnceStandby);
    ag->addAction(ui->actionPlayAfterAlwaysNext);
}

void MainWindow::setupPositionSlider()
{
    positionSlider_ = new MediaSlider();
    ui->seekbar->layout()->addWidget(positionSlider_);
    connect(positionSlider_, &MediaSlider::sliderMoved,
            this, &MainWindow::position_sliderMoved);
    connect(positionSlider_, &MediaSlider::hoverValue,
            this, &MainWindow::position_hoverValue);
    connect(positionSlider_, &MediaSlider::hoverEnd,
            this, &MainWindow::position_hoverEnd);
}

void MainWindow::setupVolumeSlider()
{
    volumeSlider_ = new VolumeSlider();
    volumeSlider_->setMinimumWidth(50);
    volumeSlider_->setMinimum(0);
    volumeSlider_->setMaximum(130);
    volumeSlider_->setValue(100);
    ui->controlbar->layout()->addWidget(volumeSlider_);
    connect(volumeSlider_, &VolumeSlider::sliderMoved,
            this, &MainWindow::volume_sliderMoved);
}

void MainWindow::setupMpvHost()
{
    // Create a special QMainWindow widget so that the playlist window will
    // dock around it rather than ourselves
    mpvHost_ = new QMainWindow(this);
    mpvHost_->setObjectName("mpvHost");
    mpvHost_->setSizePolicy(QSizePolicy(QSizePolicy::Ignored,
                                        QSizePolicy::Ignored));
    ui->mpvWidget->layout()->addWidget(mpvHost_);
}

void MainWindow::setupMpvObject()
{
    mpvObject_ = new MpvObject(this, textWindowTitle);
    mpvObject_->setHostWindow(mpvHost_);
    setupMpvWidget(Helpers::GlCbWidget);
    connect(mpvObject_, &MpvObject::logoSizeChanged,
            this, &MainWindow::setNoVideoSize);
    connect(mpvObject_, &MpvObject::subTextChanged,
            this, &MainWindow::setSubtitleText);
}

void MainWindow::setupMpvWidget(Helpers::MpvWidgetType widgetType)
{
    bool embeddedBottomArea = ui->bottomArea->parentWidget() == mpvw;
    if (embeddedBottomArea) {
        reparentBottomArea(false);
    }

    mpvw = nullptr;
    mpvObject_->setWidgetType(widgetType);
    mpvw = mpvObject_->mpvWidget();
    if (!mpvw) {
        //throw(std::runtime_error("Video widget not created"));
        // instead of exiting early, show nothing instead.
    } else {
        // CHECKME: signal could be in mpvObject and reflected internally
        connect(mpvw, &QWidget::customContextMenuRequested,
                this, &MainWindow::mpvw_customContextMenuRequested);
        // CHECKME: mouse tracking could be set by mpvObject's setWidgetType func
        mpvw->setMouseTracking(true);
    }

    if (embeddedBottomArea)
        reparentBottomArea(true);
}

void MainWindow::setupPlaylist()
{
    playlistWindow_ = new PlaylistWindow(this);
    dockHost()->addDockWidget(Qt::RightDockWidgetArea, playlistWindow_);
    connect(playlistWindow_, &PlaylistWindow::windowDocked,
            this, &MainWindow::playlistWindow_windowDocked);
    connect(playlistWindow_, &PlaylistWindow::playlistAddItem,
            this, &MainWindow::playlistWindow_playlistAddItem);
    connect(playlistWindow_, &PlaylistWindow::playlistAddFolder,
            this, &MainWindow::on_actionFileOpenDirectory_triggered);
    connect(playlistWindow_, &PlaylistWindow::currentPlaylistHasItems,
            ui->play, &QPushButton::setEnabled);
    QList<QWidget *> playlistWidgets = playlistWindow_->findChildren<QWidget *>();
    foreach(QWidget *w, playlistWidgets)
        w->setMouseTracking(true);
    playlistWindow_->setMouseTracking(true);
}

void MainWindow::setupStatus()
{
    ui->tinyicon->setPixmap(renderPixmapFromSvg(tinyIconPath));
    statusTime = new StatusTime();
    statusTime->setCursor(Qt::PointingHandCursor);
    statusTime->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->statusbarLayout->insertWidget(2, statusTime);
    connect(statusTime, &StatusTime::customContextMenuRequested,
            this, &MainWindow::statusTime_customContextMenuRequested);
}

void MainWindow::setupSizing()
{
    QSizeGrip *gripper = new QSizeGrip(this);
    ui->statusbar->layout()->addWidget(gripper);
}

void MainWindow::setupBottomArea()
{
    QList<QWidget *> bottomWidgets = ui->bottomArea->findChildren<QWidget *>();
    foreach(QWidget *w, bottomWidgets)
        w->setMouseTracking(true);
    ui->bottomArea->setMouseTracking(true);
}

void MainWindow::setupIconThemer()
{
    QVector<IconThemer::IconData> data {
        { ui->play, "media-playback-start", {} },
        { ui->pause, "media-playback-pause", {} },
        { ui->stop, "media-playback-stop", {} },
        { ui->skipBackward, "media-skip-backward", {} },
        { ui->skipForward, "media-skip-forward", {} },
        { ui->speedDecrease, "media-seek-backward", {} },
        { ui->speedIncrease, "media-seek-forward", {} },
        { ui->stepBackward, "go-previous", {} },
        { ui->stepForward, "go-next", {} },
        { ui->loopA, "zone-in", {} },
        { ui->loopB, "zone-out", {} },
        { ui->subs, "view-media-subtitles", "view-media-subtitles-hidden" },
        { ui->mute, "player-volume", "player-volume-muted" }
    };
    for (auto &d : data)
        themer.addIconData(d);
}

void MainWindow::setupHideTimer()
{
    hideTimer.setSingleShot(true);
    connect(&hideTimer, &QTimer::timeout,
            this, &MainWindow::hideTimer_timeout);
}

void MainWindow::connectActionsToSignals() const
{
    connect(ui->actionFileSaveThumbnails, &QAction::triggered,
            this, &MainWindow::takeThumbnails);
    connect(ui->actionFileProperties, &QAction::triggered,
            this, &MainWindow::showFileProperties);
}

void MainWindow::connectActionsToSlots() const
{
    connect(ui->actionHelpAboutQt, &QAction::triggered,
            qApp, &QApplication::aboutQt);
}

void MainWindow::connectButtonsToActions()
{
    connect(ui->pause, &QPushButton::clicked,
            ui->actionPlayPause, &QAction::triggered);
    connect(ui->stop, &QPushButton::clicked,
            ui->actionPlayStop, &QAction::triggered);

    connect(ui->speedDecrease, &QPushButton::clicked,
            ui->actionPlayRateDecrease, &QAction::triggered);
    connect(ui->speedIncrease, &QPushButton::clicked,
            ui->actionPlayRateIncrease, &QAction::triggered);

    connect(ui->skipBackward, &QPushButton::clicked,
            ui->actionNavigateChaptersPrevious, &QAction::triggered);
    connect(ui->stepBackward, &QPushButton::clicked,
            ui->actionPlayFrameBackward, &QAction::triggered);
    connect(ui->stepForward, &QPushButton::clicked,
            ui->actionPlayFrameForward, &QAction::triggered);
    connect(ui->skipForward, &QPushButton::clicked,
            ui->actionNavigateChaptersNext, &QAction::triggered);

    connect(ui->loopA, &QPushButton::clicked,
            ui->actionPlayLoopStart, &QAction::triggered);
    connect(ui->loopB, &QPushButton::clicked,
            ui->actionPlayLoopEnd, &QAction::triggered);

    connect(ui->subs, &QPushButton::toggled, this,
            [this](bool checked) { on_actionPlaySubtitlesEnabled_triggered(!checked); });
    connect(ui->mute, &QPushButton::toggled,
            ui->actionPlayVolumeMute, &QAction::toggled);
}

void MainWindow::connectPlaylistWindowToActions() const
{
    connect(ui->actionPlaylistCopy, &QAction::triggered,
            playlistWindow_, &PlaylistWindow::copy);
    connect(ui->actionPlaylistCopyQueue, &QAction::triggered,
            playlistWindow_, &PlaylistWindow::copyQueue);
    connect(ui->actionPlaylistPaste, &QAction::triggered,
            playlistWindow_, &PlaylistWindow::paste);
    connect(ui->actionPlaylistPasteQueue, &QAction::triggered,
            playlistWindow_, &PlaylistWindow::pasteQueue);

    connect(ui->actionPlaylistNewTab, &QAction::triggered,
            playlistWindow_, &PlaylistWindow::newTab);
    connect(ui->actionPlaylistCloseTab, &QAction::triggered,
            playlistWindow_, &PlaylistWindow::closeTab);
    connect(ui->actionPlaylistDuplicateTab, &QAction::triggered,
            playlistWindow_, &PlaylistWindow::duplicateTab);
    connect(ui->actionPlaylistImport, &QAction::triggered,
            playlistWindow_, &PlaylistWindow::importTab);
    connect(ui->actionPlaylistExport, &QAction::triggered,
            playlistWindow_, &PlaylistWindow::exportTab);
    connect(ui->actionPlaylistPlaySelected, &QAction::triggered,
            playlistWindow_, &PlaylistWindow::playCurrentItem);
    connect(ui->actionPlaylistShowQuickQueue, &QAction::triggered,
            playlistWindow_, &PlaylistWindow::setQueueMode);
    connect(ui->actionPlaylistQuickQueue, &QAction::triggered,
            playlistWindow_, &PlaylistWindow::quickQueue);
    connect(ui->actionPlaylistQueueVisible, &QAction::triggered,
            playlistWindow_, &PlaylistWindow::visibleToQueue);
    connect(ui->actionPlaylistFinishSearching, &QAction::triggered,
            playlistWindow_, &PlaylistWindow::finishSearch);

    connect(ui->actionPlaylistExtraIncrement, &QAction::triggered,
            playlistWindow_, &PlaylistWindow::incExtraPlayTimes);
    connect(ui->actionPlaylistExtraDecrement, &QAction::triggered,
            playlistWindow_, &PlaylistWindow::decExtraPlayTimes);
    connect(ui->actionPlaylistExtraZero, &QAction::triggered,
            playlistWindow_, &PlaylistWindow::zeroExtraPlayTimes);
}

void MainWindow::globalizeAllActions()
{
    for (QAction *a : ui->menubar->actions()) {
        addAction(a);
    }
    addAction(ui->actionPlaySeekBackwardsNormal);
    addAction(ui->actionPlaySeekForwardsNormal);
    addAction(ui->actionPlaySeekBackwardsLarge);
    addAction(ui->actionPlaySeekForwardsLarge);
}

void MainWindow::setUiDecorationState(DecorationState state)
{
    QString actionTexts[] = { tr("Hide &Menu"), tr("Hide &Borders"),
                              tr("Sho&w Caption and Menu") };
    ui->actionViewHideMenu->setText(actionTexts[static_cast<int>(state)]);
    decorationState_ = state;

    if (state == AllDecorations && !ui->menubar->isVisible()) {
        ui->menubar->show();
    } else if (state != AllDecorations && ui->menubar->isVisible()) {
        ui->menubar->hide();
    }

    updateWindowFlags();
}

void MainWindow::setOSDPage(int page)
{
    ui->actionViewOSDNone->setChecked(page == -1);
    ui->actionViewOSDMessages->setChecked(page == 0);
    ui->actionViewOSDStatistics->setChecked(page == 1);
    ui->actionViewOSDInputCacheStats->setChecked(page == 3);
    ui->actionViewOSDTimer->setEnabled(page == 0);
    mpvObject_->showStatsPage(page);
}

void MainWindow::setUiEnabledState(bool enabled)
{
    positionSlider_->setEnabled(enabled);
    statusTime->setEnabled(enabled);
    if (!enabled) {
        positionSlider_->setLoopA(-1);
        positionSlider_->setLoopB(-1);
    }

    ui->pause->setEnabled(enabled);
    ui->stop->setEnabled(enabled);
    ui->stepBackward->setEnabled(enabled);
    ui->speedDecrease->setEnabled(enabled);
    ui->speedIncrease->setEnabled(enabled);
    ui->stepForward->setEnabled(enabled);
    ui->skipBackward->setEnabled(enabled);
    ui->skipForward->setEnabled(enabled);
    ui->loopA->setEnabled(enabled);
    ui->loopB->setEnabled(enabled);

    ui->pause->setChecked(false);

    ui->actionFileOpenDevice->setEnabled(false);
    ui->actionFileClose->setEnabled(enabled);
    ui->actionFileSaveCopy->setEnabled(enabled && false);
    ui->menuFileScreenshot->setEnabled(enabled);
    ui->actionFileSaveImage->setEnabled(enabled);
    ui->actionFileSaveImageAuto->setEnabled(enabled);
    ui->actionFileSavePlainImage->setEnabled(enabled);
    ui->actionFileSavePlainImageAuto->setEnabled(enabled);
    ui->actionFileSaveWindowImage->setEnabled(enabled);
    ui->actionFileSaveWindowImageAuto->setEnabled(enabled);
    ui->actionFileSaveThumbnails->setEnabled(enabled);
    ui->actionFileExportEncode->setEnabled(enabled && false);
    ui->actionFileLoadSubtitle->setEnabled(enabled);
    ui->actionFileSaveSubtitle->setEnabled(enabled && false);
    ui->actionFileSubtitleDatabaseDownload->setEnabled(enabled && false);
    ui->actionPlayStop->setEnabled(enabled);
    ui->actionPlayFrameBackward->setEnabled(enabled);
    ui->actionPlayFrameForward->setEnabled(enabled);
    ui->actionPlayRateDecrease->setEnabled(enabled);
    ui->actionPlayRateIncrease->setEnabled(enabled);
    ui->actionPlayRateReset->setEnabled(enabled);
    ui->actionNavigateChaptersPrevious->setEnabled(enabled);
    ui->actionNavigateChaptersNext->setEnabled(enabled);
    ui->actionNavigateFilesPrevious->setEnabled(enabled);
    ui->actionNavigateFilesNext->setEnabled(enabled);
    ui->actionFileMoveToRecycleBin->setEnabled(enabled);
    ui->actionNavigateGoto->setEnabled(enabled);
    ui->actionFavoritesAdd->setEnabled(enabled);

    ui->menuFileSubtitleDatabase->setEnabled(enabled);
    ui->actionFileSubtitleDatabaseUpload->setEnabled(false);
    ui->actionFileSubtitleDatabaseDownload->setEnabled(false);
    ui->menuPlayLoop->setEnabled(enabled);
    if (!enabled) {
        ui->menuPlayAudio->setEnabled(false);
        ui->menuPlaySubtitles->setEnabled(false);
        ui->menuPlayVideo->setEnabled(false);
        ui->menuNavigateChapters->setEnabled(false);
    }
}

void MainWindow::setVolumeMuteState(bool checked, bool onInit)
{
    emit volumeMuteChanged(checked, onInit);
    ui->actionPlayVolumeMute->setChecked(checked);
    ui->mute->setChecked(checked);
}

void MainWindow::reparentBottomArea(bool overlay)
{
    // TODO: changing the presentation method will need to check if the
    // bottom area is inside the mpv widget!
    bool inLayout = ui->centralwidget->layout()->indexOf(ui->bottomArea) >= 0;
    if (overlay && inLayout) {
        bottomAreaHeight = ui->bottomArea->height();
        ui->centralwidget->layout()->removeWidget(ui->bottomArea);
        ui->bottomArea->setParent(nullptr);
        ui->bottomArea->setParent(mpvw ? mpvw : mpvHost_);
    }
    if (!overlay && !inLayout) {
        ui->bottomArea->setParent(nullptr);
        ui->centralwidget->layout()->addWidget(ui->bottomArea);
        ui->bottomArea->show();
    }
}

void MainWindow::checkBottomArea(QPointF mousePosition)
{
    if (!fullscreenMode_)
        return;

    QPointF relMousePosition;
    bool startTimer = false;
    switch (bottomAreaBehavior) {
    case Helpers::NeverShown:
        if (ui->bottomArea->isVisible())
            ui->bottomArea->hide();
        break;
    case Helpers::ShowWhenHovering:
        if (mpvw == nullptr)
            break;
        relMousePosition = mpvw->mapFromGlobal(mousePosition);
        if (ui->bottomArea->geometry().contains(relMousePosition.toPoint())) {
            if (ui->bottomArea->isHidden())
                ui->bottomArea->show();
        } else if (ui->bottomArea->isVisible() && !hideTimer.isActive()) {
            startTimer = true;
        }
        break;
    case Helpers::ShowWhenMoving:
        if (mpvw == nullptr)
            break;
        relMousePosition = mapFromGlobal(mousePosition);
        if (mpvw->geometry().contains(relMousePosition.toPoint())) {
            if (ui->bottomArea->isHidden())
                ui->bottomArea->show();
            startTimer = true;
        } else if (ui->bottomArea->isVisible() && !hideTimer.isActive()) {
            startTimer = true;
        }
        break;
    case Helpers::AlwaysShow:
        if (ui->bottomArea->isHidden())
            ui->bottomArea->show();
        break;
    }

    if (startTimer) {
        if (!bottomAreaHideTime)
            ui->bottomArea->hide();
        else
            hideTimer.start();
    }
}

void MainWindow::leaveBottomArea()
{
    if (!fullscreenMode_)
        return;

    if (bottomAreaBehavior == Helpers::ShowWhenHovering) {
        if (!bottomAreaHideTime) {
            ui->bottomArea->hide();
        } else {
            hideTimer.start();
        }
    }
}

void MainWindow::updateBottomAreaGeometry()
{
    if (!fullscreenMode_)
        return;

    QSize sz = size();
    bottomAreaHeight = ui->bottomArea->height();
    if (playlistWindow_->isVisible() && !fullscreenHidePanels)
        sz -= QSize(playlistWindow_->width(), 0);
    ui->bottomArea->setGeometry(0, sz.height() - bottomAreaHeight,
                                sz.width(), bottomAreaHeight);
}

void MainWindow::updateFramedrops()
{
    ui->framedrops->setText(QString(tr("vo: %1, decoder: %2"))
                            .arg(displayDrops > 0 ? displayDrops : 0)
                            .arg(decoderDrops > 0 ? decoderDrops : 0));
}

void MainWindow::updateBitrate()
{
    ui->bitrate->setText(QString(tr("v: %1 kb/s, a: %2 kb/s"))
                         .arg(std::lrint(videoBitrate / 1000))
                         .arg(std::lrint(audioBitrate / 1000)));
}

void MainWindow::updateSize(bool first_run)
{
    if (frozenWindow)
        return;

    if (!first_run && (isMaximized() || isFullScreen()))
        return;

    if (zoomMode == FitToWindow)
        return;

    QSize desired = desirableSize(first_run);
    QPoint desiredPlace = desirablePosition(desired, first_run);
    QRect where = geometry();
    if (zoomCenter)
        where.moveTopLeft(desiredPlace);
    where.setSize(desired);
    setGeometry(where);
    show();
}

void MainWindow::updateInfostats()
{
    bool infoShow = ui->actionViewHideInformation->isChecked();
    bool statShow = ui->actionViewHideStatistics->isChecked();

    ui->infoTitle->setVisible(infoShow);
    ui->infoTitleLabel->setVisible(infoShow);
    ui->chapter->setVisible(infoShow);
    ui->chapterLabel->setVisible(infoShow);

    ui->framerate->setVisible(statShow);
    ui->framerateLabel->setVisible(statShow);
    ui->avsync->setVisible(statShow);
    ui->avsyncLabel->setVisible(statShow);
    ui->framedrops->setVisible(statShow);
    ui->framedropsLabel->setVisible(statShow);
    ui->bitrate->setVisible(statShow);
    ui->bitrateLabel->setVisible(statShow);

    ui->infoStats->setVisible(infoShow || statShow);
}

void MainWindow::updateOnTop()
{
    switch (onTopMode) {
    case AlwaysOnTop:
        showOnTop = true;
        break;
    case OnTopWhenPlaying:
        showOnTop = isPlaying && !isPaused;
        break;
    case OnTopForVideos:
        showOnTop = isPlaying && !isPaused && hasVideo;
        break;
    default:
        showOnTop = false;
    }
    updateWindowFlags();
}

void MainWindow::updateWindowFlags()
{
    if (frozenWindow)
        return;

    Qt::WindowFlags flags = windowFlags();
    Qt::WindowFlags old = flags;
    bool isOnTop = flags.testFlag(Qt::WindowStaysOnTopHint);
    bool isFrameless = flags.testFlag(Qt::FramelessWindowHint);

    if (showOnTop && !isOnTop)
        flags |= Qt::WindowStaysOnTopHint;
    else if (!showOnTop && isOnTop)
        flags &= ~Qt::WindowStaysOnTopHint;

    if (decorationState_ == NoDecorations && !isFrameless)
        flags |= Qt::FramelessWindowHint;
    else if (decorationState_ != NoDecorations && isFrameless)
        flags &= ~Qt::FramelessWindowHint;

    if (old != flags) {
        QTimer::singleShot(50, this, [this,flags]() {
            setWindowFlags(flags);
            show();
        });
    }
}

void MainWindow::updateMouseHideTime()
{
    mpvObject_->setMouseHideTime(fullscreenMode_
                                 ? mouseHideTimeFullscreen
                                 : mouseHideTimeWindowed);
}


void MainWindow::updateDiscList()
{
    bool addedSomething = false;
    auto func = [this, &addedSomething](DeviceInfo *device) {
        if (device->deviceType != DeviceInfo::OpticalDrive &&
            device->deviceType != DeviceInfo::RemovableDrive)
            return;
        QAction *a = new QAction(device->toDisplayString());
        connect(a, &QAction::triggered,
                this, [this,device]() {
            if (Platform::deviceManager()->isDeviceValid(device)) {
                device->mount();
                if (!device->mountedPath.isEmpty())
                    emit dvdbdOpened(QUrl::fromLocalFile(device->mountedPath));
            }
        });
        a->setData(SKIPACTION);
        ui->menuFileOpenDisc->addAction(a);
        addedSomething = true;
    };
    ui->menuFileOpenDisc->clear();
    Platform::deviceManager()->iterateDevices(func);
    ui->menuFileOpenDisc->setEnabled(addedSomething);
}

void MainWindow::showOsdTimer(bool onSeek)
{
    if (!onSeek || osdTimerOnSeek) {
        double time = mpvObject_->playTime();
        double length = mpvObject_->playLength();
        mpvObject_->showMessage(tr("%1 / %2").arg(Helpers::toDateFormatFixed(time,
                                                         Helpers::ShortFormat),
                                                  Helpers::toDateFormatFixed(length,
                                                         Helpers::ShortFormat)));
    }
}

QList<QUrl> MainWindow::doQuickOpenFileDialog()
{
    static QFileDialog::Options options = QFileDialog::Options();
#ifdef Q_OS_MAC
    options = QFileDialog::DontUseNativeDialog;
#endif
    QList<QUrl> urls;
    static QString filter;
    if (filter.isEmpty())
        filter = Helpers::fileOpenFilter();

    urls = QFileDialog::getOpenFileUrls(this, tr("Quick Open"), currentFile, filter, nullptr, options);
    return urls;
}

void MainWindow::showStepAndSubsButtons(bool show)
{
    ui->stepBackward->setVisible(show);
    ui->stepForward->setVisible(show);
    ui->verticalLine2->setVisible(show);
    ui->subs->setVisible(show);
}

QIcon MainWindow::createIconFromSvg(const QString &svgPath, int maxSize) const
{
    QIcon icon;
    QSvgRenderer svgRenderer(svgPath);

    // Render the SVG at multiple sizes and add to the QIcon
    for (int size = 16; size <= maxSize; ++size) {
        QPixmap pixmap(QSize(size, size));
        pixmap.fill(Qt::transparent); // Ensure transparency
        QPainter painter(&pixmap);
        svgRenderer.render(&painter);
        icon.addPixmap(pixmap);
    }
    return icon;
}

QPixmap MainWindow::renderPixmapFromSvg(const QString &path) const
{
    QSvgRenderer renderer(path);
    qreal devicePixelRatio = this->devicePixelRatioF();
    int iconSize = 16;
    QPixmap pixmap(iconSize * devicePixelRatio, iconSize * devicePixelRatio);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter);
    pixmap.setDevicePixelRatio(devicePixelRatio);
    return pixmap;
}

void MainWindow::httpQuickOpenFile()
{
    if (ui->actionFileOpenQuick->isEnabled())
        ui->actionFileOpenQuick->trigger();
}

void MainWindow::httpOpenFileUrl()
{
    if (ui->actionFileOpen->isEnabled())
        ui->actionFileOpen->trigger();
}

void MainWindow::httpSaveImage()
{
    if (ui->actionFileSaveImage->isEnabled())
        ui->actionFileSaveImage->trigger();
}

void MainWindow::httpSaveImageAuto()
{
    if (ui->actionFileSaveImageAuto->isEnabled())
        ui->actionFileSaveImageAuto->trigger();
}

void MainWindow::httpSaveThumbnails()
{
    if (ui->actionFileSaveThumbnails->isEnabled())
        ui->actionFileSaveThumbnails->trigger();
}

void MainWindow::httpClose()
{
    if (ui->actionFileClose->isEnabled())
        ui->actionFileClose->trigger();
}

void MainWindow::httpProperties()
{
    if (ui->actionFileProperties->isEnabled())
        ui->actionFileProperties->trigger();
}

void MainWindow::httpExit()
{
    if (ui->actionFileExit->isEnabled())
        ui->actionFileExit->trigger();
}

void MainWindow::httpPlay()
{
    if (ui->play->isEnabled())
        ui->play->click();
}

void MainWindow::httpPause()
{
    if (ui->pause->isEnabled())
        ui->pause->click();
}

void MainWindow::httpStop()
{
    if (ui->stop->isEnabled())
        ui->stop->click();
}

void MainWindow::httpFrameStep()
{
    if (ui->stepForward->isEnabled())
        ui->stepForward->click();
}

void MainWindow::httpFrameStepBack()
{
    if (ui->skipBackward->isEnabled())
        ui->stepBackward->click();
}

void MainWindow::httpIncreaseRate()
{
    if (ui->actionPlayRateIncrease->isEnabled())
        ui->actionPlayRateIncrease->trigger();
}

void MainWindow::httpDecreaseRate()
{
    if (ui->actionPlayRateDecrease->isEnabled())
        ui->actionPlayRateDecrease->trigger();
}

void MainWindow::httpQuickAddFavorite()
{
    if (ui->actionFavoritesAdd->isEnabled())
        ui->actionFavoritesAdd->trigger();
}

void MainWindow::httpOrganizeFavories()
{
    if (ui->actionFavoritesOrganize->isEnabled())
        ui->actionFavoritesOrganize->trigger();
}

void MainWindow::httpToggleCaptionMenu()
{
    if (ui->actionViewHideMenu->isEnabled())
        ui->actionViewHideMenu->trigger();
}

void MainWindow::httpToggleSeekBar()
{
    if (ui->actionViewHideSeekbar->isEnabled())
        ui->actionViewHideSeekbar->toggle();
}

void MainWindow::httpToogleControls()
{
    if (ui->actionViewHideControls->isEnabled())
        ui->actionViewHideControls->toggle();
}

void MainWindow::httpToggleInformation()
{
    if (ui->actionViewHideInformation->isEnabled())
        ui->actionViewHideInformation->toggle();
}

void MainWindow::httpToggleStatistics()
{
    if (ui->actionViewHideStatistics->isEnabled())
        ui->actionViewHideStatistics->toggle();
}

void MainWindow::httpToggleStatus()
{
    if (ui->actionViewHideStatus->isEnabled())
        ui->actionViewHideStatus->toggle();
}

void MainWindow::httpTogglePlaylistBar()
{
    if (ui->actionViewHidePlaylist->isEnabled())
        ui->actionViewHidePlaylist->toggle();
}

void MainWindow::httpViewMinimal()
{
    if (ui->actionViewPresetsMinimal->isEnabled())
        ui->actionViewPresetsMinimal->trigger();
}

void MainWindow::httpViewCompact()
{
    if (ui->actionViewPresetsCompact->isEnabled())
        ui->actionViewPresetsCompact->trigger();
}

void MainWindow::httpViewNormal()
{
    if (ui->actionViewPresetsNormal->isEnabled())
        ui->actionViewPresetsNormal->trigger();
}

void MainWindow::httpFullscreen()
{
    if (ui->actionViewFullscreen->isEnabled())
        ui->actionViewFullscreen->trigger();
}

void MainWindow::httpZoom25()
{
    if (ui->actionViewZoom025->isEnabled()) {
        ui->actionViewZoom025->setChecked(true);
        ui->actionViewZoom025->trigger();
    }
}

void MainWindow::httpZoom50()
{
    if (ui->actionViewZoom050->isEnabled()) {
        ui->actionViewZoom050->setChecked(true);
        ui->actionViewZoom050->trigger();
    }
}

void MainWindow::httpZoom100()
{
    if (ui->actionViewZoom100->isEnabled()) {
        ui->actionViewZoom100->setChecked(true);
        ui->actionViewZoom100->trigger();
    }
}

void MainWindow::httpZoom200()
{
    if (ui->actionViewZoom200->isEnabled()) {
        ui->actionViewZoom200->setChecked(true);
        ui->actionViewZoom200->trigger();
    }
}

void MainWindow::httpZoomAutoFit()
{
    if (ui->actionViewZoomAutofit->isEnabled()) {
        ui->actionViewZoomAutofit->setChecked(true);
        ui->actionViewZoomAutofit->trigger();
    }
}

void MainWindow::httpZoomAutoFitLarger()
{
    if (ui->actionViewZoomAutofitLarger->isEnabled()) {
        ui->actionViewZoomAutofitLarger->setChecked(true);
        ui->actionViewZoomAutofitLarger->trigger();
    }
}

void MainWindow::httpVolumeUp()
{
    if (ui->actionPlayVolumeUp->isEnabled())
        ui->actionPlayVolumeUp->trigger();
}

void MainWindow::httpVolumeDown()
{
    if (ui->actionPlayVolumeDown->isEnabled())
        ui->actionPlayVolumeDown->trigger();
}

void MainWindow::httpVolumeMute()
{
    if (ui->actionPlayVolumeMute->isEnabled())
        ui->actionPlayVolumeMute->toggle();
}

void MainWindow::httpNextAudio()
{
    if (ui->actionPlayAudioTrackNext->isEnabled())
        ui->actionPlayAudioTrackNext->trigger();
}

void MainWindow::httpPrevAudio()
{
    if (ui->actionPlayAudioTrackPrevious->isEnabled())
        ui->actionPlayAudioTrackPrevious->trigger();
}

void MainWindow::httpNextSubtitle()
{
    if (ui->actionPlaySubtitlesNext->isEnabled())
        ui->actionPlaySubtitlesNext->trigger();
}

void MainWindow::httpPrevSubtitle()
{
    if (ui->actionPlaySubtitlesPrevious->isEnabled())
        ui->actionPlaySubtitlesPrevious->trigger();
}

void MainWindow::httpOnOffSubtitles()
{
    if (ui->subs->isEnabled())
        ui->subs->click();
}

void MainWindow::httpAfterPlaybackNothing()
{
    if (ui->actionPlayAfterOnceNothing->isEnabled()) {
        ui->actionPlayAfterOnceNothing->setChecked(true);
        ui->actionPlayAfterOnceNothing->trigger();
    }
}

void MainWindow::httpAfterPlaybackPlayNext()
{
    if (ui->actionPlayAfterOnceNothing->isEnabled()) {
        ui->actionPlayAfterOnceNothing->setChecked(true);
        ui->actionPlayAfterOnceNothing->trigger();
    }
    if (ui->actionPlayAfterAlwaysNext->isEnabled()) {
        ui->actionPlayAfterAlwaysNext->setChecked(true);
        ui->actionPlayAfterAlwaysNext->trigger();
    }
}

void MainWindow::httpAfterPlaybackExit()
{
    if (ui->actionPlayAfterOnceExit->isEnabled()) {
        ui->actionPlayAfterOnceExit->setChecked(true);
        ui->actionPlayAfterOnceExit->trigger();
    }
}

void MainWindow::httpAfterPlaybackStandBy()
{
    if (ui->actionPlayAfterOnceStandby->isEnabled()) {
        ui->actionPlayAfterOnceStandby->setChecked(true);
        ui->actionPlayAfterOnceStandby->trigger();
    }
}

void MainWindow::httpAfterPlaybackHibernate()
{
    if (ui->actionPlayAfterOnceHibernate->isEnabled()) {
        ui->actionPlayAfterOnceHibernate->setChecked(true);
        ui->actionPlayAfterOnceHibernate->trigger();
    }
}

void MainWindow::httpAfterPlaybackShutdown()
{
    if (ui->actionPlayAfterOnceShutdown->isEnabled()) {
        ui->actionPlayAfterOnceShutdown->setChecked(true);
        ui->actionPlayAfterOnceShutdown->trigger();
    }
}

void MainWindow::httpAfterPlaybackLogOff()
{
    if (ui->actionPlayAfterOnceLogoff->isEnabled()) {
        ui->actionPlayAfterOnceLogoff->setChecked(true);
        ui->actionPlayAfterOnceLogoff->trigger();
    }
}

void MainWindow::httpAfterPlaybackLock()
{
    if (ui->actionPlayAfterOnceLock->isEnabled()) {
        ui->actionPlayAfterOnceLock->setChecked(true);
        ui->actionPlayAfterOnceLock->trigger();
    }
}

void MainWindow::setFreestanding(bool freestanding)
{
    freestanding_ = freestanding;
    setMediaTitleWithFilename(QString(), QString());
}

void MainWindow::setNoVideoSize(const QSize &size)
{
    noVideoSize_ = size.expandedTo(QSize(500,270));
    updateSize();
}

void MainWindow::setTrayIcon(bool enabled)
{
    if (!trayIcon->isVisible() && enabled)
        trayIcon->show();
    else if (trayIcon->isVisible() && !enabled)
        trayIcon->hide();
}

void MainWindow::setTitleBarFormat(Helpers::TitlePrefix titlebarFormat)
{
    titlebarFormat_ = titlebarFormat;
    setMediaTitleWithFilename(currentFileTitle, currentFileName);
}

void MainWindow::setWindowedMouseMap(const MouseStateMap &map)
{
    mouseMapWindowed = map;
}

void MainWindow::setFullscreenMouseMap(const MouseStateMap &map)
{
    mouseMapFullscreen = map;
}

void MainWindow::setRecentDocuments(const QList<TrackInfo> &tracks)
{
    bool isEmpty = tracks.isEmpty();
    ui->menuFileRecent->clear();
    ui->menuFileRecent->setDisabled(isEmpty);
    if (isEmpty)
        return;

    addRecentDocumentsEntries(tracks, ui->menuFileRecent, 0, 20);

    if (tracks.count() > 20) {
        LogStream(logModule) << "tracks.count(): " << tracks.count();
        Logger::log(logModule, "setRecentDocuments > 20");
        QMenu *moreRecentsMenu = new QMenu(tr("More Files"));
        addRecentDocumentsEntries(tracks, moreRecentsMenu, 20, 50);
        ui->menuFileRecent->addSeparator();
        ui->menuFileRecent->addMenu(moreRecentsMenu);
        Logger::log(logModule, "setRecentDocuments > 20 done");
    }

    ui->menuFileRecent->addSeparator();
    ui->menuFileRecent->addAction(ui->actionFileRecentClear);
}

void MainWindow::addRecentDocumentsEntries(const QList<TrackInfo> &tracks, QMenu *menu, int start, int end)
{
    for (int i = start; i < tracks.count() && i < end; i++) {
        TrackInfo track = tracks[i];
        QString displayString;
        if (track.url.isLocalFile())
            displayString = track.url.fileName();
        else
            displayString = track.title;
        displayString.truncate(100);
        QAction *a = new QAction(QString("%1").arg(displayString),
                                 this);
        connect(a, &QAction::triggered, this, [this, track]() {
            emit recentOpened(track);
        });
        menu->addAction(a);
    }
}

void MainWindow::setControlsInFullscreen(bool hide, int showWhen, int showWhenDuration,
                                         bool setControlsInFullscreen = true)
{
    if (hide) {
        Helpers::ControlHiding method = static_cast<Helpers::ControlHiding>(showWhen);
        if (method == Helpers::ShowWhenMoving && !showWhenDuration) {
            setBottomAreaBehavior(Helpers::ShowWhenHovering);
            setBottomAreaHideTime(0);
        } else {
            setBottomAreaBehavior(method);
            setBottomAreaHideTime(showWhenDuration);
        }
    } else
        setBottomAreaBehavior(Helpers::AlwaysShow);

    if (setControlsInFullscreen)
        ui->actionViewHideControlsInFullscreen->setChecked(!hide);
}

void MainWindow::setFavoriteTracks(QList<TrackInfo> files, QList<TrackInfo> streams)
{
    auto addAction = [&](QAction *a) {
        menuFavoritesTail.append(a);
        ui->menuFavorites->addAction(a);
    };
    auto addSeparator = [&]() {
        auto a = new QAction(this);
        a->setSeparator(true);
        addAction(a);
    };
    auto addNotice = [&](QString notice) {
        auto a = new QAction(this);
        a->setText(notice);
        a->setEnabled(false);
        addAction(a);
    };
    auto addTracks = [&](const QList<TrackInfo> &tracks) {
        for (const auto &t : tracks) {
            auto a = new QAction(this);
            a->setText(t.title);
            connect(a, &QAction::triggered,
                    a, [t, this]() {
                emit this->recentOpened(t);
            });
            addAction(a);
        }
    };

    for (auto a : std::as_const(menuFavoritesTail))
        delete a;
    menuFavoritesTail.clear();

    addSeparator();
    if (files.empty())
        addNotice(tr("No files favorited"));
    else
        addTracks(files);
    addSeparator();
    if (streams.empty())
        addNotice(tr("No streams favorited"));
    else
        addTracks(streams);
}

void MainWindow::setIconTheme(IconThemer::FolderMode folderMode, QString fallbackFolder, QString customFolder)
{
    themer.setIconFolders(folderMode, fallbackFolder, customFolder);
}

void MainWindow::setHighContrastWidgets(bool yes)
{
    positionSlider_->setHighContrast(yes);
    volumeSlider_->setHighContrast(yes);
}

void MainWindow::setInfoColors(const QColor &foreground, const QColor &background)
{
    QColor fg = foreground.isValid() ? foreground : QColor(255,255,255);
    QColor bg = background.isValid() ? background : QColor(0,0,0);
    QString css = QString("color: %1; background: %2;").arg(fg.name(), bg.name());
    ui->infoSection->setStyleSheet(css);
}

void MainWindow::setTime(double time, double length)
{
    positionSlider_->setMaximum(length);
    positionSlider_->setValue(time);
    statusTime->setTime(time, length);
}

void MainWindow::setMediaTitleWithFilename(const QString& title, const QString& filename)
{
    currentFileTitle = title;
    currentFileName = filename;
    QString newTitle = title;
    QString windowTitle(textWindowTitle);
    if (freestanding_)
        windowTitle.append(tr(" [Freestanding]"));

    switch(titlebarFormat_) {
        case PrefixFullPath:
            newTitle = currentFile.toString();
            break;
        case PrefixFileName:
            if (!filename.isEmpty())
                newTitle = filename;
            break;
        case PrefixFileTitle:
            break;
        case NoPrefix:
            newTitle.clear();
            break;
    }
    if (!newTitle.isEmpty())
        windowTitle.prepend(" - ").prepend(newTitle);

    setWindowTitle(windowTitle);
    ui->infoTitle->setText(!newTitle.isEmpty() ? newTitle : "-");
}

void MainWindow::setChapterTitle(QString title)
{
    ui->chapter->setText(!title.isEmpty() ? title : "-");
}

void MainWindow::setAspectName(QString aspectName)
{
    ui->actionVideoAspectName->setText(!aspectName.isEmpty() ? aspectName : tr("Unknown"));
}

void MainWindow::setVideoSize(QSize size)
{
    (void)size;
    updateSize();
}

void MainWindow::setVolumeStep(int stepSize)
{
    volumeStep = stepSize;
}

void MainWindow::setSizeFactor(double factor)
{
    sizeFactor_ = factor;
    if (sizeFactor_ != 0)
        updateSize();
}

void MainWindow::setFitFactor(double fitFactor)
{
    fitFactor_ = fitFactor;
}

void MainWindow::setZoomMode(ZoomMode mode)
{
    zoomMode = mode;
}

void MainWindow::setZoomPreset(int which, double fitFactor)
{
    double factor[] = { 1.0, 1.0, 1.0, 1.0,
                        0.25, 0.5, 0.75, 1.0, 1.5, 2.0, 3.0, 4.0 };
    ZoomMode mode[] = { Autofit, AutofitLarger, AutofitSmaller, FitToWindow,
                        RegularZoom, RegularZoom, RegularZoom, RegularZoom,
                        RegularZoom, RegularZoom, RegularZoom, RegularZoom };
    QAction *action[] = {
        ui->actionViewZoomAutofit, ui->actionViewZoomAutofitLarger,
        ui->actionViewZoomAutofitSmaller, ui->actionViewZoomDisable,
        ui->actionViewZoom025, ui->actionViewZoom050, ui->actionViewZoom075,
        ui->actionViewZoom100, ui->actionViewZoom150, ui->actionViewZoom200,
        ui->actionViewZoom300, ui->actionViewZoom400 };

    if (fitFactor >= 0.0)
        setFitFactor(fitFactor);
    setZoomMode(mode[which + 4]);
    setSizeFactor(factor[which + 4]);
    action[which+4]->setChecked(true);
}

void MainWindow::setZoomCenter(bool yes)
{
    zoomCenter = yes;
}

void MainWindow::setFullscreenName(QString screenName)
{
    fullscreenName = screenName;
}

void MainWindow::setFullscreenOnPlay(bool onPlay)
{
    fullscreenOnPlay = onPlay;
}

void MainWindow::setFullscreenExitOnEnd(bool exitOnEnd)
{
    fullscreenExitOnEnd = exitOnEnd;
}

void MainWindow::setMouseHideTimeFullscreen(int msec)
{
    mouseHideTimeFullscreen = msec;
    updateMouseHideTime();
}

void MainWindow::setMouseHideTimeWindowed(int msec)
{
    mouseHideTimeWindowed = msec;
    updateMouseHideTime();
}

void MainWindow::setBottomAreaBehavior(ControlHiding method)
{
    bottomAreaBehavior = method;
    if (fullscreenMode_)
        checkBottomArea(QCursor::pos());
}

void MainWindow::setBottomAreaHideTime(int milliseconds)
{
    bottomAreaHideTime = milliseconds;
    hideTimer.setInterval(milliseconds);
}

void MainWindow::setVideoPreview(bool enable)
{
    if (!videoPreview && enable) {
        videoPreview = new VideoPreview(this);
        setVideoPreviewItem(currentFile);
    }
    else if (videoPreview && !enable) {
        videoPreview->deleteLater();
        videoPreview = nullptr;
    }
}

void MainWindow::setTimeTooltip(bool shown, bool above)
{
    if (!tooltip && shown) {
        tooltip = new Tooltip(this);
    } else if (tooltip && !shown) {
        delete tooltip;
        tooltip = nullptr;
    }
    timeTooltipShown = shown;
    timeTooltipAbove = above;
}

void MainWindow::setOsdTimerOnSeek(bool enabled)
{
    osdTimerOnSeek = enabled;
}

void MainWindow::setFullscreenHidePanels(bool hidden)
{
    fullscreenHidePanels = hidden;
    if (fullscreenMode_ && !playlistWindow_->isFloating()) {
        if (hidden && playlistWindow_->isVisible()) {
            playlistWindow_->hide();
            updateBottomAreaGeometry();
        } else if (!hidden && playlistWindow_->isHidden()) {
            playlistWindow_->show();
            updateBottomAreaGeometry();
        }
    }
}

void MainWindow::setPlaybackState(PlaybackManager::PlaybackState state)
{
    // Update the fullscreen state
    if (state == PlaybackManager::StoppedState) {
        if (fullscreenExitOnEnd && fullscreenMode_ == true) {
            ui->actionViewFullscreen->setChecked(false);
        }
    } else if (state == PlaybackManager::PlayingState) {
        if (fullscreenOnPlay && fullscreenMode_ == false) {
            ui->actionViewFullscreen->setChecked(true);
        }
    }

    // Update the UI
    ui->status->setText(state==PlaybackManager::StoppedState ? tr("Stopped") :
                        state==PlaybackManager::PausedState ? tr("Paused") :
                        state==PlaybackManager::PlayingState ? tr("Playing") :
                        state==PlaybackManager::BufferingState ? tr("Buffering") :
                                                                 tr("Unknown"));
    isPlaying = state != PlaybackManager::StoppedState;
    isPaused = state == PlaybackManager::PausedState;
    setUiEnabledState(state != PlaybackManager::StoppedState);
    if (isPaused) {
        ui->actionPlayPause->setText(tr("&Play"));
        ui->pause->setChecked(true);
    }
    else
        ui->actionPlayPause->setText(tr("&Pause"));

    if (state == PlaybackManager::WaitingState) {
        mpvObject_->setLoopPoints(-1, -1);
        positionSlider_->setLoopA(-1);
        positionSlider_->setLoopB(-1);
        ui->actionPlayLoopUse->setChecked(false);
    }
    ui->play->setChecked(state == PlaybackManager::PlayingState);
    ui->stop->setChecked(state == PlaybackManager::StoppedState);
    updateOnTop();
}

void MainWindow::setPlaybackType(PlaybackManager::PlaybackType type)
{
    setUiEnabledState(type != PlaybackManager::None);
}

void MainWindow::disableChaptersMenus()
{
    ui->menuNavigateChapters->setEnabled(false);
}

void MainWindow::setChapters(QList<Chapter> chapters)
{
    positionSlider_->clearTicks();
    ui->menuNavigateChapters->clear();
    ui->menuNavigateChapters->setEnabled(false);
    if (chapters.isEmpty())
        return;
    ui->menuNavigateChapters->setEnabled(true);
    int64_t index = 0;
    for (const Chapter &chapter : chapters) {
        positionSlider_->setTick(chapter.time, chapter.title);
        QAction *action = new QAction(this);
        action->setText(chapter.title);
        connect (action, &QAction::triggered, this, [this,index]() {
           emit chapterSelected(index);
        });
        ui->menuNavigateChapters->addAction(action);
        ++index;
    }
}

void MainWindow::setAudioTracks(QList<Track> tracks)
{
    ui->menuPlayAudio->clear();
    ui->menuPlayAudio->setEnabled(false);
    audioTracksGroup = nullptr;
    hasAudio = !tracks.isEmpty();
    ui->actionPlayAudioTrackPrevious->setEnabled(hasAudio);
    ui->actionPlayAudioTrackNext->setEnabled(hasAudio);
    if (!hasAudio)
        return;
    ui->menuPlayAudio->setEnabled(true);
    audioTracksGroup = new QActionGroup(this);
    for (const Track &track : tracks) {
        QAction *action = new QAction(this);
        action->setText(track.title);
        action->setCheckable(true);
        action->setActionGroup(audioTracksGroup);
        int64_t index = track.id;
        connect(action, &QAction::triggered, this, [this,index] {
            emit audioTrackSelected(index, true);
        });
        ui->menuPlayAudio->addAction(action);
    }
    ui->menuPlayAudio->addSeparator();
    ui->menuPlayAudio->addMenu(ui->menuPlayAudioFilters);
    ui->menuPlayAudioFilters->addAction(ui->actionAudioFilterExtrastereo);
    ui->menuPlayAudioFilters->addAction(ui->actionAudioFilterAcompressor);
    ui->menuPlayAudioFilters->addAction(ui->actionAudioFilterCrossfeed);
    ui->menuPlayAudio->addAction(ui->actionPlayAudioTrackPrevious);
    ui->menuPlayAudio->addAction(ui->actionPlayAudioTrackNext);
    audioTracksGroup->actions().constFirst()->setChecked(true);
}

void MainWindow::setVideoTracks(QList<Track> tracks)
{
    ui->menuPlayVideo->clear();
    ui->menuPlayVideo->setEnabled(false);
    videoTracksGroup = nullptr;
    hasVideo = !tracks.isEmpty();
    if (!hasVideo)
        return;
    ui->menuPlayVideo->setEnabled(true);
    videoTracksGroup = new QActionGroup(this);
    for (const Track &track : tracks) {
        QAction *action = new QAction(this);
        action->setText(track.title);
        action->setCheckable(true);
        action->setActionGroup(videoTracksGroup);
        int64_t index = track.id;
        connect(action, &QAction::triggered, this, [this,index]() {
            emit videoTrackSelected(index, true);
        });
        ui->menuPlayVideo->addAction(action);
    }
    ui->menuPlayVideo->addSeparator();
    ui->menuPlayVideo->addMenu(ui->menuPlayVideoFilters);
    ui->menuPlayVideoFilters->addAction(ui->actionVideoFilterDeinterlace);
    ui->menuPlayVideo->addMenu(ui->menuPlayVideoAspect);
    ui->menuPlayVideoAspect->addAction(ui->actionVideoAspectName);
    ui->menuPlayVideoAspect->addSeparator();
    ui->menuPlayVideoAspect->addAction(ui->action43VideoAspect);
    ui->menuPlayVideoAspect->addAction(ui->actionDecreaseVideoAspect);
    ui->menuPlayVideoAspect->addAction(ui->actionIncreaseVideoAspect);
    ui->menuPlayVideoAspect->addAction(ui->actionResetVideoAspect);
    ui->menuPlayVideoAspect->addAction(ui->actionDisableVideoAspect);
    ui->menuPlayVideo->addMenu(ui->menuPlayVideoPanScan);
    ui->menuPlayVideoPanScan->addAction(ui->actionDecreasePanScan);
    ui->menuPlayVideoPanScan->addAction(ui->actionIncreasePanScan);
    ui->menuPlayVideoPanScan->addAction(ui->actionMinPanScan);
    ui->menuPlayVideoPanScan->addAction(ui->actionMaxPanScan);
    ui->menuPlayVideo->addMenu(ui->menuPlayVideoZoom);
    ui->menuPlayVideoZoom->addAction(ui->actionDecreaseZoom);
    ui->menuPlayVideoZoom->addAction(ui->actionIncreaseZoom);
    ui->menuPlayVideoZoom->addAction(ui->actionResetZoom);
    ui->menuPlayVideo->addMenu(ui->menuPlayVideoResize);
    ui->menuPlayVideoResize->addAction(ui->actionDecreaseWidth);
    ui->menuPlayVideoResize->addAction(ui->actionIncreaseWidth);
    ui->menuPlayVideoResize->addAction(ui->actionDecreaseHeight);
    ui->menuPlayVideoResize->addAction(ui->actionIncreaseHeight);
    ui->menuPlayVideoResize->addAction(ui->actionResetWidthHeight);
    ui->menuPlayVideo->addMenu(ui->menuPlayVideoMove);
    ui->menuPlayVideoMove->addAction(ui->actionMoveLeft);
    ui->menuPlayVideoMove->addAction(ui->actionMoveRight);
    ui->menuPlayVideoMove->addAction(ui->actionMoveUp);
    ui->menuPlayVideoMove->addAction(ui->actionMoveDown);
    ui->menuPlayVideoMove->addAction(ui->actionResetMove);
    ui->menuPlayVideo->addMenu(ui->menuPlayVideoRotate);
    ui->menuPlayVideoRotate->addAction(ui->actionRotateClockwise);
    ui->menuPlayVideoRotate->addAction(ui->actionRotateCounterclockwise);
    ui->menuPlayVideoRotate->addAction(ui->actionFlipHorizontal);
    ui->menuPlayVideoRotate->addAction(ui->actionResetRotate);


    videoTracksGroup->actions().constFirst()->setChecked(true);
    updateOnTop();
}

void MainWindow::setSubtitleTracks(QList<Track > tracks)
{
    ui->menuPlaySubtitles->clear();
    ui->menuPlaySubtitles->setEnabled(false);
    subtitleTracksGroup = nullptr;
    hasSubs = !tracks.isEmpty();
    ui->actionPlaySubtitlesEnabled->setEnabled(hasSubs);
    ui->subs->setEnabled(hasSubs);
    ui->actionPlaySubtitlesNext->setEnabled(hasSubs);
    ui->actionPlaySubtitlesPrevious->setEnabled(hasSubs);
    if (!hasSubs)
        return;
    ui->menuPlaySubtitles->setEnabled(true);
    subtitleTracksGroup = new QActionGroup(this);
    for (const Track &track : tracks) {
        QAction *action = new QAction(this);
        action->setText(track.title);
        action->setCheckable(true);
        action->setActionGroup(subtitleTracksGroup);
        int64_t index = track.id;
        connect(action, &QAction::triggered, this, [this,index]() {
            emit subtitleTrackSelected(index, true);
        });
        ui->menuPlaySubtitles->addAction(action);
    }
    ui->menuPlaySubtitles->addSeparator();
    ui->menuPlaySubtitles->addAction(ui->actionPlaySubtitlesEnabled);
    ui->menuPlaySubtitles->addAction(ui->actionPlaySubtitlesPrevious);
    ui->menuPlaySubtitles->addAction(ui->actionPlaySubtitlesNext);
    ui->menuPlaySubtitles->addAction(ui->actionPlaySubtitlesCopy);
    ui->menuPlaySubtitles->addAction(ui->actionDecreaseSubtitlesDelay);
    ui->menuPlaySubtitles->addAction(ui->actionIncreaseSubtitlesDelay);
    ui->menuPlaySubtitles->addMenu(ui->menuPlaySubtitlesMove);
    ui->menuPlaySubtitlesMove->addAction(ui->actionMoveSubtitlesUp);
    ui->menuPlaySubtitlesMove->addAction(ui->actionMoveSubtitlesDown);
    subtitleTracksGroup->actions().constFirst()->setChecked(true);
}

void MainWindow::audioTrackSet(int64_t id)
{
    if (audioTracksGroup != nullptr && id <= audioTracksGroup->actions().length()) {
        if (id <= 0)
            id = 1;
        audioTracksGroup->actions().constData()[static_cast <int> (id) -1]->setChecked(true);
    }
}

void MainWindow::videoTrackSet(int64_t id)
{
    if (videoTracksGroup != nullptr && id <= videoTracksGroup->actions().length()) {
        if (id <= 0)
            id = 1;
        videoTracksGroup->actions().constData()[static_cast <int> (id) -1]->setChecked(true);
    }
}

void MainWindow::subtitleTrackSet(int64_t id)
{
    if (subtitleTracksGroup != nullptr && id <= subtitleTracksGroup->actions().length()) {
        if (id <= 0)
            id = 1;
        subtitleTracksGroup->actions().constData()[static_cast <int> (id) -1]->setChecked(true);
    }
}

void MainWindow::setSubtitleText(QString subText)
{
    subtitleText = subText;
}

void MainWindow::setVolume(int level, bool onInit)
{
    volumeSlider_->setValue(level);
    emit volumeChanged(level, onInit);
}

void MainWindow::setVolumeDouble(double level)
{
    volumeSlider_->setValue(level*100);
    emit volumeChanged(static_cast<int64_t>(level*100));
}

void MainWindow::setVolumeMax(int level)
{
    volumeSlider_->setMaximum(level);
}

void MainWindow::setSubtitlesEnabled(bool enabled, bool onInit)
{
    emit subtitlesEnabled(enabled, onInit);
    ui->actionPlaySubtitlesEnabled->setChecked(enabled);
    ui->subs->setChecked(!enabled);
}

void MainWindow::setSubtitlesDelayStep(int subtitlesDelayStep)
{
    this->subtitlesDelayStep = subtitlesDelayStep;
}

void MainWindow::setTimeShortMode(bool shortened)
{
    statusTime->setShortMode(shortened);
    timeShortMode = shortened;
}

void MainWindow::setTimeRemainingMode(bool isRemaining)
{
    statusTime->setRemainingMode(isRemaining);
    timeRemainingMode = isRemaining;
}

void MainWindow::setTimePercentageMode(bool isPercentage)
{
    statusTime->setPercentMode(isPercentage);
    timePercentageMode = isPercentage;
}

void MainWindow::resetPlayAfterOnce()
{
    ui->actionPlayAfterOnceNothing->setChecked(true);
}

void MainWindow::setPlayAfterAlways(AfterPlayback action)
{
    QMap<Helpers::AfterPlayback, QAction*> map {
        { Helpers::PlayNextAfter, ui->actionPlayAfterAlwaysNext },
        { Helpers::ExitAfter, ui->actionPlayAfterOnceExit }
    };
    if (map.contains(action))
        map[action]->setChecked(true);
}

void MainWindow::setPlayAfterAlwaysDefault(Helpers::AfterPlayback action)
{
    static bool setOnce = false;
    if (!setOnce) {
        setOnce = true;
        if (action == Helpers::RepeatAfter)
            emit repeatAfter();
        else
            setPlayAfterAlways(action);
    }
}

void MainWindow::setFps(double fps)
{
    ui->framerate->setText(std::isnan(fps) ? "-" : QString::number(fps, 'g', 6));
}

void MainWindow::setAvsync(double sync)
{
    ui->avsync->setText(std::isnan(sync) ? "-" : QString::number(sync, 'f', 3));
}

void MainWindow::setDisplayFramedrops(int64_t count)
{
    displayDrops = count;
    updateFramedrops();
}

void MainWindow::setDecoderFramedrops(int64_t count)
{
    decoderDrops = count;
    updateFramedrops();
}

void MainWindow::setAudioBitrate(double bitrate)
{
    audioBitrate = bitrate;
    updateBitrate();
}

void MainWindow::setVideoBitrate(double bitrate)
{
    videoBitrate = bitrate;
    updateBitrate();
}

void MainWindow::setIsVideo(bool isVideo)
{
    isVideo_ = isVideo;
    showStepAndSubsButtons(isVideo);
}

void MainWindow::setVideoPreviewItem(QUrl itemUrl)
{
    currentFile = itemUrl;
    isVideo_ = false;
    if (videoPreview)
        videoPreview->openFile(itemUrl);
}

void MainWindow::logWindowClosed()
{
    ui->actionViewHideLog->setChecked(false);
}

void MainWindow::libraryWindowClosed()
{
    ui->actionViewHideLibrary->setChecked(false);
}

void MainWindow::mpvObject_mouseReleased()
{
    if (!isPlaying) {
        emit playCurrentItemRequested();
        return;
    }
}

void MainWindow::setPlaylistVisibleState(bool yes) {
    Logger::log(logModule, "setPlaylistVisibleState");
    if (fullscreenMode_ || !ui || mainwindowIsClosing)
        return;
    ui->actionFileOpenQuick->setText(yes ? tr("&Quick Add To Playlist")
                                         : tr("&Quick Open File"));
    ui->actionViewHidePlaylist->setChecked(yes);
}

void MainWindow::setPlaylistQuickQueueMode(bool yes)
{
    if (!ui)
        return;
    ui->actionPlaylistShowQuickQueue->setChecked(yes);
    ui->actionPlaylistQuickQueue->setEnabled(!yes);
    ui->actionPlaylistQueueVisible->setEnabled(!yes);
}

void MainWindow::on_actionFileOpenQuick_triggered()
{
    QList<QUrl> urls = doQuickOpenFileDialog();
    if (urls.isEmpty())
        return;
    emit severalFilesOpened(urls, true);
}

void MainWindow::on_actionFileOpen_triggered()
{
    OpenFileDialog d;
    if (!d.exec())
        return;

    emit fileOpened(d.file(), d.subs());
}

void MainWindow::on_actionFileOpenDvdbd_triggered()
{
    static QFileDialog::Options options = QFileDialog::ShowDirsOnly;
#ifdef Q_OS_MAC
    options |= QFileDialog::DontUseNativeDialog;
#endif
    QUrl dir;
    static QUrl lastDir;
    dir = QFileDialog::getExistingDirectoryUrl(this, tr("Open Directory"),
                                               lastDir, options);
    if (dir.isEmpty())
        return;
    lastDir = dir;
    emit dvdbdOpened(dir);
}

void MainWindow::on_actionFileOpenDirectory_triggered()
{
    static QFileDialog::Options options = QFileDialog::ShowDirsOnly;
#ifdef Q_OS_MAC
    options |= QFileDialog::DontUseNativeDialog;
#endif
    QUrl url;
    static QUrl lastDir;
    url = QFileDialog::getExistingDirectoryUrl(this, tr("Open Directory"),
                                               lastDir, options);
    if (url.isEmpty())
        return;
    lastDir = url;

    // If we pass the directory and let Helpers::filterUrls handle it,
    // we may freeze if the user selects the root folder.  So only
    // pass through the folder contents for now.  To expand all
    // subfolders, Drag-and-drop or pass folders through the cli.
    QDir dir(url.toLocalFile());
    QList<QUrl> list;
    QFileInfoList f = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files);
    for (auto &file : f)
        list.append(QUrl::fromLocalFile(file.absoluteFilePath()));
    emit severalFilesOpened(list);
}

void MainWindow::on_actionFileOpenNetworkStream_triggered()
{
    QInputDialog *qid = new QInputDialog(this);
    qid->setAttribute(Qt::WA_DeleteOnClose);
    qid->setWindowModality(Qt::WindowModal);
    qid->setWindowTitle(tr("Enter Network Stream"));
    qid->setLabelText(tr("Network Stream"));
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    if (mimeData->hasText() && QUrl::fromUserInput(mimeData->text()).isValid())
        qid->setTextValue(QUrl::fromUserInput(mimeData->text()).toString());
    qid->resize(500, qid->height());
    connect(qid, &QInputDialog::accepted, this, [this, qid] () {
        emit streamOpened(QUrl::fromUserInput(qid->textValue()));
    });
    qid->show();
}

void MainWindow::on_actionFileRecentClear_triggered()
{
    emit recentClear();
}

void MainWindow::on_actionFileSaveImage_triggered()
{
    emit takeImage(Helpers::SubsRender);
}

void MainWindow::on_actionFileSaveImageAuto_triggered()
{
    emit takeImageAutomatically(Helpers::SubsRender);
}

void MainWindow::on_actionFileSavePlainImage_triggered()
{
    emit takeImage(Helpers::VideoRender);
}

void MainWindow::on_actionFileSavePlainImageAuto_triggered()
{
    emit takeImageAutomatically(Helpers::VideoRender);
}

void MainWindow::on_actionFileSaveWindowImage_triggered()
{
    emit takeImage(Helpers::WindowRender);
}

void MainWindow::on_actionFileSaveWindowImageAuto_triggered()
{
    emit takeImageAutomatically(Helpers::WindowRender);
}

void MainWindow::on_actionFileLoadSubtitle_triggered()
{
    QUrl url;
    static QUrl lastUrl;
    static QFileDialog::Options options;
#ifdef Q_OS_MAC
    options = QFileDialog::DontUseNativeDialog;
#endif
    url = QFileDialog::getOpenFileUrl(this, tr("Open Subtitle"), lastUrl, "", nullptr, options);
    if (url.isEmpty())
        return;
    lastUrl = url;
    emit subtitlesLoaded(url);
}

void MainWindow::on_actionFileSubtitleDatabaseSearch_triggered() const
{
    QString fileNameNoExt = QFileInfo(currentFile.fileName()).completeBaseName();
    QDesktopServices::openUrl(QUrl("https://www.opensubtitles.org/search2/moviename-" +
                                   fileNameNoExt + "/sublanguageid-all"));
}

void MainWindow::on_actionFileClose_triggered()
{
    on_actionPlayStop_triggered();
}

void MainWindow::on_actionFileExit_triggered()
{
    close();
}

void MainWindow::on_actionViewHideMenu_triggered()
{
    // View/hide are unmanaged when in fullscreen mode
    if (fullscreenMode_)
        return;

    DecorationState nextState[] = { NoMenu, NoDecorations, AllDecorations };
    setUiDecorationState(nextState[static_cast<int>(decorationState_)]);
    updateSize();
}

void MainWindow::on_actionViewHideSeekbar_toggled(bool checked)
{
    if (checked && ui->seekbar->isHidden())
        ui->seekbar->show();
    else if (!checked && ui->seekbar->isVisible())
        ui->seekbar->hide();
    ui->controlSection->adjustSize();
    updateSize();
}

void MainWindow::on_actionViewHideControls_toggled(bool checked)
{
    if (checked && ui->controlbar->isHidden())
        ui->controlbar->show();
    else if (!checked && ui->controlbar->isVisible())
        ui->controlbar->hide();
    ui->controlSection->adjustSize();
    updateSize();
}

void MainWindow::on_actionViewHideInformation_toggled(bool checked)
{
    Q_UNUSED(checked)
    updateInfostats();
    updateSize();
}

void MainWindow::on_actionViewHideStatistics_toggled(bool checked)
{
    Q_UNUSED(checked)
    updateInfostats();
    updateSize();
}

void MainWindow::on_actionViewHideStatus_toggled(bool checked)
{
    if (checked && ui->statusbar->isHidden())
        ui->statusbar->show();
    else if (!checked && ui->statusbar->isVisible())
        ui->statusbar->hide();
    updateSize();
}

void MainWindow::on_actionViewHideSubresync_toggled(bool checked)
{
    (void)checked;

    updateSize();
}

void MainWindow::on_actionViewHidePlaylist_toggled(bool checked)
{
    Logger::log(logModule, "on_actionViewHidePlaylist_toggled");
    if (fullscreenMode_ && fullscreenHidePanels)
        return;

    if (isMinimized())
        return;

    if (checked != playlistWindow_->isVisible())
        playlistWindow_->setVisible(checked);

    if (fullscreenMode_ && !fullscreenHidePanels)
        updateBottomAreaGeometry();

    updateSize();
}

void MainWindow::on_actionViewHideCapture_toggled(bool checked)
{
    (void)checked;

    updateSize();
}

void MainWindow::on_actionViewHideNavigation_toggled(bool checked)
{
    (void)checked;

    updateSize();
}


void MainWindow::on_actionViewHideLog_toggled(bool checked)
{
    if (checked)
        emit showLogWindow();
    else
        emit hideLogWindow();
}

void MainWindow::on_actionViewHideLibrary_toggled(bool checked)
{
    if (checked)
        emit showLibraryWindow();
    else
        emit hideLibraryWindow();
}

void MainWindow::on_actionViewHideControlsInFullscreen_toggled(bool checked)
{
    emit fullscreenHideControls(!checked);
}

void MainWindow::on_actionViewMusicMode_toggled(bool checked)
{
    if (!playlistWindow_->isFloating()) {
        if (checked) {
            playlistWindow_->setFeatures(playlistWindow_->features() & ~QDockWidget::DockWidgetFloatable
                                                                     & ~QDockWidget::DockWidgetMovable
                                                                     & ~QDockWidget::DockWidgetClosable);
        }
        else {
            playlistWindow_->setFeatures(playlistWindow_->features() | QDockWidget::DockWidgetFloatable
                                                                     | QDockWidget::DockWidgetMovable
                                                                     | QDockWidget::DockWidgetClosable);
            playlistWindow_->setMinimumSize(QSize(0, 0));
            playlistWindow_->setMaximumSize(QSize(65550, 65550));
        }
        playlistWindow_->setVisible(checked);
        ui->actionViewHidePlaylist->setEnabled(!checked);
        mpvObject_->setCachedMpvOption("audio-display", checked ? "no" : "embedded-first");
        resizePlaylistToFit();
    }
}

void MainWindow::on_actionViewPresetsMinimal_triggered()
{
    setUiDecorationState(NoDecorations);
    ui->actionViewHideSeekbar->setChecked(false);
    ui->actionViewHideControls->setChecked(false);
    ui->actionViewHideInformation->setChecked(false);
    ui->actionViewHideStatistics->setChecked(false);
    ui->actionViewHideStatus->setChecked(false);
    ui->actionViewHideSubresync->setChecked(false);
    ui->actionViewHideCapture->setChecked(false);
    ui->actionViewHideNavigation->setChecked(false);
}

void MainWindow::on_actionViewPresetsCompact_triggered()
{
    // we should set our menu state to something like Framed, but we can't
    // reliably do that across window managers.
    setUiDecorationState(NoDecorations);
    ui->actionViewHideMenu->setChecked(true);
    ui->actionViewHideSeekbar->setChecked(true);
    ui->actionViewHideControls->setChecked(false);
    ui->actionViewHideInformation->setChecked(false);
    ui->actionViewHideStatistics->setChecked(false);
    ui->actionViewHideStatus->setChecked(false);
    ui->actionViewHideSubresync->setChecked(false);
    ui->actionViewHideCapture->setChecked(false);
    ui->actionViewHideNavigation->setChecked(false);
}

void MainWindow::on_actionViewPresetsNormal_triggered()
{
    setUiDecorationState(AllDecorations);
    ui->actionViewHideMenu->setChecked(true);
    ui->actionViewHideSeekbar->setChecked(true);
    ui->actionViewHideControls->setChecked(true);
    ui->actionViewHideInformation->setChecked(false);
    ui->actionViewHideStatistics->setChecked(false);
    ui->actionViewHideStatus->setChecked(true);
    ui->actionViewHideSubresync->setChecked(false);
    ui->actionViewHideCapture->setChecked(false);
    ui->actionViewHideNavigation->setChecked(false);
}

void MainWindow::on_actionViewOSDNone_triggered()
{
    mpvObject_->showStatsPage(-1);
    ui->actionViewOSDTimer->setEnabled(false);
}

void MainWindow::on_actionViewOSDMessages_triggered()
{
    mpvObject_->showStatsPage(0);
    ui->actionViewOSDTimer->setEnabled(true);
}

void MainWindow::on_actionViewOSDStatistics_triggered()
{
    mpvObject_->showStatsPage(1);
    ui->actionViewOSDTimer->setEnabled(false);
}

void MainWindow::on_actionViewOSDInputCacheStats_triggered()
{
    mpvObject_->showStatsPage(3);
    ui->actionViewOSDTimer->setEnabled(false);
}

void MainWindow::on_actionViewOSDCycle_triggered()
{
    QAction *nextOsdAction;
    int newpage;

    newpage = mpvObject_->cycleStatsPage();
    QMap<int, QAction*> pageToAction = {
        { -1, ui->actionViewOSDNone },
        { 0, ui->actionViewOSDMessages },
        { 1, ui->actionViewOSDStatistics },
        { 3, ui->actionViewOSDInputCacheStats }
    };
    nextOsdAction = pageToAction.value(newpage, nullptr);
    if (nextOsdAction)
        nextOsdAction->setChecked(true);
    ui->actionViewOSDTimer->setEnabled(newpage == 0);
}

void MainWindow::on_actionViewOSDTimer_triggered()
{
    showOsdTimer(false);
}

void MainWindow::on_actionViewFullscreen_toggled(bool checked)
{
    setFullscreenMode(checked);
    reparentBottomArea(checked);

    if (checked) {
        menuBar()->hide();
        if (fullscreenHidePanels && !playlistWindow_->isFloating())
            playlistWindow_->hide();
    } else {
        if (decorationState_ == AllDecorations)
            menuBar()->show();
        ui->bottomArea->show();
        if (ui->actionViewHidePlaylist->isChecked())
            playlistWindow_->show();
    }
}

void MainWindow::on_actionViewFullscreenEscape_triggered()
{
    ui->actionViewFullscreen->setChecked(false);
}

void MainWindow::on_actionViewZoom025_triggered()
{
    setZoomPreset(0);
    emit zoomPresetChanged(0);
}

void MainWindow::on_actionViewZoom050_triggered()
{
    setZoomPreset(1);
    emit zoomPresetChanged(1);
}

void MainWindow::on_actionViewZoom075_triggered()
{
    setZoomPreset(2);
    emit zoomPresetChanged(2);
}

void MainWindow::on_actionViewZoom100_triggered()
{
    setZoomPreset(3);
    emit zoomPresetChanged(3);
}

void MainWindow::on_actionViewZoom150_triggered()
{
    setZoomPreset(4);
    emit zoomPresetChanged(4);
}

void MainWindow::on_actionViewZoom200_triggered()
{
    setZoomPreset(5);
    emit zoomPresetChanged(5);
}

void MainWindow::on_actionViewZoom300_triggered()
{
    setZoomPreset(6);
    emit zoomPresetChanged(6);
}

void MainWindow::on_actionViewZoom400_triggered()
{
    setZoomPreset(7);
    emit zoomPresetChanged(7);
}

void MainWindow::on_actionViewZoomAutofit_triggered()
{
    setZoomPreset(-4);
    emit zoomPresetChanged(-4);
}

void MainWindow::on_actionViewZoomAutofitLarger_triggered()
{
    setZoomPreset(-3);
    emit zoomPresetChanged(-3);
}

void MainWindow::on_actionViewZoomAutofitSmaller_triggered()
{
    setZoomPreset(-2);
    emit zoomPresetChanged(-2);
}

void MainWindow::on_actionViewZoomDisable_triggered()
{
    setZoomPreset(-1);
    emit zoomPresetChanged(-1);
}

void MainWindow::on_action43VideoAspect_triggered()
{
    mpvObject_->disableVideoAspect(false);
    ui->actionDisableVideoAspect->setChecked(false);
    mpvObject_->setVideoAspectPreset(1.3333);
}

void MainWindow::on_actionDecreaseVideoAspect_triggered()
{
    mpvObject_->disableVideoAspect(false);
    ui->actionDisableVideoAspect->setChecked(false);
    mpvObject_->setVideoAspect(-0.02);
}

void MainWindow::on_actionIncreaseVideoAspect_triggered()
{
    mpvObject_->disableVideoAspect(false);
    ui->actionDisableVideoAspect->setChecked(false);
    mpvObject_->setVideoAspect(0.02);
}

void MainWindow::on_actionResetVideoAspect_triggered()
{
    mpvObject_->disableVideoAspect(false);
    ui->actionDisableVideoAspect->setChecked(false);
    mpvObject_->setVideoAspectPreset(-1);
}

void MainWindow::on_actionDisableVideoAspect_toggled(bool checked)
{
    mpvObject_->disableVideoAspect(checked);
    ui->actionDisableVideoAspect->setChecked(checked);
}

void MainWindow::on_actionDecreasePanScan_triggered()
{
    panScan -= 0.01;
    if (panScan < 0)
        panScan = 0;
    mpvObject_->setPanScan(panScan);
}

void MainWindow::on_actionIncreasePanScan_triggered()
{
    panScan += 0.01;
    if (panScan > 1)
        panScan = 1;
    mpvObject_->setPanScan(panScan);
}

void MainWindow::on_actionMinPanScan_triggered()
{
    panScan = 0;
    mpvObject_->setPanScan(panScan);
}

void MainWindow::on_actionMaxPanScan_triggered()
{
    panScan = 1;
    mpvObject_->setPanScan(panScan);
}

void MainWindow::on_actionDecreaseZoom_triggered()
{
    videoZoom -= 0.02;
    mpvObject_->setVideoZoom(videoZoom);
}

void MainWindow::on_actionIncreaseZoom_triggered()
{
    videoZoom += 0.02;
    mpvObject_->setVideoZoom(videoZoom);
}

void MainWindow::on_actionResetZoom_triggered()
{
    videoZoom = 0;
    mpvObject_->setVideoZoom(videoZoom);
}

void MainWindow::on_actionDecreaseWidth_triggered()
{
    mpvObject_->disableVideoAspect(false);
    ui->actionDisableVideoAspect->setChecked(false);
    videoWidthScale -= 0.01;
    if (videoWidthScale < 0)
        videoWidthScale = 0;
    mpvObject_->setVideoWidthScale(videoWidthScale);
}

void MainWindow::on_actionIncreaseWidth_triggered()
{
    mpvObject_->disableVideoAspect(false);
    ui->actionDisableVideoAspect->setChecked(false);
    videoWidthScale += 0.01;
    mpvObject_->setVideoWidthScale(videoWidthScale);
}

void MainWindow::on_actionDecreaseHeight_triggered()
{
    mpvObject_->disableVideoAspect(false);
    ui->actionDisableVideoAspect->setChecked(false);
    videoHeightScale -= 0.01;
    if (videoHeightScale < 0)
        videoHeightScale = 0;
    mpvObject_->setVideoHeightScale(videoHeightScale);
}

void MainWindow::on_actionIncreaseHeight_triggered()
{
    mpvObject_->disableVideoAspect(false);
    ui->actionDisableVideoAspect->setChecked(false);
    videoHeightScale += 0.01;
    mpvObject_->setVideoHeightScale(videoHeightScale);
}

void MainWindow::on_actionResetWidthHeight_triggered()
{
    mpvObject_->disableVideoAspect(false);
    ui->actionDisableVideoAspect->setChecked(false);
    videoWidthScale = 1;
    videoHeightScale = 1;
    mpvObject_->setVideoWidthScale(videoWidthScale);
    mpvObject_->setVideoHeightScale(videoHeightScale);
}

void MainWindow::on_actionMoveLeft_triggered()
{
    videoPanX -= 0.01;
    mpvObject_->setVideoPanX(videoPanX);
}

void MainWindow::on_actionMoveRight_triggered()
{
    videoPanX += 0.01;
    mpvObject_->setVideoPanX(videoPanX);
}

void MainWindow::on_actionMoveUp_triggered()
{
    videoPanY -= 0.01;
    mpvObject_->setVideoPanY(videoPanY);
}

void MainWindow::on_actionMoveDown_triggered()
{
    videoPanY += 0.01;
    mpvObject_->setVideoPanY(videoPanY);
}

void MainWindow::on_actionResetMove_triggered()
{
    videoPanX = 0;
    videoPanY = 0;
    mpvObject_->setVideoPanX(videoPanX);
    mpvObject_->setVideoPanY(videoPanY);
}


void MainWindow::on_actionViewOntopDefault_toggled(bool checked)
{
    if (!checked)
        return;
    onTopMode = OnTopDefault;
    updateOnTop();
    ui->actionViewOntopAlways->setShortcut(ui->actionViewOntopDefault->shortcut());
    ui->actionViewOntopDefault->setShortcut(QKeySequence());
}

void MainWindow::on_actionRotateClockwise_triggered()
{
    currentAngle = (currentAngle + 90) % 360;
    mpvObject_->setVideoRotate(currentAngle);
}

void MainWindow::on_actionRotateCounterclockwise_triggered()
{
    currentAngle = (currentAngle - 90);
    if (currentAngle < 0) currentAngle += 360;
    mpvObject_->setVideoRotate(currentAngle);
}

void MainWindow::on_actionFlipHorizontal_triggered()
{
    flipped = !flipped;
    mpvObject_->setVideoFlip(flipped);
}

void MainWindow::on_actionResetRotate_triggered()
{
    currentAngle = 0;
    flipped = false;
    mpvObject_->setVideoRotate(0);
    mpvObject_->setVideoFlip(false);
}

void MainWindow::on_actionViewOntopAlways_toggled(bool checked)
{
    if (!checked)
        return;
    onTopMode = AlwaysOnTop;
    updateOnTop();
    ui->actionViewOntopDefault->setShortcut(ui->actionViewOntopAlways->shortcut());
    ui->actionViewOntopAlways->setShortcut(QKeySequence());
}

void MainWindow::on_actionViewOntopPlaying_toggled(bool checked)
{
    if (!checked)
        return;
    onTopMode = OnTopWhenPlaying;
    updateOnTop();
}

void MainWindow::on_actionViewOntopVideo_toggled(bool checked)
{
    if (!checked)
        return;
    onTopMode = OnTopForVideos;
    updateOnTop();
}

void MainWindow::on_actionViewOptions_triggered()
{
    emit optionsOpenRequested();
}

void MainWindow::on_actionPlayPause_triggered()
{
    on_play_clicked();
}

void MainWindow::on_actionPlayStop_triggered()
{
    emit stopped();
    isPlaying = false;
    updateSize();
    ui->play->setFocus();
}

void MainWindow::on_actionPlayFrameBackward_triggered()
{
    emit stepBackward();
}

void MainWindow::on_actionPlayFrameForward_triggered()
{
    emit stepForward();
}

void MainWindow::on_actionPlayRateDecrease_triggered()
{
    emit speedDown();
}

void MainWindow::on_actionPlayRateIncrease_triggered()
{
    emit speedUp();
}

void MainWindow::on_actionPlayRateReset_triggered()
{
    emit speedReset();
}

void MainWindow::on_actionPlaySeekForwardsNormal_triggered()
{
    emit relativeSeek(true, false);
    showOsdTimer(true);
}

void MainWindow::on_actionPlaySeekBackwardsNormal_triggered()
{
    emit relativeSeek(false, false);
    showOsdTimer(true);
}

void MainWindow::on_actionPlaySeekForwardsLarge_triggered()
{
    emit relativeSeek(true, true);
    showOsdTimer(true);
}

void MainWindow::on_actionPlaySeekBackwardsLarge_triggered()
{
    emit relativeSeek(false, true);
    showOsdTimer(true);
}

void MainWindow::on_actionAudioFilterExtrastereo_triggered(bool checked)
{
    emit audioFilter("extrastereo", "", checked);
}

void MainWindow::on_actionAudioFilterAcompressor_triggered(bool checked)
{
    emit audioFilter("acompressor", "", checked);
}

void MainWindow::on_actionAudioFilterCrossfeed_triggered(bool checked)
{
    emit audioFilter("bs2b", "profile=cmoy", checked);
}

void MainWindow::on_actionVideoFilterDeinterlace_triggered(bool checked)
{
    // Deinterlacing doesn't work with hardware acceleration
    if (checked)
        mpvObject_->setCachedMpvOption("hwdec", "no");

    emit videoFilter("yadif", "mode=1", checked);
}

void MainWindow::on_actionPlayAudioTrackNext_triggered()
{
    emit nextAudioTrackSelected();
}

void MainWindow::on_actionPlayAudioTrackPrevious_triggered()
{
    emit previousAudioTrackSelected();
}

void MainWindow::on_actionPlaySubtitlesEnabled_triggered(bool checked)
{
    setSubtitlesEnabled(checked);
}

void MainWindow::on_actionPlaySubtitlesNext_triggered()
{
    emit nextSubtitleSelected();
}

void MainWindow::on_actionPlaySubtitlesPrevious_triggered()
{
    emit previousSubtitleSelected();
}

void MainWindow::on_actionPlaySubtitlesCopy_triggered()
{
    QClipboard *clippy = QApplication::clipboard();
    clippy->setText(subtitleText);
}

void MainWindow::on_actionDecreaseSubtitlesDelay_triggered()
{
    mpvObject_->setSubtitlesDelay(-subtitlesDelayStep);
}

void MainWindow::on_actionIncreaseSubtitlesDelay_triggered()
{
    mpvObject_->setSubtitlesDelay(subtitlesDelayStep);
}

void MainWindow::on_actionMoveSubtitlesUp_triggered()
{
    mpvObject_->moveSubtitlesVertically(10);
}

void MainWindow::on_actionMoveSubtitlesDown_triggered()
{
    mpvObject_->moveSubtitlesVertically(-10);
}

void MainWindow::on_actionPlayLoopStart_triggered()
{
    positionSlider_->setLoopA(mpvObject_->playTime());
    if (ui->actionPlayLoopUse->isChecked())
        mpvObject_->setLoopPoints(positionSlider_->loopA(),
                                  positionSlider_->loopB());
}

void MainWindow::on_actionPlayLoopEnd_triggered()
{
    positionSlider_->setLoopB(mpvObject_->playTime());
    if (ui->actionPlayLoopUse->isChecked())
        mpvObject_->setLoopPoints(positionSlider_->loopA(),
                                  positionSlider_->loopB());
}

void MainWindow::on_actionPlayLoopUse_toggled(bool checked)
{
    LogStream(logModule) << "on_actionPlayLoopUse_toggled: " << checked;
    if (checked) {
        if (positionSlider_->loopA() < 0)
            positionSlider_->setLoopA(0);
        if (positionSlider_->loopB() < 0 )
            positionSlider_->setLoopB(mpvObject_->playLength());

        mpvObject_->setLoopPoints(positionSlider_->loopA(),
                                  positionSlider_->loopB());
    }
    else
        mpvObject_->setLoopPoints(-1, -1);
}

void MainWindow::on_actionPlayLoopClear_triggered()
{
    ui->actionPlayLoopUse->setChecked(false);
    mpvObject_->setLoopPoints(-1, -1);
    positionSlider_->setLoopA(-1);
    positionSlider_->setLoopB(-1);
}

void MainWindow::on_actionPlayVolumeUp_triggered()
{
    int newvol = int(std::min(volumeSlider_->value() + volumeStep, volumeSlider_->maximum()));
    emit volumeChanged(newvol);
    volumeSlider_->setValue(newvol);
}

void MainWindow::on_actionPlayVolumeDown_triggered()
{
    int newvol = int(std::max(volumeSlider_->value() - volumeStep, volumeSlider_->minimum()));
    emit volumeChanged(newvol);
    volumeSlider_->setValue(newvol);
}

void MainWindow::on_actionPlayVolumeMute_toggled(bool checked)
{
    setVolumeMuteState(checked, false);
}

void MainWindow::on_actionPlayAfterOnceExit_triggered()
{
    emit afterPlaybackOnce(Helpers::ExitAfter);
}

void MainWindow::on_actionPlayAfterOnceStandby_triggered()
{
    emit afterPlaybackOnce(Helpers::StandByAfter);
}

void MainWindow::on_actionPlayAfterOnceHibernate_triggered()
{
    emit afterPlaybackOnce(Helpers::HibernateAfter);
}

void MainWindow::on_actionPlayAfterOnceShutdown_triggered()
{
    emit afterPlaybackOnce(Helpers::ShutdownAfter);
}

void MainWindow::on_actionPlayAfterOnceLogoff_triggered()
{
    emit afterPlaybackOnce(Helpers::LogOffAfter);
}

void MainWindow::on_actionPlayAfterOnceLock_triggered()
{
    emit afterPlaybackOnce(Helpers::LockAfter);
}

void MainWindow::on_actionPlayAfterOnceNothing_triggered()
{
    emit afterPlaybackOnce(Helpers::DoNothingAfter);
}

void MainWindow::on_actionPlayAfterAlwaysNext_triggered()
{
    emit afterPlaybackAlways(Helpers::PlayNextAfter);
}

void MainWindow::on_actionNavigateChaptersPrevious_triggered()
{
    emit chapterPrevious();
}

void MainWindow::on_actionNavigateChaptersNext_triggered()
{
    emit chapterNext();
}

void MainWindow::on_actionNavigateFilesPrevious_triggered()
{
    emit filePrevious(true);
}

void MainWindow::on_actionNavigateFilesNext_triggered()
{
    emit fileNext(true);
}

void MainWindow::on_actionFileMoveToRecycleBin_triggered()
{
    emit moveToRecycleBin();
}

void MainWindow::on_actionNavigateGoto_triggered()
{
    emit showGoToWindow(mpvObject_->playTime(), mpvObject_->playLength(),
                        ui->framerate->text().toDouble());
}

void MainWindow::on_actionHelpHomepage_triggered()
{
    QDesktopServices::openUrl(QUrl("https://mpc-qt.github.io/"));
}

void MainWindow::on_actionHelpAbout_triggered()
{
#if defined(MPCQT_DEVELOPMENT)
#define BUILD_VERSION_STR QString("%1 (%2)").arg(devBuild, MPCQT_VERSION_STR)
#elif defined(MPCQT_VERSION_STR)
#define BUILD_VERSION_STR versionFmt.arg(MPCQT_VERSION_STR)
#else
#define BUILD_VERSION_STR devBuild
#endif
    QString devBuild = tr("Development Build");
    QString versionFmt = tr("Version %1");
    QString dateLineFmt = tr("Built on %1 at %2");
    QString dateLine;

#if defined(MPCQT_DEVELOPMENT)
    QDate buildDate = Helpers::dateFromCFormat(__DATE__);
    QTime buildTime = Helpers::timeFromCFormat(__TIME__);
    QString dateFormat = QLocale().dateFormat(QLocale::ShortFormat);
    QString timeFormat = QLocale().timeFormat(QLocale::ShortFormat);
    dateLine = "<br>" + dateLineFmt.arg(QLocale().toString(buildDate, dateFormat),
                                        QLocale().toString(buildTime, timeFormat));
#endif
    QString displayMode = tr("(Unknown)");
    if (QGuiApplication::platformName() == "wayland")
        displayMode = "Wayland";
    else {
        if (qEnvironmentVariable("XDG_SESSION_TYPE") == "wayland")
            displayMode = "XWayland";
        else
            displayMode = "X11";
    }
    QMessageBox::about(this, tr("About Media Player Classic Qute Theater"),
      "<h2>" + tr("Media Player Classic Qute Theater") + "</h2>" +
      "<p>" +  tr("A clone of Media Player Classic written in Qt") +
      "<br>" + tr("Based on Qt %1 and %2").arg(QT_VERSION_STR, mpvObject_->mpvVersion()) +
      "<br>" + QString("ffmpeg %1").arg(mpvObject_->ffmpegVersion()) +
      (Platform::isUnix ? "<br>" + tr("Running under %1").arg(displayMode) : "") +
      "<p>" +  BUILD_VERSION_STR +
      dateLine +
      "<h3>LICENSE</h3>"
      "<p>   Copyright (C) 2015"
      "<p>"
      "This program is free software; you can redistribute it and/or modify "
      "it under the terms of the GNU General Public License as published by "
      "the Free Software Foundation; either version 2 of the License, or "
      "(at your option) any later version."
      "<p>"
      "This program is distributed in the hope that it will be useful, "
      "but WITHOUT ANY WARRANTY; without even the implied warranty of "
      "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
      "GNU General Public License for more details."
      "<p>"
      "You should have received a copy of the GNU General Public License "
      "along with this program; if not, write to the Free Software "
      "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA "
      "02110-1301 USA.");
}


void MainWindow::mpvw_customContextMenuRequested(const QPoint &pos)
{
    if (mpvw == nullptr)
        return;
    contextMenu->popup(mpvw->mapToGlobal(pos));
}

void MainWindow::statusTime_customContextMenuRequested(const QPointF &p)
{
    QMenu *m = new QMenu(this);
    QAction *a;

    bool isTimeRemaingMode = timeRemainingMode;
    a = new QAction(m);
    a->setText(tr("Remaining time"));
    a->setCheckable(true);
    a->setChecked(timeRemainingMode);
    connect(a, &QAction::triggered,
            this, [this, isTimeRemaingMode]() {
        setTimeRemainingMode(!isTimeRemaingMode);
    });
    m->addAction(a);
    
    bool isTimeShortMode = timeShortMode;
    a = new QAction(m);
    a->setText(tr("High precision"));
    a->setCheckable(true);
    a->setChecked(!timeShortMode);
    connect(a, &QAction::triggered,
            this, [this, isTimeShortMode]() {
        setTimeShortMode(!isTimeShortMode);
    });
    m->addAction(a);

    bool isTimePercentageMode = timePercentageMode;
    a = new QAction(m);
    a->setText(tr("Show percentage"));
    a->setCheckable(true);
    a->setChecked(timePercentageMode);
    connect(a, &QAction::triggered,
            this, [this, isTimePercentageMode]() {
        setTimePercentageMode(!isTimePercentageMode);
    });
    m->addAction(a);

    m->exec(statusTime->mapToGlobal(p).toPoint());
    delete m;
}

void MainWindow::position_sliderMoved(int position)
{
    emit timeSelected(position);
}

void MainWindow::position_hoverValue(double position, QString chapterInfo, double mouseX)
{
    setCursor(Qt::PointingHandCursor);

    if (!timeTooltipShown && !videoPreview)
        return;

    QString t = QString("%1%2%3").arg(Helpers::toDateFormatFixed(
                                            position,
                                            Helpers::ShortFormat),
                                      chapterInfo.isEmpty() ? "" : " - ",
                                      chapterInfo);
    QPoint where = positionSlider_->mapTo(this, QPoint(std::round(mouseX), timeTooltipAbove ? -1 : 65));
    if (videoPreview && isVideo_)
        videoPreview->show(t, position, where, this->width());
    else if (tooltip) {
        QString textTemplate = QString("%1%2%3").arg(Helpers::toDateFormatFixed(
                                                        0,
                                                        Helpers::ShortFormat),
                                                    chapterInfo.isEmpty() ? "" : " - ",
                                                    chapterInfo);
        tooltip->show(t, where, this->width(), textTemplate);
    }
    mpvw->update();
}

void MainWindow::position_hoverEnd()
{
    if (videoPreview)
        videoPreview->hide();
    if (tooltip)
        tooltip->hide();

    setCursor(Qt::ArrowCursor);
}

void MainWindow::on_play_clicked()
{
    if (!isPlaying) {
        emit playCurrentItemRequested();
        return;
    }
    if (isPaused) {
        emit unpaused();
        isPaused = false;
        ui->pause->setChecked(false);
    } else {
        emit paused();
        isPaused = true;
        ui->pause->setChecked(true);
    }
}

void MainWindow::volume_sliderMoved(double position)
{
    emit volumeChanged(int(position));
}

void MainWindow::playlistWindow_windowDocked()
{
    if (fullscreenMode_ && fullscreenHidePanels)
        playlistWindow_->hide();
}

void MainWindow::playlistWindow_playlistAddItem(const QUuid &playlistUuid)
{
    QList<QUrl> urls = doQuickOpenFileDialog();
    if (urls.isEmpty())
        return;
    emit severalFilesOpenedForPlaylist(playlistUuid, urls);
}

void MainWindow::hideTimer_timeout()
{
    if (mpvw == nullptr)
        return;
    if (fullscreenMode_ &&
            !ui->bottomArea->geometry().contains(mpvw->mapFromGlobal(QCursor::pos())))
        ui->bottomArea->hide();
}

void MainWindow::on_actionPlaylistRemoveSelected_triggered()
{
    emit removeSelectedPlaylistItem();
}

void MainWindow::on_actionPlaylistSearch_triggered()
{
    if (playlistWindow_->isHidden())
        playlistWindow_->show();
    playlistWindow_->revealSearch();
    if (ui->actionPlaylistFinishSearching->shortcut().isEmpty()) {
        for (auto action : this->findChildren<QAction*>()) {
            if (action->shortcut() == Qt::Key::Key_Escape) {
                escShortcutActionBackup = action;
                action->setShortcut(QKeySequence());
                break;
            }
        }
        ui->actionPlaylistFinishSearching->setShortcut(Qt::Key::Key_Escape);
    }
}

void MainWindow::on_actionPlaylistFinishSearching_triggered()
{
    ui->actionPlaylistFinishSearching->setShortcut(QKeySequence());
    if (escShortcutActionBackup) {
        escShortcutActionBackup->setShortcut(Qt::Key::Key_Escape);
        escShortcutActionBackup = nullptr;
    }
}

void MainWindow::on_actionFavoritesAdd_triggered()
{
    emit favoriteCurrentTrack();
}

void MainWindow::on_actionFavoritesOrganize_triggered()
{
    emit organizeFavorites();
}


