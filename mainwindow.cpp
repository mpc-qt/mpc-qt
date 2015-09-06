#include <sstream>
#include <stdexcept>
#include <cmath>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "helpers.h"
#include <QDesktopWidget>
#include <QWindow>
#include <QMenuBar>
#include <QJsonDocument>
#include <QFileDialog>
#include <QTime>
#include <QDesktopServices>
#include <QMessageBox>
#include <QLibraryInfo>
#include <QDebug>

using namespace Helpers;



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    fullscreenMode_ = false;
    isPlaying = false;
    sizeFactor_ = 1;
    noVideoSize_ = QSize(500,270);
    decorationState_ = AllDecorations;

    ui->setupUi(this);
    setupMenu();
    setupPositionSlider();
    setupVolumeSlider();
    setupMpvWidget();
    setupMpvHost();
    setupPlaylist();
    setupSizing();
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

PlaylistWindow *MainWindow::playlistWindow()
{
    return playlistWindow_;
}

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
    fullscreenMode_ = fullscreenMode;
    if (fullscreenMode)
        showFullScreen();
    else
        showNormal();
}

void MainWindow::setNoVideoSize(QSize size)
{
    noVideoSize_ = size;
}

void MainWindow::setSizeFactor(double factor)
{
    sizeFactor_ = factor;
    if (sizeFactor_ != 0)
        updateSize();
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
}

void MainWindow::setupVolumeSlider()
{
    volumeSlider_ = new QVolumeSlider();
    volumeSlider_->setMinimumWidth(50);
    volumeSlider_->setMinimum(0);
    volumeSlider_->setMaximum(100);
    volumeSlider_->setValue(100);
    ui->controlbar->layout()->addWidget(volumeSlider_);
    connect(volumeSlider_, &QVolumeSlider::sliderMoved,
            this, &MainWindow::volume_sliderMoved);
}

void MainWindow::setupMpvWidget()
{
    mpvw = new MpvWidget(this);
}

void MainWindow::setupMpvHost()
{
    // Wrap mpvw in a special QMainWindow widget so that the playlist window
    // will dock around it rather than ourselves
    mpvHost_ = new QMainWindow(this);
    mpvHost_->setStyleSheet("background-color: black; background: center url("
                            ":/images/bitmaps/blank-screen.png) no-repeat;");
    mpvHost_->setCentralWidget(mpvw);
    mpvHost_->setSizePolicy(QSizePolicy(QSizePolicy::Preferred,
                                        QSizePolicy::Preferred));
    ui->mpvWidget->layout()->addWidget(mpvHost_);

    connectButtonsToActions();
    globalizeAllActions();
    setUiEnabledState(false);
}

void MainWindow::setupPlaylist()
{
    playlistWindow_ = new PlaylistWindow(this);
    playlistWindow_->setStyleSheet("background: window;");
    mpvHost_->addDockWidget(Qt::RightDockWidgetArea, playlistWindow_);
}

void MainWindow::setupSizing()
{
    // The point of requesting calls to updateSize through a _queued_ slot is
    // to give Qt time to respond to layout and window size changes.
    connect(this, &MainWindow::fireUpdateSize,
            this, &MainWindow::sendUpdateSize,
            Qt::QueuedConnection);

    // Guarantee that the layout has been calculated.  It seems pointless, but
    // Without it the window will temporarily display at a larger size than
    // it needs to.
    setAttribute (Qt::WA_DontShowOnScreen, true);
    show();
    QEventLoop EventLoop (this);
    while (EventLoop.processEvents()) {}
    hide();
    setAttribute (Qt::WA_DontShowOnScreen, false);

    updateSize(true);
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

    connect(ui->mute, &QPushButton::toggled,
            ui->actionPlayVolumeMute, &QAction::toggled);
}

void MainWindow::globalizeAllActions()
{
    for (QAction *a : ui->menubar->actions()) {
        addAction(a);
    }
}

void MainWindow::setUiDecorationState(DecorationState state)
{
    QString actionTexts[] = { tr("Hide &Menu"), tr("Hide &Borders"),
                              tr("Sho&w Caption and Menu") };
    Qt::WindowFlags defaults = Qt::Window | Qt::WindowTitleHint |
            Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint |
            Qt::WindowCloseButtonHint;
    Qt::WindowFlags winFlags[] = {
        defaults,
        defaults,
        Qt::Window|Qt::FramelessWindowHint };
    if (state == AllDecorations)
        menuBar()->show();
    else
        menuBar()->hide();
    ui->actionViewHideMenu->setText(actionTexts[static_cast<int>(state)]);
    setWindowFlags(winFlags[static_cast<int>(state)]);
    this->decorationState_ = state;
    show();
}

