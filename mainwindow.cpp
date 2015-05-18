#include <sstream>
#include <stdexcept>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDesktopWidget>
#include <QWindow>
#include <QMenuBar>
#include <QJsonDocument>
#include <QFileDialog>
#include <QTime>
#include <QMessageBox>
#include <QLibraryInfo>
#include <QDebug>

static QString toDateFormat(double time);

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    fullscreenMode_ = false;
    isPlaying_ = false;
    playbackSpeed_ = 1.0;
    sizeFactor_ = 1;
    noVideoSize_ = QSize(500,270);
    decorationState_ = AllDecorations;

    ui->setupUi(this);
    setupMenu();
    setupPositionSlider();
    setupVolumeSlider();
    setupMpvWidget();
    setupMpvHost();

    connect(this, &MainWindow::fireUpdateSize, this, &MainWindow::sendUpdateSize,
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

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_action_file_open_quick_triggered()
{
    // Do nothing special for the moment, call menu_file_open instead
    on_action_file_open_triggered();
}

void MainWindow::on_action_file_open_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this, "Open file");
    mpvw->fileOpen(filename);
}

void MainWindow::on_action_file_close_triggered()
{
    on_action_play_stop_triggered();
}

void MainWindow::on_action_file_exit_triggered()
{
    close();
}

void MainWindow::on_action_view_hide_menu_triggered()
{
    // View/hide are unmanaged when in fullscreen mode
    if (fullscreenMode_)
        return;

    DecorationState nextState[] = { NoMenu, NoDecorations, AllDecorations };
    setUiDecorationState(nextState[static_cast<int>(decorationState_)]);
    fireUpdateSize();
}

void MainWindow::on_action_view_hide_seekbar_toggled(bool checked)
{
    if (checked)
        ui->seekbar->show();
    else
        ui->seekbar->hide();
    ui->control_section->adjustSize();
    fireUpdateSize();
}

void MainWindow::on_action_view_hide_controls_toggled(bool checked)
{
    if (checked)
        ui->controlbar->show();
    else
        ui->controlbar->hide();
    ui->control_section->adjustSize();
    fireUpdateSize();
}

void MainWindow::on_action_view_hide_information_toggled(bool checked)
{
    if (checked)
        ui->info_stats->show();
    else
        ui->info_stats->hide();
    ui->info_section->adjustSize();
    fireUpdateSize();
}

void MainWindow::on_action_view_hide_statistics_toggled(bool checked)
{
    // this currently does nothing, because info and statistics are controlled
    // by the same widget.  We're going to manage what's shown by it
    // ourselves, and turn that on or off depending upon the settings here.
    (void)checked;

    fireUpdateSize();
}

void MainWindow::on_action_view_hide_status_toggled(bool checked)
{
    if (checked)
        ui->statusbar->show();
    else
        ui->statusbar->hide();
    ui->info_section->adjustSize();
    fireUpdateSize();
}

void MainWindow::on_action_view_hide_subresync_toggled(bool checked)
{
    (void)checked;

    fireUpdateSize();
}

void MainWindow::on_action_view_hide_playlist_toggled(bool checked)
{
    // playlist window is unimplemented for now
    (void)checked;

    fireUpdateSize();
}

void MainWindow::on_action_view_hide_capture_toggled(bool checked)
{
    (void)checked;

    fireUpdateSize();
}

void MainWindow::on_action_view_hide_navigation_toggled(bool checked)
{
    (void)checked;

    fireUpdateSize();
}

void MainWindow::on_action_view_presets_minimal_triggered()
{
    setUiDecorationState(NoDecorations);
    ui->action_view_hide_seekbar->setChecked(false);
    ui->action_view_hide_controls->setChecked(false);
    ui->action_view_hide_information->setChecked(false);
    ui->action_view_hide_statistics->setChecked(false);
    ui->action_view_hide_status->setChecked(false);
    ui->action_view_hide_subresync->setChecked(false);
    ui->action_view_hide_capture->setChecked(false);
    ui->action_view_hide_navigation->setChecked(false);
}

