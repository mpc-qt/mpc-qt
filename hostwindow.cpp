#include <sstream>
#include <stdexcept>

#include "hostwindow.h"
#include "ui_hostwindow.h"
#include <QScreen>
#include <QWindow>
#include <QMenuBar>
#include <QJsonDocument>
#include <QFileDialog>
#include <QTime>
#include <QDebug>

HostWindow::HostWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HostWindow)
{
    is_playing = false;
    speed = 1.0;
    is_muted = false;
    size_factor = 1;
    ui->setupUi(this);
    ui->info_stats->setVisible(false);

    addMenu();
    mpvw = new MpvWidget(this);
    connect(mpvw, &MpvWidget::me_play_time, this, &HostWindow::me_play_time);
    connect(mpvw, &MpvWidget::me_length, this, &HostWindow::me_length);
    connect(mpvw, &MpvWidget::me_started, this, &HostWindow::me_started);
    connect(mpvw, &MpvWidget::me_pause, this, &HostWindow::me_pause);
    connect(mpvw, &MpvWidget::me_finished, this, &HostWindow::me_finished);
    connect(mpvw, &MpvWidget::me_title, this, &HostWindow::me_title);
    connect(mpvw, &MpvWidget::me_chapter, this, &HostWindow::me_chapter);
    connect(mpvw, &MpvWidget::me_track, this, &HostWindow::me_track);
    connect(mpvw, &MpvWidget::me_size, this, &HostWindow::me_size);
    ui->mpv_host->layout()->addWidget(mpvw);

    video_width = no_video_width = 500;
    video_height = no_video_height = 270;
}

HostWindow::~HostWindow()
{
    delete ui;
}

void HostWindow::menu_file_open()
{
    QString filename = QFileDialog::getOpenFileName(this, "Open file");
    mpvw->command_file_open(filename);
}

void HostWindow::menu_file_close()
{
    on_stop_clicked();
}

void HostWindow::menu_view_zoom_50()
{
    size_factor = 0.5;
    update_size();
}

void HostWindow::menu_view_zoom_100()
{
    size_factor = 1.0;
    update_size();
}

void HostWindow::menu_view_zoom_200()
{
    size_factor = 2.0;
    update_size();
}

void HostWindow::menu_view_zoom_auto()
{
    // TODO: work out the logic for this.  In the meantime, set to manual
    // sized.
    size_factor = 0.0;
    update_size();
}

void HostWindow::menu_view_zoom_autolarger()
{
    // TODO: work out the logic for this.  In the meantime, set to manual
    // sized.
    size_factor = 0.0;
    update_size();
}


void HostWindow::addMenu()
{
    // TODO: create relevant entires similar from mpc-hc.
    QMenuBar *bar = new QMenuBar(this);
    QMenu *menu, *submenu, *subsubmenu;
    QAction *action;
    menu = bar->addMenu(tr("&File"));
        action = new QAction(tr("&Open"), this);
        action->setShortcuts(QKeySequence::Open);
        action->setStatusTip(tr("Open a file"));
        connect(action, &QAction::triggered, this, &HostWindow::menu_file_open);
        menu->addAction(action);

        action = new QAction(tr("&Close file"), this);
        action->setShortcuts(QKeySequence::Close);
        connect(action, &QAction::triggered, this, &HostWindow::menu_file_close);
        menu->addAction(action);

    menu = bar->addMenu(tr("&View"));
        submenu = menu->addMenu(tr("&Zoom"));
            action = new QAction(tr("&50%"), this);
            action->setShortcut(QKeySequence("Alt+1"));
            connect(action, &QAction::triggered, this, &HostWindow::menu_view_zoom_50);
            submenu->addAction(action);

            action = new QAction(tr("&100%"), this);
            action->setShortcut(QKeySequence("Alt+2"));
            connect(action, &QAction::triggered, this, &HostWindow::menu_view_zoom_100);
            submenu->addAction(action);

            action = new QAction(tr("&200%"), this);
            action->setShortcut(QKeySequence("Alt+3"));
            connect(action, &QAction::triggered, this, &HostWindow::menu_view_zoom_200);
            submenu->addAction(action);

            action = new QAction(tr("Auto &Fit"), this);
            action->setShortcut(QKeySequence("Alt+4"));
            connect(action, &QAction::triggered, this, &HostWindow::menu_view_zoom_auto);
            submenu->addAction(action);

            action = new QAction(tr("Auto Fit (&Larger Only)"), this);
            action->setShortcut(QKeySequence("Alt+5"));
            connect(action, &QAction::triggered, this, &HostWindow::menu_view_zoom_autolarger);
            submenu->addAction(action);

    layout()->setMenuBar(bar);

    (void)subsubmenu;
}

static QString to_date_fmt(double secs) {
    int hr = secs/3600;
    int mn = fmod(secs/60, 60);
    int se = fmod(secs, 60);
    int fr = fmod(secs,1)*100;
    return QString("%1:%2:%3.%4").arg(QString().number(hr))
            .arg(QString().number(mn),2,'0')
            .arg(QString().number(se),2,'0')
            .arg(QString().number(fr),2,'0');
}

