#include <sstream>
#include <stdexcept>

#include "hostwindow.h"
#include "ui_hostwindow.h"
#include <QDesktopWidget>
#include <QWindow>
#include <QMenuBar>
#include <QJsonDocument>
#include <QFileDialog>
#include <QTime>
#include <QMessageBox>
#include <QLibraryInfo>
#include <QDebug>

HostWindow::HostWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::HostWindow)
{
    is_fullscreen = false;
    is_playing = false;
    speed = 1.0;
    size_factor = 1;
    no_video_size = QSize(500,270);

    ui->setupUi(this);
    ui->info_stats->setVisible(false);
    connect(this, &HostWindow::fire_update_size, this, &HostWindow::send_update_size,
            Qt::QueuedConnection);

    ui_position = new QMediaSlider(this);
    ui->seekbar->layout()->addWidget(ui_position);
    connect(ui_position, &QMediaSlider::sliderMoved, this, &HostWindow::position_sliderMoved);

    mpvw = new MpvWidget(this);
    connect(mpvw, &MpvWidget::me_play_time, this, &HostWindow::me_play_time);
    connect(mpvw, &MpvWidget::me_length, this, &HostWindow::me_length);
    connect(mpvw, &MpvWidget::me_started, this, &HostWindow::me_started);
    connect(mpvw, &MpvWidget::me_pause, this, &HostWindow::me_pause);
    connect(mpvw, &MpvWidget::me_finished, this, &HostWindow::me_finished);
    connect(mpvw, &MpvWidget::me_title, this, &HostWindow::me_title);
    connect(mpvw, &MpvWidget::me_chapters, this, &HostWindow::me_chapters);
    connect(mpvw, &MpvWidget::me_tracks, this, &HostWindow::me_tracks);
    connect(mpvw, &MpvWidget::me_size, this, &HostWindow::me_size);

    // Wrap mpvw in a special QMainWindow widget so that the playlist window
    // will dock around it rather than ourselves
    mpv_host = new QMainWindow(this);
    mpv_host->setStyleSheet("background-color: black; background: center url("
                            ":/images/bitmaps/blank-screen.png) no-repeat;");
    mpv_host->setCentralWidget(mpvw);
    mpv_host->setSizePolicy(QSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred));
    ui->mpv_widget->layout()->addWidget(mpv_host);

    action_connect_buttons();
    globalize_actions();
    ui_reset_state(false);

    // Guarantee that the layout has been calculated.  It seems pointless, but
    // Without it the window will temporarily display at a larger size than
    // it needs to.
    setAttribute (Qt::WA_DontShowOnScreen, true);
    show();
    QEventLoop EventLoop (this);
    while (EventLoop.processEvents()) {}
    hide();
    setAttribute (Qt::WA_DontShowOnScreen, false);

    update_size(true);
}

HostWindow::~HostWindow()
{
    delete ui;
}

void HostWindow::on_action_file_open_quick_triggered()
{
    // Do nothing special for the moment, call menu_file_open instead
    on_action_file_open_triggered();
}

void HostWindow::on_action_file_open_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this, "Open file");
    mpvw->command_file_open(filename);
}

void HostWindow::on_action_file_close_triggered()
{
    on_action_play_stop_triggered();
}

void HostWindow::on_action_view_hide_menu_toggled(bool checked)
{
    // View/hide are unmanaged when in fullscreen mode
    if (is_fullscreen)
        return;

    if (checked)
        menuBar()->show();
    else
        menuBar()->hide();
    fire_update_size();
}

void HostWindow::on_action_view_hide_seekbar_toggled(bool checked)
{
    if (checked)
        ui->seekbar->show();
    else
        ui->seekbar->hide();
    ui->control_section->adjustSize();
    fire_update_size();
}

void HostWindow::on_action_view_hide_controls_toggled(bool checked)
{
    if (checked)
        ui->controlbar->show();
    else
        ui->controlbar->hide();
    ui->control_section->adjustSize();
    fire_update_size();
}

void HostWindow::on_action_view_hide_information_toggled(bool checked)
{
    if (checked)
        ui->info_stats->show();
    else
        ui->info_stats->hide();
    ui->info_section->adjustSize();
    fire_update_size();
}

void HostWindow::on_action_view_hide_statistics_toggled(bool checked)
{
    // this currently does nothing, because info and statistics are controlled
    // by the same widget.  We're going to manage what's shown by it
    // ourselves, and turn that on or off depending upon the settings here.
    (void)checked;

    fire_update_size();
}

void HostWindow::on_action_view_hide_status_toggled(bool checked)
{
    if (checked)
        ui->statusbar->show();
    else
        ui->statusbar->hide();
    ui->info_section->adjustSize();
    fire_update_size();
}

