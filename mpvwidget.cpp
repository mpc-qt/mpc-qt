#include <QJsonDocument>
#include <QDebug>
#include <mpv/qthelper.hpp>
#include "mpvwidget.h"

static void wakeup(void *ctx)
{
    // Notify the Qt GUI thread to wake up so that it can process events with
    // mpv_wait_event().
    MpvWidget *widget= (MpvWidget*)ctx;
    emit widget->fire_mpv_events();
}

MpvWidget::MpvWidget(QWidget *parent) :
    QWidget(parent)
{
    mpv = mpv_create();
    if (!mpv)
        throw std::runtime_error("can't create mpv instance");

    // Force Qt to create a native window, and pass the window ID to the mpv
    // wid option. Works on: X11, win32, Cocoa
    setAttribute(Qt::WA_NativeWindow);
    int64_t wid = static_cast<int64_t>(winId());
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
    connect(this, &MpvWidget::fire_mpv_events, this, &MpvWidget::mpv_events,
            Qt::QueuedConnection);
    mpv_set_wakeup_callback(mpv, wakeup, this);

    if (mpv_initialize(mpv) < 0)
        throw std::runtime_error("mpv failed to initialize");
}

MpvWidget::~MpvWidget()
{
    if (mpv)
        mpv_terminate_destroy(mpv);
}

void MpvWidget::show_message(QString message)
{
    if (!mpv) return;
    const char *array[4] = { "show_text", message.toUtf8().data(), "1000",
                             NULL };
    mpv_command(mpv, array);
}

void MpvWidget::command_file_open(QString filename)
{
    if (!mpv) return;
    const QByteArray c_filename = filename.toUtf8();
    const char *args[] = {"loadfile", c_filename.data(), NULL};
    mpv_command_async(mpv, 0, args);
}

void MpvWidget::command_step_backward()
{
    mpv_command_string(mpv, "frame_back_step");
}

void MpvWidget::command_step_forward()
{
    mpv_command_string(mpv, "frame_step");
}

void MpvWidget::command_stop()
{
    if (!mpv) return;
    mpv_command_string(mpv, "stop");
}

int64_t MpvWidget::property_chapter_get()
{
    if (!mpv) return 0;
    int64_t chapter = 0;
    mpv_get_property(mpv, "chapter", MPV_FORMAT_INT64, &chapter);
    return chapter;
}

bool MpvWidget::property_chapter_set(int64_t chapter)
{
    if (!mpv) return false;
    return mpv_set_property(mpv, "chapter", MPV_FORMAT_INT64, &chapter) == 0;
}

QString MpvWidget::property_media_title_get()
{
    QString text;
    char *media_title;
    if (mpv_get_property(mpv, "media-title", MPV_FORMAT_STRING,
                         &media_title) == 0) {
        text = QString::fromUtf8(QByteArray(media_title));
        mpv_free(media_title);
    }
    return text;
}

void MpvWidget::property_mute_set(bool yes)
{
    int64_t flag = yes ? 1 : 0;
    mpv_set_property(mpv, "mute", MPV_FORMAT_FLAG, &flag);
}

void MpvWidget::property_pause_set(bool yes)
{
    if (!mpv) return;
    mpv_set_property_string(mpv, "pause", yes ? "yes" : "no");
}

void MpvWidget::property_speed_set(double speed)
{
    if (!mpv) return;
    mpv_set_property(mpv, "speed", MPV_FORMAT_DOUBLE, &speed);
}

void MpvWidget::property_time_set(double position)
{
    if (!mpv) return;
    mpv_set_property(mpv, "time-pos", MPV_FORMAT_DOUBLE, &position);
}

void MpvWidget::property_volume_set(int64_t volume)
{
    if (!mpv) return;
    mpv_set_property(mpv, "volume", MPV_FORMAT_INT64, &volume);
}

void MpvWidget::mpv_events()
{
    // Process all events, until the event queue is empty.
    while (mpv) {
        mpv_event *event = mpv_wait_event(mpv, 0);
        if (event->event_id == MPV_EVENT_NONE)
            break;
        handle_mpv_event(event);
    }
}

void MpvWidget::handle_mpv_event(mpv_event *event)
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
        } else if (strcmp(prop->name, "track-list") == 0) {
            if (prop->format != MPV_FORMAT_NODE)
                break;
            me_track(mpv::qt::node_to_variant((mpv_node *)prop->data));
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
        if (mpv_get_property(mpv, "width", MPV_FORMAT_INT64, &w) >= 0 &&
            mpv_get_property(mpv, "height", MPV_FORMAT_INT64, &h) >= 0 &&
            w > 0 && h > 0)
        {
            // Note that the MPV_EVENT_VIDEO_RECONFIG event doesn't
            // necessarily imply a resize, and you should check yourself if
            // the video dimensions really changed.  This would happen for
            // example when playing back an mkv file that refers to other
            // videos.
            // mpv itself will scale/letter box the video to the container
            // size if the video doesn't fit and automatic zooming is not
            // active.
            me_size(w,h);
        }
        break;
    }
    case MPV_EVENT_LOG_MESSAGE: {
        struct mpv_event_log_message *msg = (struct mpv_event_log_message *)event->data;
        qDebug() << QString("[%1] %2: %3").arg(msg->prefix, msg->level, msg->text);
        break;
    }
    case MPV_EVENT_SHUTDOWN: {
        mpv_terminate_destroy(mpv);
        mpv = NULL;
        me_finished();
        break;
    }
    default: ;
        // Ignore uninteresting or unknown events.
    }
}