void HostWindow::update_time()
{
    ui->time->setText(QString("%1 / %2").arg(to_date_fmt(play_time),to_date_fmt(play_length)));
}

void HostWindow::update_status()
{
    ui->status->setText(is_playing ? is_paused ? "Paused" : "Playing" : "Stopped");
}

void HostWindow::update_size()
{
    if (size_factor <= 0)
        return;
    QSize sz_wanted(video_width*size_factor + 0.5, video_height*size_factor + 0.5);
    QSize sz_current = mpvw->size();
    QSize sz_window = size();
    QSize sz_desired = sz_wanted + sz_window - sz_current;
    emit resize(sz_desired);

    QScreen *scr = windowHandle()->screen();
    QSize sz_boundary = scr->availableSize();
    QSize pt_where = (sz_boundary - sz_desired)/2;
    emit move(pt_where.width(), pt_where.height());
}

void HostWindow::viewport_shrink_size()
{
    video_width = no_video_width;
    video_height = no_video_height;
    update_size();
}

void HostWindow::mpv_stop(bool dry_run)
{
    if (!dry_run)
        mpvw->command_stop();
    is_playing = false;
    update_status();
    viewport_shrink_size();
}

void HostWindow::mpv_set_speed(double speed)
{
    mpvw->property_speed_set(speed);
    mpvw->show_message(QString("Speed: %1").arg(speed));
}

void HostWindow::me_play_time(double time)
{
    play_time = time >= 0 ? time: 0;
    ui->position->setValue(time);
    update_time();
}

void HostWindow::me_length(double time)
{
    play_length = time >= 0 ? time : 0;
    ui->position->setMaximum(time);
    update_time();
}

void HostWindow::me_started()
{
    is_playing = true;
    me_pause(false);
}

void HostWindow::me_pause(bool yes)
{
    is_paused = yes;
    update_status();
}

void HostWindow::me_finished()
{
    mpv_stop(true);
}

void HostWindow::me_title()
{
    QString window_title("Media Player Classic Qute Theater");
    QString media_title = mpvw->property_media_title_get();

    if (!media_title.isEmpty())
        window_title.append(" - ").append(media_title);
    setWindowTitle(window_title);
}

void HostWindow::me_chapter(QVariant v)
{
    // Here we add (named) ticks to the position slider.
    // I'm going to have to reimplement QSlider for this. fml.
    // foreach item in v
    //   set position.tick at item[time] with item[title]
    (void)v;
}

void HostWindow::me_track(QVariant v)
{
    if (!v.canConvert<QVariantList>())
        return;
    QVariantList vl = v.toList();
    if (vl.empty())
        mpv_stop(true);
    else
        me_started();
}

void HostWindow::me_size(int64_t width, int64_t height)
{
    video_width = width;
    video_height = height;
    update_size();
    /*
    if (size_factor <= 0.0)
        return;
    QSize sz(video_width*size_factor + 0.5, video_height*size_factor + 0.5);
    ui->mpv_container->setMaximumSize(sz);
    ui->mpv_container->setMinimumSize(sz);*/
}

void HostWindow::on_position_sliderMoved(int position)
{
    mpvw->property_time_set(position);
}

void HostWindow::on_pause_clicked()
{
    me_pause(true);
}

void HostWindow::on_pause_clicked(bool checked)
{
    mpvw->property_pause_set(checked);
    me_pause(checked);
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

void HostWindow::on_stop_clicked()
{
    mpv_stop();
}

void HostWindow::on_speedDecrease_clicked()
{
    if (speed <= 0.125)
        return;
    speed /= 2;
    mpv_set_speed(speed);
}

void HostWindow::on_speedIncrease_clicked()
{
    if (speed >= 8.0)
        return;
    speed *= 2;
    mpv_set_speed(speed);
}

void HostWindow::on_skipBackward_clicked()
{
    int64_t chapter = mpvw->property_chapter_get();
    if (chapter > 0) chapter--;
    mpvw->property_chapter_set(chapter);
}

void HostWindow::on_skipForward_clicked()
{
    int64_t chapter = mpvw->property_chapter_get();
    chapter++;
    if (!mpvw->property_chapter_set(chapter)) {
        // most likely the reason why we're here is because the requested
        // chapter number is a past-the-end value.
        mpv_stop();
    }
}

void HostWindow::on_stepBackward_clicked()
{
    mpvw->command_step_backward();
    is_paused = true;
    update_status();
}

void HostWindow::on_stepForward_clicked()
{
    mpvw->command_step_forward();
    is_paused = true;
    update_status();
}

void HostWindow::on_volume_valueChanged(int position)
{
    mpvw->property_volume_set(position);
    mpvw->show_message(QString("Volume :%1%").arg(position));
}

void HostWindow::on_mute_clicked(bool checked)
{
    mpvw->property_mute_set(checked);
    ui->mute->setIcon(QIcon(checked ? ":/images/controls/speaker2.png" :
                                      ":/images/controls/speaker1.png"));
}
