#include <sstream>
#include <stdexcept>
#include <cmath>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "openfiledialog.h"
#include "helpers.h"
#include "platform/unify.h"
#include "platform/devicemanager.h"
#include <QDesktopWidget>
#include <QStyle>
#include <QWindow>
#include <QMenuBar>
#include <QSizeGrip>
#include <QMimeData>
#include <QJsonDocument>
#include <QFileDialog>
#include <QInputDialog>
#include <QTime>
#include <QDesktopServices>
#include <QMessageBox>
#include <QLibraryInfo>
#include <QToolTip>
#include <QStyle>

using namespace Helpers;



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupMenu();
    setupContextMenu();
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

    mpvw->installEventFilter(this);
    playlistWindow_->installEventFilter(this);

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
        qApp->processEvents(QEventLoop::AllEvents |
                            QEventLoop::WaitForMoreEvents,
                            50);
        hide();
        setAttribute(Qt::WA_DontShowOnScreen, false);
    }
}

MainWindow::~MainWindow()
{
    mpvObject_->setWidgetType(Helpers::NullWidget);

    delete ui;
    ui = nullptr;
}

MpvObject *MainWindow::mpvObject()
{
    return mpvObject_;
}

QMainWindow *MainWindow::mpvHost()
{
    return mpvHost_;
}

PlaylistWindow *MainWindow::playlistWindow()
{
    return playlistWindow_;
}