void MainWindow::on_action_view_presets_compact_triggered()
{
    // we should set our menu state to something like Framed, but we can't
    // reliably do that across window managers.
    setUiDecorationState(NoDecorations);
    ui->action_view_hide_menu->setChecked(true);
    ui->action_view_hide_seekbar->setChecked(true);
    ui->action_view_hide_controls->setChecked(false);
    ui->action_view_hide_information->setChecked(false);
    ui->action_view_hide_statistics->setChecked(false);
    ui->action_view_hide_status->setChecked(false);
    ui->action_view_hide_subresync->setChecked(false);
    ui->action_view_hide_capture->setChecked(false);
    ui->action_view_hide_navigation->setChecked(false);
}

void MainWindow::on_action_view_presets_normal_triggered()
{
    setUiDecorationState(AllDecorations);
    ui->action_view_hide_menu->setChecked(true);
    ui->action_view_hide_seekbar->setChecked(true);
    ui->action_view_hide_controls->setChecked(true);
    ui->action_view_hide_information->setChecked(true);
    ui->action_view_hide_statistics->setChecked(false);
    ui->action_view_hide_status->setChecked(true);
    ui->action_view_hide_subresync->setChecked(false);
    ui->action_view_hide_capture->setChecked(false);
    ui->action_view_hide_navigation->setChecked(false);
}

void MainWindow::on_action_view_fullscreen_toggled(bool checked)
{
    setFullscreenMode(checked);

    if (checked) {
        menuBar()->hide();
        ui->control_section->hide();
        ui->info_section->hide();
    } else {
        if (ui->action_view_hide_menu->isChecked())
            menuBar()->show();
        ui->control_section->show();
        ui->info_section->show();
    }
}

void MainWindow::on_action_view_zoom_050_triggered()
{
    sizeFactor_ = 0.5;
    updateSize();
}

void MainWindow::on_action_view_zoom_100_triggered()
{
    sizeFactor_ = 1.0;
    updateSize();
}

void MainWindow::on_action_view_zoom_200_triggered()
{
    sizeFactor_ = 2.0;
    updateSize();
}

void MainWindow::on_action_view_zoom_autofit_triggered()
{
    // TODO: work out the logic for this.  In the meantime, set to manual
    // sized.
    sizeFactor_ = 0.0;
}

void MainWindow::on_action_view_zoom_autofit_larger_triggered()
{
    // TODO: work out the logic for this.  In the meantime, set to manual
    // sized.
    sizeFactor_ = 0.0;
}

void MainWindow::on_action_view_zoom_disable_triggered()
{
    sizeFactor_ = 0.0;
}

void MainWindow::on_action_play_pause_toggled(bool checked)
{
    mpvw->setPaused(checked);
    mpvw_pausedChanged(checked);

    ui->pause->setChecked(checked);
    ui->action_play_pause->setChecked(checked);
}

void MainWindow::on_action_play_stop_triggered()
{
    doMpvStopPlayback();
}

void MainWindow::on_action_play_frame_backward_triggered()
{
    mpvw->stepBackward();
    setPaused(true);
    updatePlaybackStatus();
}

void MainWindow::on_action_play_frame_forward_triggered()
{
    mpvw->stepForward();
    setPaused(true);
    updatePlaybackStatus();
}

void MainWindow::on_action_play_rate_decrease_triggered()
{
    setPlaybackSpeed(playbackSpeed() / 2);
}

void MainWindow::on_action_play_rate_increase_triggered()
{
    setPlaybackSpeed(playbackSpeed() * 2);
}

void MainWindow::on_action_play_rate_reset_triggered()
{
    if (playbackSpeed() == 1.0)
        return;
    setPlaybackSpeed(1.0);
}

