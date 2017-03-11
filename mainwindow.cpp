#include <sstream>
#include <stdexcept>
#include <cmath>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "openfiledialog.h"
#include "helpers.h"
#include <QDesktopWidget>
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

using namespace Helpers;



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    fullscreenMode_ = false;
    fullscreenMaximized = false;
    bottomAreaBehavior = Helpers::ShowWhenHovering;
    bottomAreaHeight = 0;
    bottomAreaHideTime = 0;
    timeTooltipAbove = false;
    isPlaying = false;
    sizeFactor_ = 1;
    fitFactor_ = 0.75;
    zoomMode = RegularZoom;
    zoomCenter = true;

    mouseHideTimeFullscreen = 1000;
    mouseHideTimeWindowed = 1000;

    noVideoSize_ = QSize(500,270);
    decorationState_ = AllDecorations;

    displayDrops = 0;
    decoderDrops = 0;
    audioBitrate = 0;
    videoBitrate = 0;

    ui->setupUi(this);
    setupMenu();
    setupPositionSlider();
    setupVolumeSlider();
    setupMpvWidget();
    setupMpvHost();
    setupPlaylist();
    setupStatus();
    setupSizing();
    setupBottomArea();
    setupHideTimer();

    mpvw->installEventFilter(this);
    playlistWindow_->installEventFilter(this);

    connectActionsToSlots();
    connectButtonsToActions();
    connectPlaylistWindowToActions();
    globalizeAllActions();
    setUiEnabledState(false);
    setDiscState(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

MpvWidget *MainWindow::mpvWidget()
{
    return mpvw;
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
        { "decorationState", (int)decorationState_ },
        { WRAP(ui->actionViewHideSeekbar) },
        { WRAP(ui->actionViewHideControls) },
        { WRAP(ui->actionViewHideInformation) },
        { WRAP(ui->actionViewHideStatistics) },
        { WRAP(ui->actionViewHideStatus) },
        { WRAP(ui->actionViewHideSubresync) },
        { WRAP(ui->actionViewHidePlaylist) },
        { WRAP(ui->actionViewHideCapture) },
        { WRAP(ui->actionViewHideNavigation) }
    };
#undef WRAP
}

void MainWindow::setState(const QVariantMap &map)
{
#define UNWRAP(a,b) \
    a->setChecked(map.value(a->objectName(), b).toBool())

    setUiDecorationState((DecorationState)map["decorationState"].toInt());
    UNWRAP(ui->actionViewHideSeekbar, true);
    UNWRAP(ui->actionViewHideControls, true);
    UNWRAP(ui->actionViewHideInformation, false);
    UNWRAP(ui->actionViewHideStatistics, false);
    UNWRAP(ui->actionViewHideStatus, true);
    UNWRAP(ui->actionViewHideSubresync, false);
    UNWRAP(ui->actionViewHidePlaylist, true);
    UNWRAP(ui->actionViewHideCapture, false);
    UNWRAP(ui->actionViewHideNavigation, false);
#undef UNWRAP
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
    emit applicationShouldQuit();
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (fullscreenMode_)
        checkBottomArea(event->globalPos());
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (mouseStateEvent(MouseState::fromMouseEvent(event, MouseState::MouseDown)))
        event->accept();
    else
        QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (mouseStateEvent(MouseState::fromMouseEvent(event, MouseState::PressTwice)))
        event->accept();
    mousePressEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (mouseStateEvent(MouseState::fromMouseEvent(event, MouseState::MouseUp)))
        event->accept();
    else
        QMainWindow::mouseReleaseEvent(event);
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
        action->triggered(action->isChecked());
        return true;
    }
    return false;
}
void on_actionPlaylistSearch_triggered();

QMediaSlider *MainWindow::positionSlider()
{
    return positionSlider_;
}

QVolumeSlider *MainWindow::volumeSlider()
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
    // This is where the monitor settings should be honored.
    // For now, use the current screen.
    windowHandle()->setScreen(windowHandle()->screen());

    if (!fullscreenMode_ && fullscreenMode)
        fullscreenMaximized = isMaximized();

    fullscreenMode_ = fullscreenMode;
    if (fullscreenMode)
        showFullScreen();
    else if (fullscreenMaximized)
        showMaximized();
    else
        showNormal();

    updateMouseHideTime();

    //REMOVEME: work around OpenGL blackness bug after fullscreen
    QTimer::singleShot(50, [this]() {
        positionSlider_->update();
        volumeSlider_->update();
        timePosition->update();
        timeDuration->update();
    });
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
}

