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
#include <QDebug>

HostWindow::HostWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HostWindow)
{
    is_playing = false;
    speed = 1.0;
    size_factor = 1;
    no_video_size = QSize(500,270);

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
    connect(mpvw, &MpvWidget::me_chapters, this, &HostWindow::me_chapters);
    connect(mpvw, &MpvWidget::me_tracks, this, &HostWindow::me_tracks);
    connect(mpvw, &MpvWidget::me_size, this, &HostWindow::me_size);
    ui->mpv_host->layout()->addWidget(mpvw);

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
}

void HostWindow::menu_view_zoom_autolarger()
{
    // TODO: work out the logic for this.  In the meantime, set to manual
    // sized.
    size_factor = 0.0;
}

void HostWindow::menu_view_zoom_disable()
{
    size_factor = 0.0;
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

            submenu->addSeparator();

            action = new QAction(tr("Disable snapping"), this);
            action->setShortcut(QKeySequence("Alt+0"));
            connect(action, &QAction::triggered, this, &HostWindow::menu_view_zoom_disable);
            submenu->addAction(action);

    layout()->setMenuBar(bar);

    (void)subsubmenu;
}

void HostWindow::ui_reset_state(bool enabled)
{
    ui->position->setEnabled(enabled);

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
}

static QString to_date_fmt(double secs) {
    if (secs < 0.0)
        secs = 0;
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
    if (size_factor <= 0)
        return;

    QSize sz_player = is_playing ? mpvw->state_video_size_get() : no_video_size;
    QSize sz_wanted(sz_player.width()*size_factor + 0.5, sz_player.height()*size_factor + 0.5);
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

void HostWindow::me_play_time()
{
    double time = mpvw->state_play_time_get();
    ui->position->setValue(time >= 0 ? time : 0);
    update_time();
}

void HostWindow::me_length()
{
    double length = mpvw->state_play_length_get();
    ui->position->setMaximum(length >= 0 ? length : 0);
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
    // I'm going to have to reimplement QSlider for this. fml.
    // foreach item in chapters
    //   set position.tick at item[time] with item[title]
    qDebug() << chapters;
    (void)chapters;
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

void HostWindow::on_position_sliderMoved(int position)
{
    mpvw->property_time_set(position);
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
        // chapter number is a past-the-end value, so halt playback.  If mpv
        // was playing back a playlist, this stops it.  But we intend to do
        // our own playlist parsing anyway, so no biggie.
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
    if (!is_playing)
        return;
    mpvw->property_mute_set(checked);
    ui->mute->setIcon(QIcon(checked ? ":/images/controls/speaker2.png" :
                                      ":/images/controls/speaker1.png"));
}