void MainWindow::action_play_audio_selected(QVariant data)
{
    int64_t id = data.toLongLong();
    mpvw->setAudioTrack(id);
}

void MainWindow::action_play_subtitles_selected(QVariant data)
{
    int64_t id = data.toLongLong();
    mpvw->setSubtitleTrack(id);
}

void MainWindow::action_play_video_tracks_selected(QVariant data)
{
    int64_t id = data.toLongLong();
    mpvw->setVideoTrack(id);
}

void MainWindow::on_action_play_volume_up_triggered()
{
    int newvol = std::min(volumeSlider_->value() + 10, 100.0);
    doMpvSetVolume(newvol);
    volumeSlider_->setValue(newvol);
}

void MainWindow::on_action_play_volume_down_triggered()
{
    int newvol = std::max(volumeSlider_->value() - 10, 0.0);
    doMpvSetVolume(newvol);
    volumeSlider_->setValue(newvol);
}

void MainWindow::on_action_play_volume_mute_toggled(bool checked)
{
    if (!isPlaying_)
        return;
    mpvw->setMute(checked);
    ui->mute->setIcon(QIcon(checked ? ":/images/controls/speaker2.png" :
                                      ":/images/controls/speaker1.png"));
    ui->action_play_volume_mute->setChecked(checked);
    ui->mute->setChecked(checked);
}

void MainWindow::on_action_navigate_chapters_previous_triggered()
{
    int64_t chapter = mpvw->chapter();
    if (chapter > 0) chapter--;
    mpvw->setChapter(chapter);
}

void MainWindow::on_action_navigate_chapters_next_triggered()
{
    int64_t chapter = mpvw->chapter();
    chapter++;
    if (!mpvw->setChapter(chapter)) {
        // most likely the reason why we're here is because the requested
        // chapter number is a past-the-end value, so halt playback.  If mpv
        // was playing back a playlist, this stops it.  But we intend to do
        // our own playlist parsing anyway, so no biggie.
        doMpvStopPlayback();
    }
}

void MainWindow::menu_navigate_chapters(QVariant data)
{
    mpvw->setChapter(data.toInt());
}

void MainWindow::on_action_help_about_triggered()
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
      "You should have received a copy of the GNU General Public License along "
      "with this program; if not, write to the Free Software Foundation, Inc., "
      "51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.");
}

void MainWindow::position_sliderMoved(int position)
{
    mpvw->setTime(position);
}

void MainWindow::on_play_clicked()
{
    if (!isPlaying())
        return;
    if (isPaused()) {
        mpvw->setPaused(false);
        mpvw_pausedChanged(false);
        ui->pause->setChecked(false);
    }
    on_action_play_rate_reset_triggered();
}

void MainWindow::volume_sliderMoved(double position)
{
    doMpvSetVolume(position);
}

void MainWindow::mpvw_playTimeChanged(double time)
{
    positionSlider()->setValue(time >= 0 ? time : 0);
    updateTime();
}

void MainWindow::mpvw_playLengthChanged(double length)
{
    positionSlider()->setMaximum(length >= 0 ? length : 0);
    updateTime();
}

void MainWindow::mpvw_playbackStarted()
{
    setPlaying(true);
    mpvw_pausedChanged(false);
    setUiEnabledState(true);
}

void MainWindow::mpvw_pausedChanged(bool yes)
{
    setPaused(yes);
    updatePlaybackStatus();
}

void MainWindow::mpvw_playbackFinished()
{
    doMpvStopPlayback(true);
    setUiEnabledState(false);
}

void MainWindow::mpvw_mediaTitleChanged(QString title)
{
    QString window_title("Media Player Classic Qute Theater");

    if (!title.isEmpty())
        window_title.append(" - ").append(title);
    setWindowTitle(window_title);
}

