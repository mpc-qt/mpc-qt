#ifndef HOSTWINDOW_H
#define HOSTWINDOW_H

#include <QMainWindow>
#include <mpvwidget.h>
#include <QMenuBar>
#include "qdrawnslider.h"
#include "manager.h"

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

    MpvWidget *mpvWidget();

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
    void setDiscState(bool playingADisc);

    void setupMenu();
    void setupPositionSlider();
    void setupVolumeSlider();
    void setupMpvWidget();
    void setupMpvHost();
    void setupSizing();
    void connectButtonsToActions();
    void globalizeAllActions();
    void setUiDecorationState(DecorationState state);
    void setUiEnabledState(bool enabled);
    void updateTime();
    void updatePlaybackStatus();
    void updateSize(bool first_run = false);
    void doMpvStopPlayback(bool dry_run = false);
    void doMpvSetVolume(int volume);

public slots:
    void setPlaybackState(PlaybackManager::PlaybackState state);
    void setPlaybackType(PlaybackManager::PlaybackType type);

private slots:
    void on_actionFileOpenQuick_triggered();
    void on_actionFileOpen_triggered();
    void on_actionFileClose_triggered();
    void on_actionFileExit_triggered();

    void on_actionViewHideMenu_triggered();
    void on_actionViewHideSeekbar_toggled(bool checked);
    void on_actionViewHideControls_toggled(bool checked);
    void on_actionViewHideInformation_toggled(bool checked);
    void on_actionViewHideStatistics_toggled(bool checked);
    void on_actionViewHideStatus_toggled(bool checked);
    void on_actionViewHideSubresync_toggled(bool checked);
    void on_actionViewHidePlaylist_toggled(bool checked);
    void on_actionViewHideCapture_toggled(bool checked);
    void on_actionViewHideNavigation_toggled(bool checked);

    void on_actionViewPresetsMinimal_triggered();
    void on_actionViewPresetsCompact_triggered();
    void on_actionViewPresetsNormal_triggered();

    void on_actionViewFullscreen_toggled(bool checked);

    void on_actionViewZoom050_triggered();
    void on_actionViewZoom100_triggered();
    void on_actionViewZoom200_triggered();
    void on_actionViewZoomAutofit_triggered();
    void on_actionViewZoomAutofitLarger_triggered();
    void on_actionViewZoomDisable_triggered();

    void on_actionPlayPause_toggled(bool checked);
    void on_actionPlayStop_triggered();
    void on_actionPlayFrameBackward_triggered();
    void on_actionPlayFrameForward_triggered();
    void on_actionPlayRateDecrease_triggered();
    void on_actionPlayRateIncrease_triggered();
    void on_actionPlayRateReset_triggered();

    void actionPlayAudio_selected(QVariant data);
    void actionPlaySubtitles_selected(QVariant data);
    void actionPlayVideoTracks_selected(QVariant data);

    void on_actionPlayVolumeUp_triggered();
    void on_actionPlayVolumeDown_triggered();
    void on_actionPlayVolumeMute_toggled(bool checked);

    void on_actionNavigateChaptersPrevious_triggered();
    void on_actionNavigateChaptersNext_triggered();
    void menuNavigateChapters_selected(QVariant data);

    void on_actionHelpHomepage_triggered();
    void on_actionHelpAbout_triggered();

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
};

// Helper class for emitting data
class DataEmitter : public QObject
{
    Q_OBJECT
public:
    QVariant data;
    DataEmitter(QObject *parent) : QObject(parent) {}

public slots:
    void gotSomething() { heresSomething(data); }

signals:
    void heresSomething(QVariant data);
};


#endif // HOSTWINDOW_H