void HostWindow::on_action_view_hide_subresync_toggled(bool checked)
{
    if (checked)
        ui->statusbar->show();
    else
        ui->statusbar->hide();

    fire_update_size();
}

void HostWindow::on_action_view_hide_playlist_toggled(bool checked)
{
    // playlist window is unimplemented for now
    (void)checked;

    fire_update_size();
}

void HostWindow::on_action_view_hide_capture_toggled(bool checked)
{
    (void)checked;

    fire_update_size();
}

void HostWindow::on_action_view_hide_navigation_toggled(bool checked)
{
    (void)checked;

    fire_update_size();
}

void HostWindow::on_action_view_fullscreen_toggled(bool checked)
{
    is_fullscreen = checked;

    if (checked) {
        showFullScreen();
        menuBar()->hide();
        ui->control_section->hide();
        ui->info_section->hide();
    } else {
        showNormal();
        if (ui->action_view_hide_menu->isChecked())
            menuBar()->show();
        ui->control_section->show();
        ui->info_section->show();
    }
}

void HostWindow::on_action_view_zoom_050_triggered()
{
    size_factor = 0.5;
    update_size();
}

void HostWindow::on_action_view_zoom_100_triggered()
{
    size_factor = 1.0;
    update_size();
}

void HostWindow::on_action_view_zoom_200_triggered()
{
    size_factor = 2.0;
    update_size();
}

void HostWindow::on_action_view_zoom_autofit_triggered()
{
    // TODO: work out the logic for this.  In the meantime, set to manual
    // sized.
    size_factor = 0.0;
}

void HostWindow::on_action_view_zoom_autofit_larger_triggered()
{
    // TODO: work out the logic for this.  In the meantime, set to manual
    // sized.
    size_factor = 0.0;
}

void HostWindow::on_action_view_zoom_disable_triggered()
{
    size_factor = 0.0;
}

void HostWindow::on_action_play_rate_reset_triggered()
{
    if (speed == 1.0)
        return;
    speed = 1.0;
    mpv_set_speed(speed);
}

void HostWindow::on_action_play_volume_up_triggered()
{
    int newvol = std::min(ui->volume->value() + 10, 100);
    mpv_set_volume(newvol);
    ui->volume->setValue(newvol);
}

void HostWindow::on_action_play_volume_down_triggered()
{
    int newvol = std::max(ui->volume->value() - 10, 0);
    mpv_set_volume(newvol);
    ui->volume->setValue(newvol);
}

void HostWindow::menu_navigate_chapters(int data)
{
    mpvw->property_chapter_set(data);
}