void MainWindow::setupPositionSlider()
{
    positionSlider_ = new QMediaSlider();
    ui->seekbar->layout()->addWidget(positionSlider_);
    connect(positionSlider_, &QMediaSlider::sliderMoved,
            this, &MainWindow::position_sliderMoved);
    connect(positionSlider_, &QMediaSlider::hoverValue,
            this, &MainWindow::position_hoverValue);
}

void MainWindow::setupVolumeSlider()
{
    volumeSlider_ = new QVolumeSlider();
    volumeSlider_->setMinimumWidth(50);
    volumeSlider_->setMinimum(0);
    volumeSlider_->setMaximum(130);
    volumeSlider_->setValue(100);
    ui->controlbar->layout()->addWidget(volumeSlider_);
    connect(volumeSlider_, &QVolumeSlider::sliderMoved,
            this, &MainWindow::volume_sliderMoved);
}

void MainWindow::setupMpvWidget()
{
    mpvw = new MpvWidget(this, "Media Player Classic Qute Theater");
    connect(mpvw, &MpvWidget::logoSizeChanged,
            this, &MainWindow::setNoVideoSize);
    mpvw->setMouseTracking(true);
}

void MainWindow::setupMpvHost()
{
    // Crate a special QMainWindow widget so that the playlist window will
    // dock around it rather than ourselves
    mpvHost_ = new QMainWindow(this);
    mpvHost_->setCentralWidget(mpvw);
    mpvHost_->setSizePolicy(QSizePolicy(QSizePolicy::Preferred,
                                        QSizePolicy::Preferred));
    ui->mpvWidget->layout()->addWidget(mpvHost_);
}

void MainWindow::setupPlaylist()
{
    playlistWindow_ = new PlaylistWindow(this);
    mpvHost_->addDockWidget(Qt::RightDockWidgetArea, playlistWindow_);
    connect(playlistWindow_, &PlaylistWindow::windowDocked,
            this, &MainWindow::playlistWindow_windowDocked);
    QList<QWidget *> playlistWidgets = playlistWindow_->findChildren<QWidget *>();
    foreach(QWidget *w, playlistWidgets)
        w->setMouseTracking(true);
    playlistWindow_->setMouseTracking(true);
}

void MainWindow::setupStatus()
{
    timePosition = new QStatusTime();
    ui->statusbarLayout->insertWidget(2, timePosition);
    timeDuration = new QStatusTime();
    ui->statusbarLayout->insertWidget(4, timeDuration);
}

