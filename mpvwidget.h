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

    void command_file_open(QString filename);

    void command_stop();
    void command_step_backward();
    void command_step_forward();

    int64_t property_chapter_get();
    bool property_chapter_set(int64_t chapter);
    QString property_media_title_get();
    void property_mute_set(bool yes);
    void property_pause_set(bool yes);
    void property_speed_set(double speed);
    void property_time_set(double position);
    void property_volume_set(int64_t volume);

signals:
    void fire_mpv_events();

    // Pronounce 'me' as 'mpv event'.
    void me_play_time(double time);
    void me_length(double time);
    void me_started();
    void me_pause(bool yes);
    void me_finished();
    void me_title();
    void me_chapter(QVariant v);
    void me_track(QVariant v);
    void me_size(int64_t width, int64_t height);

public slots:

private slots:
    void mpv_events();

private:
    mpv_handle *mpv;

    void bootMpv();
    void handle_mpv_event(mpv_event *event);

};

#endif // MPVWIDGET_H
