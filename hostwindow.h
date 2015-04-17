#ifndef HOSTWINDOW_H
#define HOSTWINDOW_H

#include <QWidget>
#include <mpvwidget.h>
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

    void me_play_time();
    void me_length();
    void me_started();
    void me_pause(bool yes);
    void me_finished();
    void me_title();
    void me_chapters();
    void me_tracks();
    void me_size();

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

private:
    Ui::HostWindow *ui;
    MpvWidget* mpvw;
    QSize no_video_size;
    bool is_playing;
    bool is_paused;
    double speed;
    double size_factor;

    void addMenu();
    void ui_reset_state(bool enabled);
    void update_time();
    void update_status();
    void update_size();
    void viewport_shrink_size();
    void mpv_stop(bool dry_run = false);
    void mpv_show_message(const char* text);
    void mpv_set_speed(double speed);
};

#endif // HOSTWINDOW_H