static void actionsToList(QList<QAction*> &actionList, const QList<QAction*> &actions) {
    for (QAction *a : actions) {
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
    actionToMap(ui->actionPlayPause, {MouseState::Left,0,MouseState::MouseDown});
    actionToMap(ui->actionNavigateChaptersNext, {MouseState::Forward,0,MouseState::MouseDown});
    actionToMap(ui->actionNavigateChaptersPrevious, {MouseState::Back,0,MouseState::MouseDown});
    actionToMap(ui->actionPlayVolumeDown, {MouseState::Wheel,0,MouseState::MouseDown});
    actionToMap(ui->actionPlayVolumeUp, {MouseState::Wheel,0,MouseState::MouseUp});
    return commandMap;
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
        { WRAP(ui->actionViewOntopDefault) },
        { WRAP(ui->actionViewOntopAlways) },
        { WRAP(ui->actionViewOntopPlaying) },
        { WRAP(ui->actionViewOntopVideo) },
        { WRAP(ui->actionViewFullscreen) },
        { WRAP(ui->actionPlaySubtitlesEnabled) }
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
    UNWRAP(ui->actionViewOntopDefault, true);
    UNWRAP(ui->actionViewOntopAlways, false);
    UNWRAP(ui->actionViewOntopPlaying, false);
    UNWRAP(ui->actionViewOntopVideo, false);
    UNWRAP(ui->actionViewFullscreen, false);
    UNWRAP(ui->actionPlaySubtitlesEnabled, true);
    on_actionPlaySubtitlesEnabled_triggered(ui->actionPlaySubtitlesEnabled->isChecked());
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
    qreal ratio = devicePixelRatio();

    // calculate available area for the window
    QDesktopWidget *desktop = qApp->desktop();
    QRect available = first_run ? desktop->availableGeometry(
                                      desktop->screenNumber(QCursor::pos()))
                                : desktop->availableGeometry(this);
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
    QSize fudgeFactor = size() - mpvw->size();

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

QPoint MainWindow::desirablePosition(QSize &size, bool first_run)
{
    QDesktopWidget *desktop = qApp->desktop();

    QRect available = first_run ? desktop->availableGeometry(
                                      desktop->screenNumber(QCursor::pos()))
                                : desktop->availableGeometry(this);
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

void MainWindow::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    updateBottomAreaGeometry();
    checkBottomArea(QCursor::pos());
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    if ((object == mpvw || object == playlistWindow_) && event->type() == QEvent::MouseMove) {
        this->mouseMoveEvent(static_cast<QMouseEvent*>(event));
    }
    return QMainWindow::eventFilter(object, event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->accept();
    emit instanceShouldQuit();
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (fullscreenMode_)
        checkBottomArea(event->globalPos());
}

static bool insideWidget(QPoint p, QWidget *mpvw) {
    QRect rc(mpvw->mapToGlobal(QPoint()), mpvw->size());
    return rc.contains(p);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    bool ok = insideWidget(event->globalPos(), mpvw);
    if (ok && mouseStateEvent(MouseState::fromMouseEvent(event, MouseState::MouseDown)))
        event->accept();
    else
        QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    bool ok = insideWidget(event->globalPos(), mpvw);
    if (ok && mouseStateEvent(MouseState::fromMouseEvent(event, MouseState::PressTwice)))
        event->accept();
    mousePressEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    bool ok = insideWidget(event->globalPos(), mpvw);
    if (ok && mouseStateEvent(MouseState::fromMouseEvent(event, MouseState::MouseUp)))
        event->accept();
    else
        QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    bool ok = insideWidget(event->globalPos(), mpvw);
    if (ok && mouseStateEvent(MouseState::fromWheelEvent(event)))
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
void on_actionPlaylistSearch_triggered();

MediaSlider *MainWindow::positionSlider()
{
    return positionSlider_;
}

VolumeSlider *MainWindow::volumeSlider()
{
    return volumeSlider_;
}

MainWindow::DecorationState MainWindow::decorationState()
{
    return decorationState_;
}

bool MainWindow::fullscreenMode()
{
    return fullscreenMode_;
}

QSize MainWindow::noVideoSize()
{
    return noVideoSize_;
}

double MainWindow::sizeFactor()
{
    return sizeFactor_;
}

void MainWindow::setFullscreenMode(bool fullscreenMode)
{
    if (fullscreenMode_ == fullscreenMode)
        return;

    // This is where the monitor settings should be honored.
    // For now, use the current screen.
    windowHandle()->setScreen(windowHandle()->screen());

    // save maximized state if changing from windowed to fullscreen
    if (!fullscreenMode_ && fullscreenMode)
        fullscreenMaximized = isMaximized();

    fullscreenMode_ = fullscreenMode;
    if (fullscreenMode)
        showFullScreen();
    else if (fullscreenMaximized)
        showMaximized();
    else
        showNormal();

    ui->actionViewFullscreenEscape->setEnabled(fullscreenMode);
    updateMouseHideTime();

    //REMOVEME: work around OpenGL blackness bug after fullscreen
    QTimer::singleShot(50, [this]() {
        positionSlider_->update();
        volumeSlider_->update();
        timePosition->update();
        timeDuration->update();
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
    // Work around separators with text in the designer not showing as
    // sections
    ui->menuPlayAfter->insertSection(ui->actionPlayAfterOnceExit,
                                       tr("Once"));
    ui->menuPlayAfter->insertSection(ui->actionPlayAfterAlwaysExit,
                                      tr("Every time"));

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
    contextView->addAction(ui->actionViewHideCapture);
    contextView->addAction(ui->actionViewHideNavigation);
    contextView->addMenu(ui->menuViewPresets);

    contextMenu->addMenu(contextView);
    contextMenu->addAction(ui->actionFileProperties);
    contextMenu->addAction(ui->actionViewOptions);
    contextMenu->addSeparator();
    contextMenu->addAction(ui->actionFileExit);
}

void MainWindow::setupActionGroups()
{
    QActionGroup *ag;

    ag = new QActionGroup(this);
    ag->addAction(ui->actionViewOSDNone);
    ag->addAction(ui->actionViewOSDMessages);
    ag->addAction(ui->actionViewOSDStatistics);
    ag->addAction(ui->actionViewOSDFrameTimings);

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
    ag->addAction(ui->actionPlayAfterOnceRepeat);
    ag->addAction(ui->actionPlayAfterOnceShutdown);
    ag->addAction(ui->actionPlayAfterOnceStandby);

    ag = new QActionGroup(this);
    ag->addAction(ui->actionPlayAfterAlwaysExit);
    ag->addAction(ui->actionPlayAfterAlwaysNext);
    ag->addAction(ui->actionPlayAfterAlwaysNothing);
    ag->addAction(ui->actionPlayAfterAlwaysRepeat);
}

void MainWindow::setupPositionSlider()
{
    positionSlider_ = new MediaSlider();
    ui->seekbar->layout()->addWidget(positionSlider_);
    connect(positionSlider_, &MediaSlider::sliderMoved,
            this, &MainWindow::position_sliderMoved);
    connect(positionSlider_, &MediaSlider::hoverValue,
            this, &MainWindow::position_hoverValue);
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
    // Crate a special QMainWindow widget so that the playlist window will
    // dock around it rather than ourselves
    mpvHost_ = new QMainWindow(this);
    mpvHost_->setSizePolicy(QSizePolicy(QSizePolicy::Ignored,
                                        QSizePolicy::Ignored));
    ui->mpvWidget->layout()->addWidget(mpvHost_);
}

void MainWindow::setupMpvObject()
{
    mpvObject_ = new MpvObject(this, "Media Player Classic Qute Theater");
    mpvObject_->setHostWindow(mpvHost_);
    mpvObject_->setWidgetType(Helpers::GlCbWidget);
    mpvw = mpvObject_->mpvWidget();
    if (!mpvw)
        throw(std::runtime_error("Video widget not created"));
    connect(mpvObject_, &MpvObject::logoSizeChanged,
            this, &MainWindow::setNoVideoSize);
    // CHECKME: signal could be in mpvObject and reflected internally
    connect(mpvw, &QWidget::customContextMenuRequested,
            this, &MainWindow::mpvw_customContextMenuRequested);
    // CHECKME: mouse tracking could be set by mpvObject's setWidgetType func
    mpvObject_->mpvWidget()->setMouseTracking(true);
}

void MainWindow::setupPlaylist()
{
    playlistWindow_ = new PlaylistWindow(this);
    mpvHost_->addDockWidget(Qt::RightDockWidgetArea, playlistWindow_);
    connect(playlistWindow_, &PlaylistWindow::windowDocked,
            this, &MainWindow::playlistWindow_windowDocked);
    connect(playlistWindow_, &PlaylistWindow::playlistAddItem,
            this, &MainWindow::playlistWindow_playlistAddItem);
    connect(playlistWindow_, &PlaylistWindow::currentPlaylistHasItems,
            ui->play, &QPushButton::setEnabled);
    QList<QWidget *> playlistWidgets = playlistWindow_->findChildren<QWidget *>();
    foreach(QWidget *w, playlistWidgets)
        w->setMouseTracking(true);
    playlistWindow_->setMouseTracking(true);
}

void MainWindow::setupStatus()
{
    timePosition = new StatusTime();
    ui->statusbarLayout->insertWidget(2, timePosition);
    timeDuration = new StatusTime();
    ui->statusbarLayout->insertWidget(4, timeDuration);
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
    QList<IconThemer::IconData> data {
        { ui->play, "media-playback-start" },
        { ui->pause, "media-playback-pause" },
        { ui->stop, "media-playback-stop" },
        { ui->skipBackward, "go-previous" },
        { ui->skipForward, "go-next" },
        { ui->speedDecrease, "media-seek-backward" },
        { ui->speedIncrease, "media-seek-forward" },
        { ui->stepBackward, "media-skip-backward" },
        { ui->stepForward, "media-skip-forward" },
        { ui->loopA, "zone-in" },
        { ui->loopB, "zone-out" },
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

void MainWindow::connectActionsToSignals()
{
    connect(ui->actionFileProperties, &QAction::triggered,
            this, &MainWindow::showFileProperties);
}

void MainWindow::connectActionsToSlots()
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

    connect(ui->subs, &QPushButton::toggled,
            [this](bool checked) { on_actionPlaySubtitlesEnabled_triggered(!checked); });
    connect(ui->mute, &QPushButton::toggled,
            ui->actionPlayVolumeMute, &QAction::toggled);
}

void MainWindow::connectPlaylistWindowToActions()
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
    connect(ui->actionPlaylistPlayCurrent, &QAction::triggered,
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
    addAction(ui->actionPlaySeekForwards);
    addAction(ui->actionPlaySeekForwardsFine);
    addAction(ui->actionPlaySeekBackwards);
    addAction(ui->actionPlaySeekBackwardsFine);
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

void MainWindow::setUiEnabledState(bool enabled)
{
    positionSlider_->setEnabled(enabled);
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

    ui->mute->setEnabled(enabled);
    volumeSlider()->setEnabled(enabled);

    ui->pause->setChecked(false);
    ui->actionPlayPause->setChecked(false);

    ui->actionFileOpenDevice->setEnabled(false);
    ui->actionFileClose->setEnabled(enabled);
    ui->actionFileSaveCopy->setEnabled(enabled);
    ui->actionFileSaveImage->setEnabled(enabled);
    ui->actionFileSaveImageAuto->setEnabled(enabled);
    ui->actionFileSavePlainImage->setEnabled(enabled);
    ui->actionFileSavePlainImageAuto->setEnabled(enabled);
    ui->actionFileSaveWindowImage->setEnabled(enabled);
    ui->actionFileSaveWindowImageAuto->setEnabled(enabled);
    ui->actionFileSaveThumbnails->setEnabled(enabled && false);
    ui->actionFileExportEncode->setEnabled(enabled && false);
    ui->actionFileLoadSubtitle->setEnabled(enabled);
    ui->actionFileSaveSubtitle->setEnabled(enabled && false);
    ui->actionFileSubtitleDatabaseDownload->setEnabled(enabled && false);
    ui->actionPlayPause->setEnabled(enabled);
    ui->actionPlayStop->setEnabled(enabled);
    ui->actionPlayFrameBackward->setEnabled(enabled);
    ui->actionPlayFrameForward->setEnabled(enabled);
    ui->actionPlayRateDecrease->setEnabled(enabled);
    ui->actionPlayRateIncrease->setEnabled(enabled);
    ui->actionPlayRateReset->setEnabled(enabled);
    ui->actionPlayVolumeUp->setEnabled(enabled);
    ui->actionPlayVolumeDown->setEnabled(enabled);
    ui->actionPlayVolumeMute->setEnabled(enabled);
    ui->actionNavigateChaptersPrevious->setEnabled(enabled);
    ui->actionNavigateChaptersNext->setEnabled(enabled);
    ui->actionNavigateGoto->setEnabled(false);
    ui->actionFavoritesAdd->setEnabled(enabled);

    ui->menuFileSubtitleDatabase->setEnabled(false);
    ui->menuPlayLoop->setEnabled(enabled);
    ui->menuPlayAudio->setEnabled(enabled);
    ui->menuPlaySubtitles->setEnabled(enabled);
    ui->menuPlayVideo->setEnabled(enabled);
    ui->menuNavigateChapters->setEnabled(enabled && false);
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
        ui->bottomArea->setParent(mpvObject_->mpvWidget());
    }
    if (!overlay && !inLayout) {
        ui->bottomArea->setParent(nullptr);
        ui->centralwidget->layout()->addWidget(ui->bottomArea);
        ui->bottomArea->show();
    }
}

void MainWindow::checkBottomArea(QPoint mousePosition)
{
    if (!fullscreenMode_)
        return;

    QPoint relMousePosition;
    bool startTimer = false;
    switch (bottomAreaBehavior) {
    case Helpers::NeverShown:
        if (ui->bottomArea->isVisible())
            ui->bottomArea->hide();
        break;
    case Helpers::ShowWhenHovering:
        relMousePosition = mpvw->mapFromGlobal(mousePosition);
        if (ui->bottomArea->geometry().contains(relMousePosition)) {
            if (ui->bottomArea->isHidden())
                ui->bottomArea->show();
        } else if (ui->bottomArea->isVisible() && !hideTimer.isActive()) {
            startTimer = true;
        }
        break;
    case Helpers::ShowWhenMoving:
        relMousePosition = mapFromGlobal(mousePosition);
        if (mpvw->geometry().contains(relMousePosition)) {
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

void MainWindow::updateTime()
{
    timeDuration->setTime(mpvObject_->playLength());
    timePosition->setTime(mpvObject_->playTime());
}

void MainWindow::updateFramedrops()
{
    ui->framedrops->setText(QString("vo: %1, decoder: %2")
                            .arg(displayDrops > 0 ? displayDrops : 0)
                            .arg(decoderDrops > 0 ? decoderDrops : 0));
}

void MainWindow::updateBitrate()
{
    ui->bitrate->setText(QString("v: %1 kb/s, a: %2 kb/s")
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
        showOnTop = isPlaying;
        break;
    case OnTopForVideos:
        showOnTop = isPlaying && hasVideo;
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
    auto func = [&](DeviceInfo *device) -> void {
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
        ui->menuFileOpenDisc->addAction(a);
        addedSomething = true;
    };
    ui->menuFileOpenDisc->clear();
    Platform::deviceManager()->iterateDevices(func);
    ui->menuFileOpenDisc->setEnabled(addedSomething);
}

QList<QUrl> MainWindow::doQuickOpenFileDialog()
{
    QList<QUrl> urls;
    static QUrl lastDir;
    static QString filter;
    if (filter.isEmpty())
        filter = Helpers::fileOpenFilter();

    urls = QFileDialog::getOpenFileUrls(this, tr("Quick Open"), lastDir, filter);
    if (!urls.isEmpty())
        lastDir = urls[0];
    return urls;
}

void MainWindow::setFreestanding(bool freestanding)
{
    freestanding_ = freestanding;
    setMediaTitle(QString());
}

void MainWindow::setNoVideoSize(const QSize &size)
{
    noVideoSize_ = size.expandedTo(QSize(500,270));
    updateSize();
}

void MainWindow::setWindowedMouseMap(const MouseStateMap &map)
{
    mouseMapWindowed = map;
}

void MainWindow::setFullscreenMouseMap(const MouseStateMap &map)
{
    mouseMapFullscreen = map;
}

void MainWindow::setRecentDocuments(QList<TrackInfo> tracks)
{
    bool isEmpty = tracks.count() == 0;
    ui->menuFileRecent->clear();
    ui->menuFileRecent->setDisabled(isEmpty);
    if (isEmpty)
        return;

    for (int i = 0; i < tracks.count(); i++) {
        TrackInfo track = tracks[i];
        QString displayString = track.url.fileName();
        QAction *a = new QAction(QString("%1 - %2").arg(i).arg(displayString),
                                 this);
        connect(a, &QAction::triggered, [=]() {
            emit recentOpened(track);
        });
        ui->menuFileRecent->addAction(a);
    }
    ui->menuFileRecent->addSeparator();
    ui->menuFileRecent->addAction(ui->actionFileRecentClear);
}

void MainWindow::setFavoriteTracks(QList<TrackInfo> files, QList<TrackInfo> streams)
{
    auto addAction = [&](QAction *a) {
        menuFavoritesTail.append(a);
        ui->menuFavorites->addAction(a);
    };
    auto addSeperator = [&]() {
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
            a->setText(t.text);
            connect(a, &QAction::triggered,
                    a, [t, this]() {
                this->recentOpened(t);
            });
            addAction(a);
        }
    };

    for (auto a : menuFavoritesTail)
        delete a;
    menuFavoritesTail.clear();

    addSeperator();
    if (files.empty())
        addNotice(tr("No files favorited"));
    else
        addTracks(files);
    addSeperator();
    if (streams.empty())
        addNotice(tr("No streams favorited"));
    else
        addTracks(streams);
}

void MainWindow::setIconTheme(IconThemer::FolderMode mode, QString fallback, QString custom)
{
    themer.setIconFolders(mode, fallback, custom);
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
    positionSlider_->setMaximum(length >= 0 ? length : 0);
    positionSlider_->setValue(time >= 0 ? time : 0);
    updateTime();
}

void MainWindow::setMediaTitle(QString title)
{
    QString window_title("Media Player Classic Qute Theater");

    if (freestanding_)
        window_title.append(tr(" [Freestanding]"));
    if (!title.isEmpty())
        window_title.append(" - ").append(title);
    setWindowTitle(window_title);
}

void MainWindow::setChapterTitle(QString title)
{
    ui->chapter->setText(!title.isEmpty() ? title : "-");
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

void MainWindow::setTimeTooltip(bool shown, bool above)
{
    timeTooltipShown = shown;
    timeTooltipAbove = above;
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
    ui->status->setText(state==PlaybackManager::StoppedState ? "Stopped" :
                        state==PlaybackManager::PausedState ? "Paused" :
                        state==PlaybackManager::PlayingState ? "Playing" :
                        state==PlaybackManager::BufferingState ? "Buffering" :
                                                                 "Unknown");
    isPlaying = state != PlaybackManager::StoppedState;
    isPaused = state == PlaybackManager::PausedState;
    setUiEnabledState(state != PlaybackManager::StoppedState);
    if (isPaused) {
        ui->actionPlayPause->setChecked(true);
        ui->pause->setChecked(true);
    }
    if (state == PlaybackManager::WaitingState) {
        mpvObject_->setLoopPoints(-1, -1);
        positionSlider_->setLoopA(-1);
        positionSlider_->setLoopB(-1);
    }
    updateOnTop();
}

void MainWindow::setPlaybackType(PlaybackManager::PlaybackType type)
{
    setUiEnabledState(type != PlaybackManager::None);
}

void MainWindow::setChapters(QList<QPair<double, QString>> chapters)
{
    positionSlider_->clearTicks();
    ui->menuNavigateChapters->clear();
    int64_t index = 0;
    for (const QPair<double,QString> &chapter : chapters) {
        positionSlider_->setTick(chapter.first, chapter.second);
        QAction *action = new QAction(this);
        action->setText(chapter.second);
        connect (action, &QAction::triggered, [this,index]() {
           emit chapterSelected(index);
        });
        ui->menuNavigateChapters->addAction(action);
        ++index;
    }
}

void MainWindow::setAudioTracks(QList<QPair<int64_t, QString>> tracks)
{
    ui->menuPlayAudio->clear();
    for (const QPair<int64_t, QString> &track : tracks) {
        QAction *action = new QAction(this);
        action->setText(track.second);
        int64_t index = track.first;
        connect(action, &QAction::triggered, [this,index] {
            emit audioTrackSelected(index);
        });
        ui->menuPlayAudio->addAction(action);
    }
    hasAudio = !tracks.isEmpty();
}

void MainWindow::setVideoTracks(QList<QPair<int64_t, QString>> tracks)
{
    ui->menuPlayVideo->clear();
    for (const QPair<int64_t, QString> &track : tracks) {
        QAction *action = new QAction(this);
        action->setText(track.second);
        int64_t index = track.first;
        connect(action, &QAction::triggered, [this,index]() {
           emit videoTrackSelected(index);
        });
        ui->menuPlayVideo->addAction(action);
    }
    hasVideo = !tracks.isEmpty();
    updateOnTop();
}

void MainWindow::setSubtitleTracks(QList<QPair<int64_t, QString> > tracks)
{
    ui->menuPlaySubtitles->clear();
    hasSubs = !tracks.isEmpty();
    ui->actionPlaySubtitlesEnabled->setEnabled(hasSubs);
    ui->subs->setEnabled(hasSubs);
    ui->actionPlaySubtitlesNext->setEnabled(hasSubs);
    ui->actionPlaySubtitlesPrevious->setEnabled(hasSubs);
    if (!hasSubs)
        return;
    ui->menuPlaySubtitles->addAction(ui->actionPlaySubtitlesEnabled);
    ui->menuPlaySubtitles->addAction(ui->actionPlaySubtitlesNext);
    ui->menuPlaySubtitles->addAction(ui->actionPlaySubtitlesPrevious);
    ui->menuPlaySubtitles->addSeparator();
    for (const QPair<int64_t, QString> &track : tracks) {
        QAction *action = new QAction(this);
        action->setText(track.second);
        int64_t index = track.first;
        connect(action, &QAction::triggered, [this,index]() {
            emit subtitleTrackSelected(index);
        });
        ui->menuPlaySubtitles->addAction(action);
    }
}

void MainWindow::setVolume(int level)
{
    volumeSlider_->setValue(level);
    emit volumeChanged(level);
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

void MainWindow::resetPlayAfterOnce()
{
    ui->actionPlayAfterOnceNothing->setChecked(true);
}

void MainWindow::setPlayAfterAlways(AfterPlayback action)
{
    QMap<Helpers::AfterPlayback, QAction*> map {
        { Helpers::DoNothingAfter, ui->actionPlayAfterAlwaysNothing },
        { Helpers::RepeatAfter, ui->actionPlayAfterAlwaysRepeat },
        { Helpers::PlayNextAfter, ui->actionPlayAfterAlwaysNext },
        { Helpers::ExitAfter, ui->actionPlayAfterAlwaysExit }
    };
    if (map.contains(action))
        map[action]->setChecked(true);
}

void MainWindow::setFps(double fps)
{
    ui->framerate->setText(std::isnan(fps) ? "-" : QString::number(fps, 'f', 2));
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

void MainWindow::logWindowClosed()
{
    ui->actionViewHideLog->setChecked(false);
}

void MainWindow::setPlaylistVisibleState(bool yes) {
    if (fullscreenMode_)
        return;
    if (!ui)
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
    emit severalFilesOpened(urls);
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
    QUrl dir;
    static QUrl lastDir;
    dir = QFileDialog::getExistingDirectoryUrl(this, tr("Open Directory"),
                                               lastDir);
    if (dir.isEmpty())
        return;
    lastDir = dir;
    emit dvdbdOpened(dir);
}

void MainWindow::on_actionFileOpenDirectory_triggered()
{
    QUrl url;
    static QUrl lastDir;
    url = QFileDialog::getExistingDirectoryUrl(this, tr("Open Directory"),
                                               lastDir);
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
    connect(qid, &QInputDialog::accepted, [=] () {
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
    url = QFileDialog::getOpenFileUrl(this, tr("Open Subtitle"), lastUrl);
    if (url.isEmpty())
        return;
    lastUrl = url;
    emit subtitlesLoaded(url);
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
    Q_UNUSED(checked);
    updateInfostats();
    updateSize();
}

void MainWindow::on_actionViewHideStatistics_toggled(bool checked)
{
    Q_UNUSED(checked);
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
    if (fullscreenMode_ && fullscreenHidePanels)
        return;

    if (isMinimized())
        return;

    if (checked && playlistWindow_->isHidden())
        playlistWindow_->show();
    else if (!checked && playlistWindow_->isVisible())
        playlistWindow_->hide();

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
}

void MainWindow::on_actionViewOSDMessages_triggered()
{
    mpvObject_->showStatsPage(0);
}

void MainWindow::on_actionViewOSDStatistics_triggered()
{
    mpvObject_->showStatsPage(1);
}

void MainWindow::on_actionViewOSDFrameTimings_triggered()
{
    mpvObject_->showStatsPage(2);
}

void MainWindow::on_actionViewOSDCycle_triggered()
{
    QActionGroup *osdActionGroup = ui->actionViewOSDMessages->actionGroup();
    QAction *nextOsdAction;
    int newpage;

    newpage = mpvObject_->cycleStatsPage();
    nextOsdAction = osdActionGroup->actions().value(newpage+1, nullptr);
    if (nextOsdAction)
        nextOsdAction->setChecked(true);
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

void MainWindow::on_actionViewOntopDefault_toggled(bool checked)
{
    if (!checked)
        return;
    onTopMode = OnTopDefault;
    updateOnTop();
}

void MainWindow::on_actionViewOntopAlways_toggled(bool checked)
{
    if (!checked)
        return;
    onTopMode = AlwaysOnTop;
    updateOnTop();
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

void MainWindow::on_actionPlayPause_triggered(bool checked)
{
    if (checked)
        emit paused();
    else
        emit unpaused();
}

void MainWindow::on_actionPlayStop_triggered()
{
    emit stopped();
    isPlaying = false;
    updateSize();
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

void MainWindow::on_actionPlaySeekForwards_triggered()
{
    emit relativeSeek(true, false);
}

void MainWindow::on_actionPlaySeekBackwards_triggered()
{
    emit relativeSeek(false, false);
}

void MainWindow::on_actionPlaySeekForwardsFine_triggered()
{
    emit relativeSeek(true, true);
}

void MainWindow::on_actionPlaySeekBackwardsFine_triggered()
{
    emit relativeSeek(false, true);
}

void MainWindow::on_actionPlaySubtitlesEnabled_triggered(bool checked)
{
    emit subtitlesEnabled(checked);
    ui->actionPlaySubtitlesEnabled->setChecked(checked);
    ui->subs->setChecked(!checked);
}

void MainWindow::on_actionPlaySubtitlesNext_triggered()
{
    emit nextSubtitleSelected();
}

void MainWindow::on_actionPlaySubtitlesPrevious_triggered()
{
    emit previousSubtitleSelected();
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

void MainWindow::on_actionPlayLoopUse_triggered(bool checked)
{
    if (checked && !positionSlider_->isLoopEmpty()) {
        mpvObject_->setLoopPoints(positionSlider_->loopA(),
                                  positionSlider_->loopB());
    } else if (checked) {
        ui->actionPlayLoopUse->setChecked(false);
    } else {
        mpvObject_->setLoopPoints(-1, -1);
    }
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
    int newvol = int(std::min(volumeSlider_->value() + volumeStep, 130.0));
    emit volumeChanged(newvol);
    volumeSlider_->setValue(newvol);
}

void MainWindow::on_actionPlayVolumeDown_triggered()
{
    int newvol = int(std::max(volumeSlider_->value() - volumeStep, 0.0));
    emit volumeChanged(newvol);
    volumeSlider_->setValue(newvol);
}

void MainWindow::on_actionPlayVolumeMute_toggled(bool checked)
{
    if (!isPlaying)
        return;
    emit volumeMuteChanged(checked);
    ui->actionPlayVolumeMute->setChecked(checked);
    ui->mute->setChecked(checked);
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

void MainWindow::on_actionPlayAfterOnceRepeat_triggered()
{
    emit afterPlaybackOnce(Helpers::RepeatAfter);
}

void MainWindow::on_actionPlayAfterAlwaysRepeat_triggered()
{
    emit afterPlaybackAlways(Helpers::RepeatAfter);
}

void MainWindow::on_actionPlayAfterAlwaysExit_triggered()
{
    emit afterPlaybackAlways(Helpers::ExitAfter);
}

void MainWindow::on_actionPlayAfterAlwaysNothing_triggered()
{
    emit afterPlaybackAlways(Helpers::DoNothingAfter);
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

void MainWindow::on_actionHelpHomepage_triggered()
{
    QDesktopServices::openUrl(QUrl("https://github.com/cmdrkotori/mpc-qt"));
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
    QDate buildDate = Helpers::dateFromCFormat(__DATE__);
    QTime buildTime = Helpers::timeFromCFormat(__TIME__);
    QMessageBox::about(this, tr("About Media Player Classic Qute Theater"),
      "<h2>" + tr("Media Player Classic Qute Theater") + "</h2>" +
      "<p>" +  tr("A clone of Media Player Classic written in Qt") +
      "<br>" + tr("Based on Qt %1 and %2").arg(QT_VERSION_STR, mpvObject_->mpvVersion()) +
      "<p>" +  BUILD_VERSION_STR +
      "<br>" + tr("Built on %1 at %2").arg(buildDate.toString(Qt::DefaultLocaleShortDate),
                                           buildTime.toString(Qt::DefaultLocaleShortDate)) +
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
    contextMenu->popup(mpvw->mapToGlobal(pos));
}

void MainWindow::position_sliderMoved(int position)
{
    emit timeSelected(position);
}

void MainWindow::position_hoverValue(double value, QString text, double x)
{
    if (!timeTooltipShown)
        return;
    if (text.isEmpty())
        text = "<unknown>";

    QString t = QString("%1 - %2").arg(Helpers::toDateFormat(value), text);
    QPoint where = positionSlider_->mapToGlobal(QPoint(int(x), timeTooltipAbove ? -40 : 0));
    QToolTip::showText(where, t, positionSlider_);

    //FIXME: use a widget not the system tooltip?
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
    if (fullscreenMode_ &&
            !ui->bottomArea->geometry().contains(mpvw->mapFromGlobal(QCursor::pos())))
        ui->bottomArea->hide();
}

void MainWindow::on_actionPlaylistSearch_triggered()
{
    if (playlistWindow_->isHidden())
        playlistWindow_->show();
    playlistWindow_->revealSearch();
}


void MainWindow::on_actionFavoritesAdd_triggered()
{
    emit favoriteCurrentTrack();
}

void MainWindow::on_actionFavoritesOrganize_triggered()
{
    emit organizeFavorites();
}