void MainWindow::mpvw_chaptersChanged(QVariantList chapters)
{
    // Here we add (named) ticks to the position slider.
    positionSlider()->clearTicks();
    for (QVariant v : chapters) {
        QMap<QString, QVariant> node = v.toMap();
        positionSlider()->setTick(node["time"].toDouble(), node["title"].toString());
    }

    // Here we populate the chapters menu with the chapters.
    QAction *action;
    data_emitter *emitter;
    ui->menu_navigate_chapters->clear();
    int index = 0;
    for (QVariant v : chapters) {
        QMap<QString, QVariant> node = v.toMap();
        action = new QAction(this);
        action->setText(QString("[%1] - %2").arg(
                            toDateFormat(node["time"].toDouble()),
                            node["title"].toString()));
        emitter = new data_emitter(action);
        emitter->data = index;
        connect(action, &QAction::triggered, emitter, &data_emitter::got_something);
        connect(emitter, &data_emitter::heres_something, this, &MainWindow::menu_navigate_chapters);
        ui->menu_navigate_chapters->addAction(action);
        index++;
    }
}

void MainWindow::mpvw_tracksChanged(QVariantList tracks)
{
    auto str = [](QVariantMap map, QString key) {
        return map[key].toString();
    };
    auto formatter = [&str](QVariantMap track) {
        QString output;
        output.append(QString("%1: ").arg(str(track,"id")));
        if (track.contains("codec"))
            output.append(QString("[%1] ").arg(str(track,"codec")));
        if (track.contains("lang"))
            output.append(QString("%1 ").arg(str(track,"lang")));
        if (track.contains("title"))
            output.append(QString("- %1 ").arg(str(track,"title")));
        return output;
    };

    ui->menu_play_audio->clear();
    ui->menu_play_subtitles->clear();
    ui->menu_play_video->clear();
    for (QVariant track : tracks) {
        QVariantMap t = track.toMap();
        QAction *action = new QAction(this);
        data_emitter *de = new data_emitter(action);
        action->setText(formatter(t));
        de->data = t["id"];
        connect(action, &QAction::triggered, de, &data_emitter::got_something);
        if (str(t,"type") == "audio") {
            connect(de, &data_emitter::heres_something, this, &MainWindow::action_play_audio_selected);
            ui->menu_play_audio->addAction(action);
        } else if (str(t,"type") == "sub") {
            connect(de, &data_emitter::heres_something, this, &MainWindow::action_play_subtitles_selected);
            ui->menu_play_subtitles->addAction(action);
        } else if (str(t,"type") == "video") {
            connect(de, &data_emitter::heres_something, this, &MainWindow::action_play_video_tracks_selected);
            ui->menu_play_video->addAction(action);
        } else {
            // the track is unused by us for now, so delete the stuff we were
            // going to associate with it
            delete de;
            delete action;
        }
    }
}

void MainWindow::mpvw_videoSizeChanged(QSize size)
{
    (void)size;
    updateSize();
}