void HostWindow::on_action_help_about_triggered()
{
    QMessageBox::about(this, "About Media Player Classic Qute Theater",
      "<h2>Media Player Classic Qute Theater</h2>"
      "<p>A clone of Media Player Classic written in Qt"
      "<p>Based on Qt " QT_VERSION_STR " and " + mpvw->property_version_get() +
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

void HostWindow::action_connect_buttons()
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

void HostWindow::globalize_actions()
{
    for (QAction *a : ui->menubar->actions()) {
        addAction(a);
    }
}

void HostWindow::ui_reset_state(bool enabled)
{
    ui_position->setEnabled(enabled);

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
    ui->volume->setEnabled(enabled);

    ui->pause->setChecked(false);
    ui->action_play_pause->setChecked(false);

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

    ui->menu_navigate_chapters->setEnabled(enabled);
}

static QString to_date_fmt(double secs) {
    int t = secs*1000 + 0.5;
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

void HostWindow::update_time()
{
    double play_time = mpvw->state_play_time_get();
    double play_length = mpvw->state_play_length_get();
    ui->time->setText(QString("%1 / %2").arg(to_date_fmt(play_time),to_date_fmt(play_length)));
}

void HostWindow::update_status()
{
    ui->status->setText(is_playing ? is_paused ? "Paused" : "Playing" : "Stopped");
}

void HostWindow::update_size(bool first_run)
{
    if (size_factor <= 0 || is_fullscreen || isMaximized())
        return;

    QSize sz_player = is_playing ? mpvw->state_video_size_get() : no_video_size;
    double factor_to_use = is_playing ? size_factor : std::max(1.0, size_factor);
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

void HostWindow::mpv_stop(bool dry_run)
{
    if (!dry_run)
        mpvw->command_stop();
    is_playing = false;
    update_status();
    update_size();
}

void HostWindow::mpv_set_speed(double speed)
{
    mpvw->property_speed_set(speed);
    mpvw->show_message(QString("Speed: %1").arg(speed));
}

void HostWindow::mpv_set_volume(int volume)
{
    mpvw->property_volume_set(volume);
    mpvw->show_message(QString("Volume :%1%").arg(volume));
}

void HostWindow::me_play_time()
{
    double time = mpvw->state_play_time_get();
    ui_position->setValue(time >= 0 ? time : 0);
    update_time();
}

void HostWindow::me_length()
{
    double length = mpvw->state_play_length_get();
    ui_position->setMaximum(length >= 0 ? length : 0);
    update_time();
}

void HostWindow::me_started()
{
    is_playing = true;
    me_pause(false);
    ui_reset_state(true);
}

void HostWindow::me_pause(bool yes)
{
    is_paused = yes;
    update_status();
}

void HostWindow::me_finished()
{
    mpv_stop(true);
    ui_reset_state(false);
}

void HostWindow::me_title()
{
    QString window_title("Media Player Classic Qute Theater");
    QString media_title = mpvw->property_media_title_get();

    if (!media_title.isEmpty())
        window_title.append(" - ").append(media_title);
    setWindowTitle(window_title);
}

void HostWindow::me_chapters()
{
    QVariantList chapters = mpvw->state_chapters_get();
    // Here we add (named) ticks to the position slider.
    ui_position->clearTicks();
    for (QVariant v : chapters) {
        QMap<QString, QVariant> node = v.toMap();
        ui_position->setTick(node["time"].toDouble(), node["title"].toString());
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
                            to_date_fmt(node["time"].toDouble()),
                            node["title"].toString()));
        emitter = new data_emitter(action);
        emitter->data = index;
        connect(action, &QAction::triggered, emitter, &data_emitter::got_something);
        connect(emitter, &data_emitter::heres_something, this, &HostWindow::menu_navigate_chapters);
        ui->menu_navigate_chapters->addAction(action);
        index++;
    }
}

void HostWindow::me_tracks()
{
    QVariantList tracks = mpvw->state_tracks_get();
    (void)tracks;
}

void HostWindow::me_size()
{
    update_size();
}

void HostWindow::position_sliderMoved(int position)
{
    mpvw->property_time_set(position);
}

void HostWindow::on_action_play_pause_toggled(bool checked)
{
    mpvw->property_pause_set(checked);
    me_pause(checked);    

    ui->pause->setChecked(checked);
    ui->action_play_pause->setChecked(checked);
}

void HostWindow::on_play_clicked()
{
    if (!is_playing)
        return;
    if (is_paused) {
        mpvw->property_pause_set(false);
        me_pause(false);
        ui->pause->setChecked(false);
    }
    if (speed != 1.0) {
        speed = 1.0;
        mpv_set_speed(speed);
    }
}

void HostWindow::on_action_play_stop_triggered()
{
    mpv_stop();
}

void HostWindow::on_action_play_rate_decrease_triggered()
{
    if (speed <= 0.125)
        return;
    speed /= 2;
    mpv_set_speed(speed);
}

void HostWindow::on_action_play_rate_increase_triggered()
{
    if (speed >= 8.0)
        return;
    speed *= 2;
    mpv_set_speed(speed);
}

void HostWindow::on_action_navigate_chapters_previous_triggered()
{
    int64_t chapter = mpvw->property_chapter_get();
    if (chapter > 0) chapter--;
    mpvw->property_chapter_set(chapter);
}

void HostWindow::on_action_navigate_chapters_next_triggered()
{
    int64_t chapter = mpvw->property_chapter_get();
    chapter++;
    if (!mpvw->property_chapter_set(chapter)) {
        // most likely the reason why we're here is because the requested
        // chapter number is a past-the-end value, so halt playback.  If mpv
        // was playing back a playlist, this stops it.  But we intend to do
        // our own playlist parsing anyway, so no biggie.
        mpv_stop();
    }
}

void HostWindow::on_action_play_frame_backward_triggered()
{
    mpvw->command_step_backward();
    is_paused = true;
    update_status();
}

void HostWindow::on_action_play_frame_forward_triggered()
{
    mpvw->command_step_forward();
    is_paused = true;
    update_status();
}

void HostWindow::on_volume_valueChanged(int position)
{
    mpv_set_volume(position);
}

void HostWindow::on_action_play_volume_mute_toggled(bool checked)
{
    if (!is_playing)
        return;
    mpvw->property_mute_set(checked);
    ui->mute->setIcon(QIcon(checked ? ":/images/controls/speaker2.png" :
                                      ":/images/controls/speaker1.png"));
    ui->action_play_volume_mute->setChecked(checked);
    ui->mute->setChecked(checked);
}

void HostWindow::send_update_size()
{
    update_size();
}
