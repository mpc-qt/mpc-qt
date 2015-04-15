#include <sstream>
#include <stdexcept>

#include "hostwindow.h"
#include "ui_hostwindow.h"
#include <QMenuBar>
#include <QJsonDocument>
#include <QFileDialog>
#include <QTime>
#include <mpv/qthelper.hpp>
#include <QDebug>

static void wakeup(void *ctx)
{
    // Notify the Qt GUI thread to wake up so that it can process events with
    // mpv_wait_event().
    HostWindow *hostwindow = (HostWindow *)ctx;
    emit hostwindow->fire_mpv_events();
}

HostWindow::HostWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HostWindow)
{
    is_playing = false;
    speed = 1.0;
    is_muted = false;
    ui->setupUi(this);
    ui->info_stats->setVisible(false);

    addMenu();
    bootMpv();
}

HostWindow::~HostWindow()
{
    if (mpv)
        mpv_terminate_destroy(mpv);
    delete ui;
}

void HostWindow::menu_file_open()
{
    QString filename = QFileDialog::getOpenFileName(this, "Open file");
    if (mpv) {
        const QByteArray c_filename = filename.toUtf8();
        const char *args[] = {"loadfile", c_filename.data(), NULL};
        mpv_command_async(mpv, 0, args);
        me_started();
    }
}

void HostWindow::menu_file_close()
{
    on_stop_clicked();
}

void HostWindow::mpv_events()
{
    // Process all events, until the event queue is empty.
    while (mpv) {
        mpv_event *event = mpv_wait_event(mpv, 0);
        if (event->event_id == MPV_EVENT_NONE)
            break;
        handle_mpv_event(event);
    }
}

void HostWindow::handle_mpv_event(mpv_event *event)
{
    switch (event->event_id) {
    case MPV_EVENT_PROPERTY_CHANGE: {
        // TODO: Refactor this by looking up a map of functions to supported
        // property changes.
        mpv_event_property *prop = (mpv_event_property *)event->data;
        if (strcmp(prop->name, "time-pos") == 0) {
            if (prop->format == MPV_FORMAT_DOUBLE) {
                double time = *(double *)prop->data;
                me_play_time(time);
            } else if (prop->format == MPV_FORMAT_NONE) {
                // The property is unavailable, which probably means playback
                // was stopped.
                me_play_time(-1);
            }
        } else if (strcmp(prop->name, "length") == 0) {
            double time = prop->format == MPV_FORMAT_DOUBLE ?
                        *(double*)prop->data :
                        -1;
            me_length(time);
        } else if (strcmp(prop->name, "media-title") == 0) {
            me_title();
        } else if (strcmp(prop->name, "chapter-list") == 0) {
            me_chapter(mpv::qt::node_to_variant((mpv_node*)prop->data));
        } else {
            // Dump other properties as JSON for development purposes.
            // Eventually this will never be called as more control features
            // are implemented.
            if (prop->format == MPV_FORMAT_NODE) {
                QVariant v = mpv::qt::node_to_variant((mpv_node *)prop->data);
                QJsonDocument d = QJsonDocument::fromVariant(v);
                qDebug() << "Change property " + QString(prop->name) + ":\n"
                            << d.toJson().data();
            } else if (prop->format == MPV_FORMAT_DOUBLE) {
                qDebug() << "double change:" + QString::number(*(double*)prop->data) + "\n";
            } else {
                qDebug() << QString("Change property: %1 (%2)").arg(prop->name, QString::number(prop->format));
                if (prop->format == MPV_FORMAT_STRING) {
                    qDebug() << QString("%1 %2").arg(QString::number((long long)prop->data,16),reinterpret_cast<char*>(prop->data));
                }
            }
        }
        break;
    }
    case MPV_EVENT_VIDEO_RECONFIG: {
        // Retrieve the new video size.
        int64_t w, h;
        if (mpv_get_property(mpv, "dwidth", MPV_FORMAT_INT64, &w) >= 0 &&
            mpv_get_property(mpv, "dheight", MPV_FORMAT_INT64, &h) >= 0 &&
            w > 0 && h > 0)
        {
            // Note that the MPV_EVENT_VIDEO_RECONFIG event doesn't
            // necessarily imply a resize, and you should check yourself if
            // the video dimensions really changed.  This would happen for
            // example when playing back an mkv file that refers to other
            // videos.
            // mpv itself will scale/letter box the video to the container size
            // if the video doesn't fit.
            video_width = w;
            video_height = h;
        }
        break;
    }
    case MPV_EVENT_LOG_MESSAGE: {
        struct mpv_event_log_message *msg = (struct mpv_event_log_message *)event->data;
        std::stringstream ss;
        ss << "[" << msg->prefix << "] " << msg->level << ": " << msg->text;
        qDebug() << QString::fromStdString(ss.str());
        break;
    }
    case MPV_EVENT_SHUTDOWN: {
        me_finished();
        mpv_terminate_destroy(mpv);
        mpv = NULL;
        break;
    }
    default: ;
        // Ignore uninteresting or unknown events.
    }
}

void HostWindow::addMenu()
{
    // TODO: create relevant entires similar from mpc-hc.
    QMenuBar *bar = new QMenuBar(this);
    QMenu *menu = bar->addMenu(tr("&File"));
    QAction *on_open = new QAction(tr("&Open"), this);
    on_open->setShortcuts(QKeySequence::Open);
    on_open->setStatusTip(tr("Open a file"));
    connect(on_open, &QAction::triggered, this, &HostWindow::menu_file_open);
    menu->addAction(on_open);

    QAction *on_close = new QAction(tr("&Close file"), this);
    connect(on_close, &QAction::triggered, this, &HostWindow::menu_file_close);
    menu->addAction(on_close);

    layout()->setMenuBar(bar);
}