void MainWindow::sendUpdateSize()
{
    updateSize();
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

bool MainWindow::isPlaying()
{
    return isPlaying_;
}

bool MainWindow::isPaused()
{
    return isPaused_;
}

double MainWindow::playbackSpeed()
{
    return playbackSpeed_;
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

void MainWindow::setPlaying(bool yes)
{
    isPlaying_ = yes;
}

void MainWindow::setPaused(bool yes)
{
    isPaused_ = yes;
}

void MainWindow::setPlaybackSpeed(double speed)
{
    playbackSpeed_ = std::max(0.125, std::min(8.0, speed));
    mpvw->setSpeed(speed);
    mpvw->showMessage(QString("Speed: %1").arg(speed));
}

void MainWindow::setSizeFactor(double factor)
{
    sizeFactor_ = factor;
}

void MainWindow::setupMenu()
{
    // Work around separators with text in the designer not showing as
    // sections
    ui->menu_play_after->insertSection(ui->action_play_after_once_exit,
                                       tr("Once"));
    ui->menu_play_after->insertSection(ui->action_play_after_always_exit,
                                       tr("Every time"));

    ui->info_stats->setVisible(false);
}

void MainWindow::setupPositionSlider()
{
    positionSlider_ = new QMediaSlider();
    ui->seekbar->layout()->addWidget(positionSlider_);
    connect(positionSlider_, &QMediaSlider::sliderMoved, this, &MainWindow::position_sliderMoved);
}

void MainWindow::setupVolumeSlider()
{
    volumeSlider_ = new QVolumeSlider();
    volumeSlider_->setMinimumWidth(50);
    volumeSlider_->setMinimum(0);
    volumeSlider_->setMaximum(100);
    volumeSlider_->setValue(100);
    ui->controlbar->layout()->addWidget(volumeSlider_);
    connect(volumeSlider_, &QVolumeSlider::sliderMoved, this, &MainWindow::volume_sliderMoved);
}

void MainWindow::setupMpvWidget()
{
    mpvw = new MpvWidget(this);
    connect(mpvw, &MpvWidget::playTimeChanged, this, &MainWindow::mpvw_playTimeChanged);
    connect(mpvw, &MpvWidget::playLengthChanged, this, &MainWindow::mpvw_playLengthChanged);
    connect(mpvw, &MpvWidget::playbackStarted, this, &MainWindow::mpvw_playbackStarted);
    connect(mpvw, &MpvWidget::pausedChanged, this, &MainWindow::mpvw_pausedChanged);
    connect(mpvw, &MpvWidget::playbackFinished, this, &MainWindow::mpvw_playbackFinished);
    connect(mpvw, &MpvWidget::mediaTitleChanged, this, &MainWindow::mpvw_mediaTitleChanged);
    connect(mpvw, &MpvWidget::chaptersChanged, this, &MainWindow::mpvw_chaptersChanged);
    connect(mpvw, &MpvWidget::tracksChanged, this, &MainWindow::mpvw_tracksChanged);
    connect(mpvw, &MpvWidget::videoSizeChanged, this, &MainWindow::mpvw_videoSizeChanged);
}

void MainWindow::setupMpvHost()
{
    // Wrap mpvw in a special QMainWindow widget so that the playlist window
    // will dock around it rather than ourselves
    mpvHost_ = new QMainWindow(this);
    mpvHost_->setStyleSheet("background-color: black; background: center url("
                            ":/images/bitmaps/blank-screen.png) no-repeat;");
    mpvHost_->setCentralWidget(mpvw);
    mpvHost_->setSizePolicy(QSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred));
    ui->mpv_widget->layout()->addWidget(mpvHost_);

    connectButtonsToActions();
    globalizeAllActions();
    setUiEnabledState(false);
}

void MainWindow::connectButtonsToActions()
{
    connect(ui->pause, &QPushButton::toggled, ui->action_play_pause, &QAction::toggled);
    connect(ui->stop, &QPushButton::clicked, ui->action_play_stop, &QAction::triggered);

    connect(ui->speedDecrease, &QPushButton::clicked, ui->action_play_rate_decrease, &QAction::triggered);
    connect(ui->speedIncrease, &QPushButton::clicked, ui->action_play_rate_increase, &QAction::triggered);

    connect(ui->skipBackward, &QPushButton::clicked, ui->action_navigate_chapters_previous, &QAction::triggered);
    connect(ui->stepBackward, &QPushButton::clicked, ui->action_play_frame_backward, &QAction::triggered);
    connect(ui->stepForward, &QPushButton::clicked, ui->action_play_frame_forward, &QAction::triggered);
    connect(ui->skipForward, &QPushButton::clicked, ui->action_navigate_chapters_next, &QAction::triggered);

    connect(ui->mute, &QPushButton::toggled, ui->action_play_volume_mute, &QAction::toggled);
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
    ui->action_view_hide_menu->setText(actionTexts[static_cast<int>(state)]);
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
    ui->action_play_pause->setChecked(false);

    ui->action_file_close->setEnabled(enabled);
    ui->action_file_save_copy->setEnabled(enabled);
    ui->action_file_save_image->setEnabled(enabled);
    ui->action_file_save_thumbnails->setEnabled(enabled);
    ui->action_file_load_subtitle->setEnabled(enabled);
    ui->action_file_save_subtitle->setEnabled(enabled);
    ui->action_file_subtitle_database_download->setEnabled(enabled);
    ui->action_play_pause->setEnabled(enabled);
    ui->action_play_stop->setEnabled(enabled);
    ui->action_play_frame_backward->setEnabled(enabled);
    ui->action_play_frame_forward->setEnabled(enabled);
    ui->action_play_rate_decrease->setEnabled(enabled);
    ui->action_play_rate_increase->setEnabled(enabled);
    ui->action_play_rate_reset->setEnabled(enabled);
    ui->action_play_volume_up->setEnabled(enabled);
    ui->action_play_volume_down->setEnabled(enabled);
    ui->action_play_volume_mute->setEnabled(enabled);
    ui->action_navigate_chapters_previous->setEnabled(enabled);
    ui->action_navigate_chapters_next->setEnabled(enabled);
    ui->action_favorites_add->setEnabled(enabled);

    ui->menu_play_audio->setEnabled(enabled);
    ui->menu_play_subtitles->setEnabled(enabled);
    ui->menu_play_video->setEnabled(enabled);
    ui->menu_navigate_chapters->setEnabled(enabled);
}

void MainWindow::updateTime()
{
    double play_time = mpvw->playTime();
    double play_length = mpvw->playLength();
    ui->time->setText(QString("%1 / %2").arg(toDateFormat(play_time),toDateFormat(play_length)));
}

void MainWindow::updatePlaybackStatus()
{
    ui->status->setText(isPlaying() ? isPaused() ? "Paused" : "Playing" : "Stopped");
}

void MainWindow::updateSize(bool first_run)
{
    if (sizeFactor() <= 0 || fullscreenMode() || isMaximized())
        return;

    QSize sz_player = isPlaying() ? mpvw->videoSize() : noVideoSize();
    double factor_to_use = isPlaying() ? sizeFactor() : std::max(1.0, sizeFactor());
    QSize sz_wanted(sz_player.width()*factor_to_use + 0.5,
                    sz_player.height()*factor_to_use + 0.5);
    QSize sz_current = mpvw->size();
    QSize sz_window = size();
    QSize sz_desired = sz_wanted + sz_window - sz_current;

    QDesktopWidget *desktop = qApp->desktop();
    if (first_run)
        setGeometry(QStyle::alignedRect(
                    Qt::LeftToRight, Qt::AlignCenter, sz_desired,
                    desktop->availableGeometry(desktop->screenNumber(QCursor::pos()))));
    else
        setGeometry(QStyle::alignedRect(
                    Qt::LeftToRight, Qt::AlignCenter, sz_desired,
                    desktop->availableGeometry(this)));
}

void MainWindow::doMpvStopPlayback(bool dry_run)
{
    if (!dry_run)
        mpvw->stopPlayback();
    setPlaying(false);
    updatePlaybackStatus();
    updateSize();
}

void MainWindow::doMpvSetVolume(int volume)
{
    mpvw->setVolume(volume);
    mpvw->showMessage(QString("Volume :%1%").arg(volume));
}

static QString toDateFormat(double time) {
    int t = time*1000 + 0.5;
    if (t < 0)
        t = 0;
    int hr = t/3600000;
    int mn = t/60000 % 60;
    int se = t%60000 / 1000;
    int fr = t % 1000;
    return QString("%1:%2:%3.%4").arg(QString().number(hr))
            .arg(QString().number(mn),2,'0')
            .arg(QString().number(se),2,'0')
            .arg(QString().number(fr),3,'0');
}
