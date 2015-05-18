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

    enum DecorationState { AllDecorations, NoMenu, NoDecorations };

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

    void mpvw_playTimeChanged(double time);
    void mpvw_playLengthChanged(double length);
    void mpvw_playbackStarted();
    void mpvw_pausedChanged(bool yes);
    void mpvw_playbackFinished();
    void mpvw_mediaTitleChanged(QString title);
    void mpvw_chaptersChanged(QVariantList chapters);
    void mpvw_tracksChanged(QVariantList tracks);
    void mpvw_videoSizeChanged(QSize size);

    void sendUpdateSize();

signals:
    void fireUpdateSize();

private:
    Ui::MainWindow *ui;
    QMainWindow *mpvHost_;
    MpvWidget *mpvw;
    QMediaSlider *positionSlider_;
    QVolumeSlider *volumeSlider_;

    DecorationState decorationState_;
    bool fullscreenMode_;

    QSize noVideoSize_;
    bool isPlaying_;        // TODO: move to mpvwidget
    bool isPaused_;         // TODO: move to mpvwidget
    double playbackSpeed_;  // TODO: move to mpvwidget
    double sizeFactor_;

    QMediaSlider *positionSlider();
    QVolumeSlider *volumeSlider();

    DecorationState decorationState();
    bool fullscreenMode();
    QSize noVideoSize();
    bool isPlaying();
    bool isPaused();
    double playbackSpeed();
    double sizeFactor();

    void setFullscreenMode(bool fullscreenMode);
    void setDecorationState(DecorationState decorationState);
    void setNoVideoSize(QSize size);
    void setPlaying(bool yes);
    void setPaused(bool yes);
    void setPlaybackSpeed(double speed);
    void setSizeFactor(double factor);

    void connectButtonsToActions();
    void globalizeAllActions();
    void setUiDecorationState(DecorationState state);
    void setUiEnabledState(bool enabled);
    void updateTime();
    void updatePlaybackStatus();
    void updateSize(bool first_run = false);
    void doMpvStopPlayback(bool dry_run = false);
    void doMpvSetVolume(int volume);
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
