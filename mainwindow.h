#ifndef HOSTWINDOW_H
#define HOSTWINDOW_H

#include <QMainWindow>
#include <mpvwidget.h>
#include <QMenuBar>
#include "qdrawnslider.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    typedef enum { msShown = 0, msNoMenu = 1, msNothing = 2 } menuState;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_action_file_open_quick_triggered();
    void on_action_file_open_triggered();
    void on_action_file_close_triggered();
    void on_action_file_exit_triggered();

    void on_action_view_hide_menu_triggered();
    void on_action_view_hide_seekbar_toggled(bool checked);
    void on_action_view_hide_controls_toggled(bool checked);
    void on_action_view_hide_information_toggled(bool checked);
    void on_action_view_hide_statistics_toggled(bool checked);
    void on_action_view_hide_status_toggled(bool checked);
    void on_action_view_hide_subresync_toggled(bool checked);
    void on_action_view_hide_playlist_toggled(bool checked);
    void on_action_view_hide_capture_toggled(bool checked);
    void on_action_view_hide_navigation_toggled(bool checked);

    void on_action_view_presets_minimal_triggered();
    void on_action_view_presets_compact_triggered();
    void on_action_view_presets_normal_triggered();

    void on_action_view_fullscreen_toggled(bool checked);

    void on_action_view_zoom_050_triggered();
    void on_action_view_zoom_100_triggered();
    void on_action_view_zoom_200_triggered();
    void on_action_view_zoom_autofit_triggered();
    void on_action_view_zoom_autofit_larger_triggered();
    void on_action_view_zoom_disable_triggered();

    void on_action_play_pause_toggled(bool checked);
    void on_action_play_stop_triggered();
    void on_action_play_frame_backward_triggered();
    void on_action_play_frame_forward_triggered();
    void on_action_play_rate_decrease_triggered();
    void on_action_play_rate_increase_triggered();
    void on_action_play_rate_reset_triggered();

    void action_play_audio_selected(QVariant data);
    void action_play_subtitles_selected(QVariant data);
    void action_play_video_tracks_selected(QVariant data);

    void on_action_play_volume_up_triggered();
    void on_action_play_volume_down_triggered();
    void on_action_play_volume_mute_toggled(bool checked);

    void on_action_navigate_chapters_previous_triggered();
    void on_action_navigate_chapters_next_triggered();
    void menu_navigate_chapters(QVariant data);

    void on_action_help_about_triggered();

    void position_sliderMoved(int position);
    void on_play_clicked();
    void volume_sliderMoved(double position);

    void me_play_time();
    void me_length();
    void me_started();
    void me_pause(bool yes);
    void me_finished();
    void me_title();
    void me_chapters();
    void me_tracks();
    void me_size();

    void send_update_size();

signals:
    void fire_update_size();

private:
    Ui::MainWindow *ui;
    QMainWindow *mpv_host;
    MpvWidget *mpvw;
    QMediaSlider *ui_position;
    QVolumeSlider *ui_volume;

    menuState menu_state;
    bool is_fullscreen;

    QSize no_video_size;
    bool is_playing;
    bool is_paused;
    double speed;
    double size_factor;

    void action_connect_buttons();
    void action_globalize_all();
    void ui_set_menu_state(menuState state);
    void ui_reset_state(bool enabled);
    void update_time();
    void update_status();
    void update_size(bool first_run = false);
    void viewport_shrink_size();
    void mpv_stop(bool dry_run = false);
    void mpv_show_message(const char* text);
    void mpv_set_speed(double speed);
    void mpv_set_volume(int volume);
};

// Helper class for emitting data
class data_emitter : public QObject
{
    Q_OBJECT
public:
    QVariant data;
    data_emitter(QObject *parent) : QObject(parent) {}

public slots:
    void got_something() { heres_something(data); }

signals:
    void heres_something(QVariant data);
};


#endif // HOSTWINDOW_H