void HostWindow::bootMpv()
{
    mpv = mpv_create();
    if (!mpv)
        throw std::runtime_error("can't create mpv instance");

    // Create a video child window. Force Qt to create a native window, and
    // pass the window ID to the mpv wid option. Works on: X11, win32, Cocoa
    QWidget *mpv_container = ui->mpv_container;
    mpv_container->setAttribute(Qt::WA_NativeWindow);
    int64_t wid = static_cast<int64_t>(mpv_container->winId());
    mpv_set_option(mpv, "wid", MPV_FORMAT_INT64, &wid);

    // Let us receive property change events with MPV_EVENT_PROPERTY_CHANGE if
    // this property changes.
    mpv_observe_property(mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "media-title", MPV_FORMAT_STRING);
    mpv_observe_property(mpv, 0, "track-list", MPV_FORMAT_NODE);
    mpv_observe_property(mpv, 0, "chapter-list", MPV_FORMAT_NODE);
    mpv_observe_property(mpv, 0, "length", MPV_FORMAT_DOUBLE);

    // Request log messages with level "info" or higher.
    // They are received as MPV_EVENT_LOG_MESSAGE.
    mpv_request_log_messages(mpv, "info");

    // From this point on, the wakeup function will be called. The callback
    // can come from any thread, so we use the QueuedConnection mechanism to
    // relay the wakeup in a thread-safe way.
    connect(this, &HostWindow::fire_mpv_events, this, &HostWindow::mpv_events,
            Qt::QueuedConnection);
    mpv_set_wakeup_callback(mpv, wakeup, this);

    if (mpv_initialize(mpv) < 0)
        throw std::runtime_error("mpv failed to initialize");
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

void HostWindow::mpv_show_message(const char *text)
{
    const char *array[4] = { "show_text", text, "1000", NULL };
    qDebug() << mpv_command(mpv, array);
}

void HostWindow::mpv_set_speed(double speed)
{
    mpv_set_property(mpv, "speed", MPV_FORMAT_DOUBLE, &speed);

    QString txt("Speed: %1");
    mpv_show_message(txt.arg(speed).toUtf8().data());
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
    is_playing = false;
    update_status();
}

void HostWindow::me_title()
{
    char *media_title = NULL;
    QString window_title("Media Player Classic Qute Theater");
    if (mpv_get_property(mpv, "media-title", MPV_FORMAT_STRING,
                         &media_title) == 0)
        window_title.append(" - ").append(media_title);
    setWindowTitle(window_title);
    mpv_free(media_title);
}

void HostWindow::me_chapter(QVariant v)
{
    // Here we add (named) ticks to the position slider.
    // I'm going to have to reimplement QSlider for this. fml.
    // foreach item in v
    //   set position.tick at item[time] with item[title]
    (void)v;
}

void HostWindow::on_position_sliderMoved(int position)
{
    double p = (double)position;
    mpv_set_property(mpv, "time-pos", MPV_FORMAT_DOUBLE, &p);
}

void HostWindow::on_pause_clicked()
{
    me_pause(true);
}

void HostWindow::on_pause_clicked(bool checked)
{
    mpv_set_property_string(mpv, "pause", checked ? "yes" : "no");
    me_pause(checked);
}

void HostWindow::on_play_clicked()
{
    if (!is_playing)
        return;
    if (is_paused) {
        mpv_set_property_string(mpv, "pause", "no");
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
    mpv_command_string(mpv, "stop");
    is_playing = false;
    update_status();
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
    int64_t chapter = 0;
    mpv_get_property(mpv, "chapter", MPV_FORMAT_INT64, &chapter);
    if (chapter > 0) chapter--;
    mpv_set_property(mpv, "chapter", MPV_FORMAT_INT64, &chapter);
}

void HostWindow::on_skipForward_clicked()
{
    int64_t chapter = 0;
    mpv_get_property(mpv, "chapter", MPV_FORMAT_INT64, &chapter);
    chapter++;
    if (mpv_set_property(mpv, "chapter", MPV_FORMAT_INT64, &chapter) != 0) {
        // most likely the reason why we're here is because the requested
        // chapter number is a past-the-end value.
        on_stop_clicked();
    }
}

void HostWindow::on_stepBackward_clicked()
{
    mpv_command_string(mpv, "frame_back_step");
    is_paused = true;
    update_status();
}

void HostWindow::on_stepForward_clicked()
{
    mpv_command_string(mpv, "frame_step");
    is_paused = true;
    update_status();
}

void HostWindow::on_volume_valueChanged(int position)
{
    int64_t value = position;
    mpv_set_property(mpv, "volume", MPV_FORMAT_INT64, &value);
    QString msg("Volume :%1%");
    mpv_show_message(msg.arg(value).toUtf8().data());
}

void HostWindow::on_mute_clicked(bool checked)
{
    int64_t flag = checked ? 1 : 0;
    mpv_set_property(mpv, "mute", MPV_FORMAT_FLAG, &flag);
    ui->mute->setIcon(QIcon(checked ? ":/images/controls/speaker2.png" :
                                      ":/images/controls/speaker1.png"));
}
