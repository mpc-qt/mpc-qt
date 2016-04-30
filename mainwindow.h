#ifndef HOSTWINDOW_H
#define HOSTWINDOW_H

#include <QMainWindow>
#include <mpvwidget.h>
#include <QMenuBar>
#include "qdrawnslider.h"
#include "manager.h"
#include "playlistwindow.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    enum DecorationState { AllDecorations, NoMenu, NoDecorations };
    enum ZoomMode { RegularZoom, Autofit, AutofitSmaller, AutofitLarger,
                    FitToWindow };

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    MpvWidget *mpvWidget();
    PlaylistWindow *playlistWindow();
    QList<QAction *> editableActions();

protected:
    void closeEvent(QCloseEvent *event);

private:
    QMediaSlider *positionSlider();
    QVolumeSlider *volumeSlider();

    DecorationState decorationState();
    bool fullscreenMode();
    QSize noVideoSize();
    double sizeFactor();

    void setFullscreenMode(bool fullscreenMode);
    void setNoVideoSize(QSize size);
    void setDiscState(bool playingADisc);

    void setupMenu();
    void setupPositionSlider();
    void setupVolumeSlider();
    void setupMpvWidget();
    void setupMpvHost();
    void setupPlaylist();
    void setupSizing();
    void connectButtonsToActions();
    void globalizeAllActions();
    void setUiDecorationState(DecorationState state);
    void setUiEnabledState(bool enabled);
    void updateTime();
    void updateFramedrops();
    void updatePlaybackStatus();
    void updateSize(bool first_run = false);
    void updateInfostats();
    void doMpvStopPlayback(bool dry_run = false);
    void doMpvSetVolume(int volume);

signals:
    void applicationShouldQuit();
    void fileOpened(QUrl what);
    void severalFilesOpened(QList<QUrl> what);
    void dvdbdOpened(QUrl what);
    void streamOpened(QUrl what);
    void recentOpened(TrackInfo info);
    void recentClear();
    void takeImage();
    void takeImageAutomatically();
    void optionsOpenRequested();
    void paused();
    void unpaused();
    void stopped();
    void stepBackward();
    void stepForward();
    void speedDown();
    void speedUp();
    void speedReset();
    void relativeSeek(bool forwards, bool small);
    void audioTrackSelected(int64_t id);
    void subtitleTrackSelected(int64_t id);
    void videoTrackSelected(int64_t id);
    void volumeChanged(int64_t volume);
    void volumeMuteChanged(bool muted);
    void chapterPrevious();
    void chapterNext();
    void chapterSelected(int64_t id);
    void timeSelected(double time);
    void zoomPresetChanged(int which);

    void playCurrentItemRequested();
    void fireUpdateSize();

public slots:
    void setRecentDocuments(QList<TrackInfo> tracks);
    void setTime(double time, double length);
    void setMediaTitle(QString title);
    void setChapterTitle(QString title);
    void setVideoSize(QSize size);
    void setSizeFactor(double factor);
    void setFitFactor(double fitFactor);
    void setZoomMode(ZoomMode mode);
    void setZoomPreset(int which, double fitFactor = -1.0);
    void setPlaybackState(PlaybackManager::PlaybackState state);
    void setPlaybackType(PlaybackManager::PlaybackType type);
    void setGlobalSeek(bool yes);
    void setChapters(QList<QPair<double,QString>> chapters);
    void setAudioTracks(QList<QPair<int64_t,QString>> tracks);
    void setVideoTracks(QList<QPair<int64_t,QString>> tracks);
    void setSubtitleTracks(QList<QPair<int64_t,QString>> tracks);
    void setFps(double fps);
    void setAvsync(double sync);
    void setDisplayFramedrops(int64_t count);
    void setDecoderFramedrops(int64_t count);
    void setPlaylistVisibleState(bool yes);

private slots:
    void on_actionFileOpenQuick_triggered();
    void on_actionFileOpen_triggered();
    void on_actionFileOpenDvdbd_triggered();
    void on_actionFileOpenDirectory_triggered();
    void on_actionFileOpenNetworkStream_triggered();
    void on_actionFileRecentClear_triggered();
    void on_actionFileSaveImage_triggered();
    void on_actionFileSaveImageAuto_triggered();
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
    void on_actionViewZoomAutofitSmaller_triggered();
    void on_actionViewZoomDisable_triggered();

    void on_actionViewOptions_triggered();

    void on_actionPlayPause_triggered(bool checked);
    void on_actionPlayStop_triggered();
    void on_actionPlayFrameBackward_triggered();
    void on_actionPlayFrameForward_triggered();
    void on_actionPlayRateDecrease_triggered();
    void on_actionPlayRateIncrease_triggered();
    void on_actionPlayRateReset_triggered();
    void on_actionPlaySeekForwards_triggered();
    void on_actionPlaySeekBackwards_triggered();
    void on_actionPlaySeekForwardsFine_triggered();
    void on_actionPlaySeekBackwardsFine_triggered();

    void on_actionPlayVolumeUp_triggered();
    void on_actionPlayVolumeDown_triggered();
    void on_actionPlayVolumeMute_toggled(bool checked);

    void on_actionNavigateChaptersPrevious_triggered();
    void on_actionNavigateChaptersNext_triggered();

    void on_actionHelpHomepage_triggered();
    void on_actionHelpAbout_triggered();

    void position_sliderMoved(int position);
    void on_play_clicked();
    void volume_sliderMoved(double position);

    void sendUpdateSize();

    void on_actionPlaylistPlayCurrent_triggered();

private:
    Ui::MainWindow *ui;
    QMainWindow *mpvHost_;
    MpvWidget *mpvw;
    QMediaSlider *positionSlider_;
    QVolumeSlider *volumeSlider_;
    PlaylistWindow *playlistWindow_;

    DecorationState decorationState_;
    bool fullscreenMode_;

    QString previousOpenDir;
    QSize noVideoSize_;
    bool isPlaying;
    bool isPaused;
    double sizeFactor_;
    double fitFactor_;
    ZoomMode zoomMode;
    int64_t displayDrops;
    int64_t decoderDrops;
};

#endif // HOSTWINDOW_H