void MainWindow::setUiEnabledState(bool enabled)
{
    positionSlider()->setEnabled(enabled);

    ui->play->setEnabled(enabled);
    ui->pause->setEnabled(enabled);
    ui->stop->setEnabled(enabled);
    ui->stepBackward->setEnabled(enabled);
    ui->speedDecrease->setEnabled(enabled);
    ui->speedIncrease->setEnabled(enabled);
    ui->stepForward->setEnabled(enabled);
    ui->skipBackward->setEnabled(enabled);
    ui->skipForward->setEnabled(enabled);

    ui->mute->setEnabled(enabled);
    volumeSlider()->setEnabled(enabled);

    ui->pause->setChecked(false);
    ui->actionPlayPause->setChecked(false);

    ui->actionFileClose->setEnabled(enabled);
    ui->actionFileSaveCopy->setEnabled(enabled);
    ui->actionFileSaveImage->setEnabled(enabled);
    ui->actionFileSaveThumbnails->setEnabled(enabled);
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

    ui->menuPlayAudio->setEnabled(enabled);
    ui->menuPlaySubtitles->setEnabled(enabled);
    ui->menuPlayVideo->setEnabled(enabled);
    ui->menuNavigateChapters->setEnabled(enabled);
}

void MainWindow::updateTime()
{
    double playTime = mpvw->playTime();
    double playLength = mpvw->playLength();
    ui->time->setText(QString("%1 / %2").arg(toDateFormat(playTime),
                                             toDateFormat(playLength)));
}

void MainWindow::updateFramedrops()
{
    ui->framedrops->setText(QString("vo: %1, decoder: %2")
                            .arg(displayDrops > 0 ? displayDrops : 0)
                            .arg(decoderDrops > 0 ? decoderDrops : 0));
}

void MainWindow::updateSize(bool first_run)
{
    if (sizeFactor() <= 0 || fullscreenMode() || isMaximized()) {
        ui->infoSection->layout()->update();
        return;
    }

    QSize player = isPlaying ? mpvw->videoSize() : noVideoSize();
    double factor = isPlaying ? sizeFactor() :
                                  std::max(1.0, sizeFactor());
    QSize wanted(player.width()*factor + 0.5,
                    player.height()*factor + 0.5);
    QSize current = mpvw->size();
    QSize window = size();
    QSize desired = wanted + window - current;

    QDesktopWidget *desktop = qApp->desktop();
    if (first_run)
        setGeometry(QStyle::alignedRect(
                    Qt::LeftToRight, Qt::AlignCenter, desired,
                    desktop->availableGeometry(desktop->screenNumber(
                                                   QCursor::pos()))));
    else
        setGeometry(QStyle::alignedRect(
                    Qt::LeftToRight, Qt::AlignCenter, desired,
                        desktop->availableGeometry(this)));
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

void MainWindow::doMpvStopPlayback(bool dry_run)
{
    // TODO: emit signal instead
    if (!dry_run)
        emit stopped();
    isPlaying = false;
    updateSize();
}

void MainWindow::doMpvSetVolume(int volume)
{
    mpvw->setVolume(volume);
    mpvw->showMessage(QString("Volume :%1%").arg(volume));
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
        DataEmitter *de = new DataEmitter(action);
        action->setText(chapter.second);
        de->data = QVariant::fromValue(index);
        connect(action, &QAction::triggered, de, &DataEmitter::gotSomething);
        connect(de, &DataEmitter::heresSomething,
                this, &MainWindow::menuNavigateChapters_selected);
        ui->menuNavigateChapters->addAction(action);
        ++index;
    }
}

void MainWindow::setAudioTracks(QList<QPair<int64_t, QString>> tracks)
{
    ui->menuPlayAudio->clear();
    for (QPair<int64_t, QString> track : tracks) {
        QAction *action = new QAction(this);
        DataEmitter *de = new DataEmitter(action);
        action->setText(track.second);
        de->data = QVariant::fromValue(track.first);
        connect(action, &QAction::triggered, de, &DataEmitter::gotSomething);
        connect(de, &DataEmitter::heresSomething,
                this, &MainWindow::actionPlayAudio_selected);
        ui->menuPlayAudio->addAction(action);
    }
}

void MainWindow::setVideoTracks(QList<QPair<int64_t, QString>> tracks)
{
    ui->menuPlayVideo->clear();
    for (QPair<int64_t, QString> track : tracks) {
        QAction *action = new QAction(this);
        DataEmitter *de = new DataEmitter(action);
        action->setText(track.second);
        de->data = QVariant::fromValue(track.first);
        connect(action, &QAction::triggered, de, &DataEmitter::gotSomething);
        connect(de, &DataEmitter::heresSomething,
                this, &MainWindow::actionPlayAudio_selected);
        ui->menuPlayVideo->addAction(action);
    }
}

