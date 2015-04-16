#ifndef HOSTWINDOW_H
#define HOSTWINDOW_H

#include <QWidget>

#include <mpv/client.h>

namespace Ui {
class HostWindow;
}

class HostWindow : public QWidget
{
    Q_OBJECT

public:
    explicit HostWindow(QWidget *parent = 0);
    ~HostWindow();

private slots:
    void menu_file_open();
    void menu_file_close();

    void menu_view_zoom_50();
    void menu_view_zoom_100();
    void menu_view_zoom_200();
    void menu_view_zoom_auto();
    void menu_view_zoom_autolarger();

    void mpv_events();

    void on_position_sliderMoved(int position);

    void on_pause_clicked();

    void on_pause_clicked(bool checked);

    void on_play_clicked();

    void on_stop_clicked();

    void on_speedDecrease_clicked();

    void on_speedIncrease_clicked();

    void on_skipBackward_clicked();

    void on_skipForward_clicked();

    void on_stepBackward_clicked();

    void on_stepForward_clicked();

    void on_volume_valueChanged(int position);

    void on_mute_clicked(bool checked);

signals:
    void fire_mpv_events();

private:
    Ui::HostWindow *ui;
    mpv_handle *mpv;
    int video_width;
    int video_height;
    int no_video_width;
    int no_video_height;
    double play_time;
    double play_length;
    bool is_playing;
    bool is_paused;
    double speed;
    bool is_muted;
    double size_factor;

    void handle_mpv_event(mpv_event *event);

    void addMenu();
    void bootMpv();
    void update_time();
    void update_status();
    void update_size();
    void viewport_shrink_size();
    void mpv_stop(bool dry_run = false);
    void mpv_show_message(const char* text);
    void mpv_set_speed(double speed);

    void me_play_time(double time);
    void me_length(double time);
    void me_started();
    void me_pause(bool yes);
    void me_finished();
    void me_title();
    void me_chapter(QVariant v);
    void me_track(QVariant v);

    void me_size(int64_t width, int64_t height);
};

#endif // HOSTWINDOW_H
