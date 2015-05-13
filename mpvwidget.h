#ifndef MPVWIDGET_H
#define MPVWIDGET_H

#include <QWidget>
#include <QVariant>
#include <mpv/client.h>

class MpvWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MpvWidget(QWidget *parent = 0);
    ~MpvWidget();
    void show_message(QString message);

    // These are straightforward calls to libmpv
    void command_file_open(QString filename);
    void command_stop();
    void command_step_backward();
    void command_step_forward();

    // These are straightforward queries to libmpv
    int64_t property_chapter_get();
    bool property_chapter_set(int64_t chapter);
    QString property_media_title_get();
    void property_mute_set(bool yes);
    void property_pause_set(bool yes);
    void property_speed_set(double speed);
    void property_time_set(double position);
    void property_track_audio_set(int64_t id);
    void property_track_subtitle_set(int64_t id);
    void property_track_video_set(int64_t id);
    QString property_version_get();
    void property_volume_set(int64_t volume);

    // These query the tracked video state
    QVariantList state_chapters_get();
    double state_play_length_get();
    double state_play_time_get();
    QVariantList state_tracks_get();
    QSize state_video_size_get();

signals:
    void fire_mpv_events();

    // For the most part we do not pass the value of property changes over
    // during an event, we just notify that something happened.  If the
    // receiver really does want a property value or state information, they
    // can ask for it.  The UI should not be keeping that much track of player
    // state anyway, it should be communicating with its widgets.  Do not
    // fret, at worst it adds a single pointer redirection and a function
    // call.

    // Pronounce 'me' as 'mpv event'.
    void me_play_time();
    void me_length();
    void me_started();
    void me_pause(bool yes);
    void me_finished();
    void me_title();
    void me_chapters();
    void me_tracks();
    void me_size();

public slots:

private slots:
    void mpv_events();

private:
    mpv_handle *mpv;
    QSize video_size;
    double play_time;
    double play_length;
    QVariantList tracks;
    QVariantList chapters;

    void handle_mpv_event(mpv_event *event);
    QString get_property_string(char *property);
};

#endif // MPVWIDGET_H