void MainWindow::setupSizing()
{
    // The point of requesting calls to updateSize through a _queued_ slot is
    // to give Qt time to respond to layout and window size changes.
    connect(this, &MainWindow::fireUpdateSize,
            this, &MainWindow::sendUpdateSize,
            Qt::QueuedConnection);

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

void MainWindow::setupHideTimer()
{
    hideTimer.setSingleShot(true);
    connect(&hideTimer, &QTimer::timeout,
            this, &MainWindow::hideTimer_timeout);
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

    if (state == NoDecorations) {
        setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
        show();
    } else if (windowFlags() & Qt::FramelessWindowHint) {
        setWindowFlags(windowFlags() & ~Qt::FramelessWindowHint);
        show();
    }
}

void MainWindow::setUiEnabledState(bool enabled)
{
    positionSlider_->setEnabled(enabled);
    if (!enabled) {
        positionSlider_->setLoopA(-1);
        positionSlider_->setLoopB(-1);
    }

    ui->play->setEnabled(enabled);
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

    ui->actionFileClose->setEnabled(enabled);
    ui->actionFileSaveCopy->setEnabled(enabled);
    ui->actionFileSaveImage->setEnabled(enabled);
    ui->actionFileSaveImageAuto->setEnabled(enabled);
    ui->actionFileSavePlainImage->setEnabled(enabled);
    ui->actionFileSavePlainImageAuto->setEnabled(enabled);
    ui->actionFileSaveThumbnails->setEnabled(enabled);
    ui->actionFileExportEncode->setEnabled(enabled);
    ui->actionFileLoadSubtitle->setEnabled(enabled);
    ui->actionFileSaveSubtitle->setEnabled(enabled);
    ui->actionFileSubtitleDatabaseDownload->setEnabled(enabled);
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
    ui->actionFavoritesAdd->setEnabled(enabled);

    ui->menuPlayLoop->setEnabled(enabled);
    ui->menuPlayAudio->setEnabled(enabled);
    ui->menuPlaySubtitles->setEnabled(enabled);
    ui->menuPlayVideo->setEnabled(enabled);
    ui->menuNavigateChapters->setEnabled(enabled);
}

void MainWindow::reparentBottomArea(bool overlay)
{
    bool inLayout = ui->centralwidget->layout()->indexOf(ui->bottomArea) >= 0;
    if (overlay && inLayout) {
        bottomAreaHeight = ui->bottomArea->height();
        ui->centralwidget->layout()->removeWidget(ui->bottomArea);
        ui->bottomArea->setParent(NULL);
        ui->bottomArea->setParent(mpvw);
    }
    if (!overlay && !inLayout) {
        ui->bottomArea->setParent(NULL);
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
    if (playlistWindow_->isVisible() && !fullscreenHidePanels)
        sz -= QSize(playlistWindow_->width(), 0);
    ui->bottomArea->setGeometry(0, sz.height() - bottomAreaHeight,
                                sz.width(), bottomAreaHeight);
}

void MainWindow::updateTime()
{
    timeDuration->setTime(mpvw->playLength());
    timePosition->setTime(mpvw->playTime());
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
    if (zoomMode == FitToWindow || fullscreenMode() || isMaximized()) {
        ui->infoSection->layout()->update();
        return;
    }

    // Grab device pixel ratio (2=HiDPI screen)
    qreal ratio = devicePixelRatio();

    // calculate available area for the window
    QDesktopWidget *desktop = qApp->desktop();
    QRect available = first_run ? desktop->availableGeometry(
                                      desktop->screenNumber(QCursor::pos()))
                                : desktop->availableGeometry(this);

    // remove the window frame size from the size available
    QSize fudgeFactor = this->frameGeometry().size() - this->geometry().size();
    fudgeFactor /= ratio;
    available.adjust(0,0, -fudgeFactor.width(), -fudgeFactor.height());

    // calculate player size
    QSize player = mpvw->videoSize() / ratio;
    double factor = sizeFactor();
    if (!isPlaying || player.isEmpty()) {
        player = noVideoSize_;
        factor = std::max(1.0, sizeFactor());
    }

    // calculate the amount taken by widgets outside the video frame
    fudgeFactor = size() - mpvw->size();

    // calculate desired client size, depending upon the zoom mode
    QSize wanted, desired;
    if (zoomMode == RegularZoom) {
        wanted = QSize(player.width()*factor + 0.5,
                       player.height()*factor + 0.5);
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
    desired = wanted + fudgeFactor;

    if (!zoomCenter) {
        resize(desired.width(), desired.height());
        return;
    }

    auto setToSize = [this](QSize &desired, const QRect &available) {
        // limit window to available desktop area
        if (desired.height() > available.height())
            desired.setHeight(available.height());
        if (desired.width() > available.width())
            desired.setWidth(available.width());

        setGeometry(QStyle::alignedRect(
                        Qt::LeftToRight, Qt::AlignCenter, desired, available));
        return;
    };

    // get it right first time, and try again with slight adjustment if needed
    setToSize(desired, available);
    QSize finalAdjustment = mpvw->size() - wanted;
    if (!finalAdjustment.isEmpty()) {
        desired += finalAdjustment;
        setToSize(desired, available);
    }
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
    ui->infoStats->adjustSize();
    ui->infoSection->adjustSize();
}

void MainWindow::updateMouseHideTime()
{
    mpvw->setMouseHideTime(fullscreenMode_
                           ? mouseHideTimeFullscreen
                           : mouseHideTimeWindowed);
}

void MainWindow::setNoVideoSize(const QSize &size)
{
    noVideoSize_ = size.expandedTo(QSize(500,270));
    fireUpdateSize();
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

void MainWindow::setTime(double time, double length)
{
    positionSlider_->setMaximum(length >= 0 ? length : 0);
    positionSlider_->setValue(time >= 0 ? time : 0);
    updateTime();
}

void MainWindow::setMediaTitle(QString title)
{
    QString window_title("Media Player Classic Qute Theater");

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
    if (fitFactor >= 0.0)
        setFitFactor(fitFactor);
    setZoomMode(mode[which + 4]);
    setSizeFactor(factor[which + 4]);
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
        mpvWidget()->setLoopPoints(-1, -1);
        positionSlider_->setLoopA(-1);
        positionSlider_->setLoopB(-1);
    }
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
    for (QPair<double,QString> chapter : chapters) {
        positionSlider_->setTick(chapter.first, chapter.second);
        QAction *action = new QAction(this);
        action->setText(chapter.second);
        connect (action, &QAction::triggered, [=]() {
           emit chapterSelected(index);
        });
        ui->menuNavigateChapters->addAction(action);
        ++index;
    }
}

void MainWindow::setAudioTracks(QList<QPair<int64_t, QString>> tracks)
{
    ui->menuPlayAudio->clear();
    for (QPair<int64_t, QString> track : tracks) {
        QAction *action = new QAction(this);
        action->setText(track.second);
        connect(action, &QAction::triggered, [=]{
            emit audioTrackSelected(track.first);
        });
        ui->menuPlayAudio->addAction(action);
    }
}

void MainWindow::setVideoTracks(QList<QPair<int64_t, QString>> tracks)
{
    ui->menuPlayVideo->clear();
    for (QPair<int64_t, QString> track : tracks) {
        QAction *action = new QAction(this);
        action->setText(track.second);
        connect(action, &QAction::triggered, [=]() {
           emit videoTrackSelected(track.first);
        });
        ui->menuPlayVideo->addAction(action);
    }
}

void MainWindow::setSubtitleTracks(QList<QPair<int64_t, QString> > tracks)
{
    ui->menuPlaySubtitles->clear();
    for (QPair<int64_t, QString> track : tracks) {
        QAction *action = new QAction(this);
        action->setText(track.second);
        connect(action, &QAction::triggered, [=]() {
            emit subtitleTrackSelected(track.first);
        });
        ui->menuPlaySubtitles->addAction(action);
    }
}

void MainWindow::setVolume(int level)
{
    volumeSlider_->setValue(level);
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

void MainWindow::setPlaylistVisibleState(bool yes) {
    if (fullscreenMode_)
        return;

    ui->actionFileOpenQuick->setText(yes ? tr("&Quick Add To Playlist")
                                         : tr("&Quick Open File"));
    ui->actionViewHidePlaylist->setChecked(yes);
}

void MainWindow::setPlaylistQuickQueueMode(bool yes)
{
    ui->actionPlaylistShowQuickQueue->setChecked(yes);
    ui->actionPlaylistQuickQueue->setEnabled(!yes);
    ui->actionPlaylistQueueVisible->setEnabled(!yes);
}

void MainWindow::on_actionFileOpenQuick_triggered()
{
    QList<QUrl> urls;
    static QUrl lastDir;
    urls = QFileDialog::getOpenFileUrls(this, tr("Quick Open"), lastDir);
    if (urls.isEmpty())
        return;
    lastDir = urls[0];
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

    QDir dir(url.toLocalFile());
    QList<QUrl> list;
    QFileInfoList f = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files);
    for(auto file : f)
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
    emit takeImage(true);
}

void MainWindow::on_actionFileSaveImageAuto_triggered()
{
    emit takeImageAutomatically(true);
}

void MainWindow::on_actionFileSavePlainImage_triggered()
{
    emit takeImage(false);
}

void MainWindow::on_actionFileSavePlainImageAuto_triggered()
{
    emit takeImageAutomatically(false);
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
    fireUpdateSize();
}

void MainWindow::on_actionViewHideSeekbar_toggled(bool checked)
{
    if (checked && ui->seekbar->isHidden())
        ui->seekbar->show();
    else if (!checked && ui->seekbar->isVisible())
        ui->seekbar->hide();
    ui->controlSection->adjustSize();
    fireUpdateSize();
}

void MainWindow::on_actionViewHideControls_toggled(bool checked)
{
    if (checked && ui->controlbar->isHidden())
        ui->controlbar->show();
    else if (!checked && ui->controlbar->isVisible())
        ui->controlbar->hide();
    ui->controlSection->adjustSize();
    fireUpdateSize();
}

void MainWindow::on_actionViewHideInformation_toggled(bool checked)
{
    Q_UNUSED(checked);
    updateInfostats();
    fireUpdateSize();
}

void MainWindow::on_actionViewHideStatistics_toggled(bool checked)
{
    Q_UNUSED(checked);
    updateInfostats();
    fireUpdateSize();
}

void MainWindow::on_actionViewHideStatus_toggled(bool checked)
{
    if (checked && ui->statusbar->isHidden())
        ui->statusbar->show();
    else if (!checked && ui->statusbar->isVisible())
        ui->statusbar->hide();
    ui->infoSection->adjustSize();
    fireUpdateSize();
}

void MainWindow::on_actionViewHideSubresync_toggled(bool checked)
{
    (void)checked;

    fireUpdateSize();
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

    fireUpdateSize();
}

void MainWindow::on_actionViewHideCapture_toggled(bool checked)
{
    (void)checked;

    fireUpdateSize();
}

void MainWindow::on_actionViewHideNavigation_toggled(bool checked)
{
    (void)checked;

    fireUpdateSize();
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


void MainWindow::on_actionPlayLoopStart_triggered()
{
    positionSlider_->setLoopA(mpvw->playTime());
    if (ui->actionPlayLoopUse->isChecked())
        mpvw->setLoopPoints(positionSlider_->loopA(),
                            positionSlider_->loopB());
}

void MainWindow::on_actionPlayLoopEnd_triggered()
{
    positionSlider_->setLoopB(mpvw->playTime());
    if (ui->actionPlayLoopUse->isChecked())
        mpvw->setLoopPoints(positionSlider_->loopA(),
                            positionSlider_->loopB());
}

void MainWindow::on_actionPlayLoopUse_triggered(bool checked)
{
    if (checked && !positionSlider_->isLoopEmpty()) {
        mpvw->setLoopPoints(positionSlider_->loopA(),
                            positionSlider_->loopB());
    } else if (checked) {
        ui->actionPlayLoopUse->setChecked(false);
    } else {
        mpvw->setLoopPoints(-1, -1);
    }
}

void MainWindow::on_actionPlayLoopClear_triggered()
{
    ui->actionPlayLoopUse->setChecked(false);
    mpvw->setLoopPoints(-1, -1);
    positionSlider_->setLoopA(-1);
    positionSlider_->setLoopB(-1);
}

void MainWindow::on_actionPlayVolumeUp_triggered()
{
    int newvol = std::min(volumeSlider_->value() + 10, 130.0);
    emit volumeChanged(newvol);
    volumeSlider_->setValue(newvol);
}

void MainWindow::on_actionPlayVolumeDown_triggered()
{
    int newvol = std::max(volumeSlider_->value() - 10, 0.0);
    emit volumeChanged(newvol);
    volumeSlider_->setValue(newvol);
}

void MainWindow::on_actionPlayVolumeMute_toggled(bool checked)
{
    if (!isPlaying)
        return;
    emit volumeMuteChanged(checked);
    ui->mute->setIcon(QIcon(checked ? ":/images/controls/speaker2.png" :
                                      ":/images/controls/speaker1.png"));
    ui->actionPlayVolumeMute->setChecked(checked);
    ui->mute->setChecked(checked);
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
    QMessageBox::about(this, "About Media Player Classic Qute Theater",
      "<h2>Media Player Classic Qute Theater</h2>"
      "<p>A clone of Media Player Classic written in Qt"
      "<p>Based on Qt " QT_VERSION_STR " and " + mpvw->mpvVersion() +
      "<p>Built on " __DATE__ " at " __TIME__
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

    QString t = QString("%1 - %2").arg(Helpers::toDateFormat(value)).arg(text);
    QPoint where = positionSlider_->mapToGlobal(QPoint(x, timeTooltipAbove ? -40 : 0));
    QToolTip::showText(where, t, positionSlider_);

    //FIXME: use a widget not the system tooltip?
}

void MainWindow::on_play_clicked()
{
    if (!isPlaying)
        return;
    if (isPaused) {
        emit unpaused();
        isPaused = false;
        ui->pause->setChecked(false);
    } else {
        emit paused();
        isPaused = true;
        ui->pause->setChecked(true);
    }
    on_actionPlayRateReset_triggered();
}

void MainWindow::volume_sliderMoved(double position)
{
    emit volumeChanged(position);
}

void MainWindow::playlistWindow_windowDocked()
{
    if (fullscreenMode_ && fullscreenHidePanels)
        playlistWindow_->hide();
}

void MainWindow::hideTimer_timeout()
{
    if (fullscreenMode_ &&
            !ui->bottomArea->geometry().contains(mpvw->mapFromGlobal(QCursor::pos())))
        ui->bottomArea->hide();
}

void MainWindow::sendUpdateSize()
{
    updateSize();
}

void MainWindow::on_actionPlaylistSearch_triggered()
{
    if (playlistWindow_->isHidden())
        playlistWindow_->show();
    playlistWindow_->revealSearch();
}