void MainWindow::setSubtitleTracks(QList<QPair<int64_t, QString> > tracks)
{
    ui->menuPlaySubtitles->clear();
    for (QPair<int64_t, QString> track : tracks) {
        QAction *action = new QAction(this);
        DataEmitter *de = new DataEmitter(action);
        action->setText(track.second);
        de->data = QVariant::fromValue(track.first);
        connect(action, &QAction::triggered, de, &DataEmitter::gotSomething);
        connect(de, &DataEmitter::heresSomething,
                this, &MainWindow::actionPlayAudio_selected);
        ui->menuPlaySubtitles->addAction(action);
    }
}

void MainWindow::setFps(double fps)
{
    ui->framerate->setText(isnan(fps) ? "-" : QString::number(fps, 'f', 2));
}

void MainWindow::setAvsync(double sync)
{
    ui->avsync->setText(isnan(sync) ? "-" : QString::number(sync, 'f', 3));
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

void MainWindow::on_actionFileOpenQuick_triggered()
{
    // Do nothing special for the moment, call menu_file_open instead
    on_actionFileOpen_triggered();
}

void MainWindow::on_actionFileOpen_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this, "Open file");
    QList<QUrl> list;
    list.append(QUrl::fromLocalFile(filename));
    emit filesOpened(list);
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
    if (checked)
        ui->seekbar->show();
    else
        ui->seekbar->hide();
    ui->controlSection->adjustSize();
    fireUpdateSize();
}

void MainWindow::on_actionViewHideControls_toggled(bool checked)
{
    if (checked)
        ui->controlbar->show();
    else
        ui->controlbar->hide();
    ui->controlSection->adjustSize();
    fireUpdateSize();
}

void MainWindow::on_actionViewHideInformation_toggled(bool checked)
{
    updateInfostats();
    fireUpdateSize();
}

void MainWindow::on_actionViewHideStatistics_toggled(bool checked)
{
    updateInfostats();
    fireUpdateSize();
}

void MainWindow::on_actionViewHideStatus_toggled(bool checked)
{
    if (checked)
        ui->statusbar->show();
    else
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
    // playlist window is unimplemented for now
    (void)checked;

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
    ui->actionViewHideInformation->setChecked(true);
    ui->actionViewHideStatistics->setChecked(false);
    ui->actionViewHideStatus->setChecked(true);
    ui->actionViewHideSubresync->setChecked(false);
    ui->actionViewHideCapture->setChecked(false);
    ui->actionViewHideNavigation->setChecked(false);
}

void MainWindow::on_actionViewFullscreen_toggled(bool checked)
{
    setFullscreenMode(checked);

    if (checked) {
        menuBar()->hide();
        ui->controlSection->hide();
        ui->infoSection->hide();
    } else {
        if (decorationState_ == AllDecorations)
            menuBar()->show();
        ui->controlSection->show();
        ui->infoSection->show();
    }
}

void MainWindow::on_actionViewZoom050_triggered()
{
    setSizeFactor(0.5);
}

void MainWindow::on_actionViewZoom100_triggered()
{
    setSizeFactor(1.0);
}

void MainWindow::on_actionViewZoom200_triggered()
{
    setSizeFactor(2.0);
}

void MainWindow::on_actionViewZoomAutofit_triggered()
{
    // TODO: work out the logic for this.  In the meantime, set to manual
    // sized.
    setSizeFactor(0.0);
}

void MainWindow::on_actionViewZoomAutofitLarger_triggered()
{
    // TODO: work out the logic for this.  In the meantime, set to manual
    // sized.
    setSizeFactor(0.0);
}

void MainWindow::on_actionViewZoomDisable_triggered()
{
    setSizeFactor(0.0);
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
    doMpvStopPlayback();
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

void MainWindow::actionPlayAudio_selected(QVariant data)
{
    int64_t id = data.toLongLong();
    emit audioTrackSelected(id);
}

void MainWindow::actionPlaySubtitles_selected(QVariant data)
{
    int64_t id = data.toLongLong();
    emit subtitleTrackSelected(id);
}

void MainWindow::actionPlayVideoTracks_selected(QVariant data)
{
    int64_t id = data.toLongLong();
    emit videoTrackSelected(id);
}

void MainWindow::on_actionPlayVolumeUp_triggered()
{
    int newvol = std::min(volumeSlider_->value() + 10, 100.0);
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

void MainWindow::menuNavigateChapters_selected(QVariant data)
{
    emit chapterSelected(data.toInt());
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
    //mpvw->setTime(position);
}

void MainWindow::on_play_clicked()
{
    if (!isPlaying)
        return;
    if (isPaused) {
        emit unpaused();
        isPaused = false;
        ui->pause->setChecked(false);
    }
    on_actionPlayRateReset_triggered();
}

void MainWindow::volume_sliderMoved(double position)
{
    emit volumeChanged(position);
}

void MainWindow::sendUpdateSize()
{
    updateSize();
}
