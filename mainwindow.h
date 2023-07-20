#ifndef HOSTWINDOW_H
#define HOSTWINDOW_H

#include <QMainWindow>
#include <mpvwidget.h>
#include <QMenuBar>
#include <QTimer>
#include "helpers.h"
#include "widgets/drawnslider.h"
#include "widgets/drawnstatus.h"
#include "manager.h"
#include "playlistwindow.h"
#include "platform/screensaver.h"
#include "platform/windowmanager.h"

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
    enum OnTopMode { OnTopDefault, AlwaysOnTop, OnTopWhenPlaying,
                     OnTopForVideos };

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    MpvObject *mpvObject();
    QMainWindow *mpvHost();
    PlaylistWindow *playlistWindow();
    QList<QAction *> editableActions();
    QVariantMap mouseMapDefaults();
    QMap<int, QAction *> wmCommandMap();
    QVariantMap state();
    void setState(const QVariantMap &map);
    void setScreensaverAbilities(QSet<ScreenSaver::Ability> ab);
    QSize desirableSize(bool first_run = false);
    QPoint desirablePosition(QSize &size, bool first_run = false);
    void unfreezeWindow();

protected:
    void resizeEvent(QResizeEvent *event);
    bool eventFilter(QObject *object, QEvent *event);
    void closeEvent(QCloseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

private:
    bool mouseStateEvent(const MouseState &state);

    MediaSlider *positionSlider();
    VolumeSlider *volumeSlider();

    DecorationState decorationState();
    bool fullscreenMode();
    QSize noVideoSize();
    double sizeFactor();

    void setDiscState(bool playingADisc);

    void setupMenu();
    void setupContextMenu();
    void setupActionGroups();
    void setupPositionSlider();
    void setupVolumeSlider();
    void setupMpvHost();
    void setupMpvObject();
    void setupMpvWidget(Helpers::MpvWidgetType widgetType);
    void setupPlaylist();
    void setupStatus();
    void setupSizing();
    void setupBottomArea();
    void setupIconThemer();
    void setupHideTimer();
    void connectActionsToSignals();
    void connectActionsToSlots();
    void connectButtonsToActions();
    void connectPlaylistWindowToActions();
    void globalizeAllActions();
    void setUiDecorationState(DecorationState state);
    void setOSDPage(int page);
    void setUiEnabledState(bool enabled);
    void reparentBottomArea(bool overlay);
    void checkBottomArea(QPointF mousePosition);
    void leaveBottomArea();
    void updateBottomAreaGeometry();
    void updateTime();
    void updateFramedrops();
    void updateBitrate();
    void updatePlaybackStatus();
    void updateSize(bool first_run = false);
    void updateInfostats();
    void updateOnTop();
    void updateWindowFlags();
    void updateMouseHideTime();
    void updateDiscList();
    QList<QUrl> doQuickOpenFileDialog();

signals:
    void instanceShouldQuit();
    void fileOpened(QUrl what, QUrl subs);
    void severalFilesOpened(QList<QUrl> what, bool important = false);
    void severalFilesOpenedForPlaylist(QUuid destination, QList<QUrl> what);
    void dvdbdOpened(QUrl what);
    void streamOpened(QUrl what);
    void recentOpened(TrackInfo info);
    void recentClear();
    void takeImage(Helpers::ScreenshotRender render);
    void takeImageAutomatically(Helpers::ScreenshotRender render);
    void takeThumbnails();
    void subtitlesLoaded(QUrl subs);
    void showFileProperties();
    void showLogWindow();
    void hideLogWindow();
    void showLibraryWindow();
    void hideLibraryWindow();
    void optionsOpenRequested();
    void paused();
    void unpaused();
    void stopped();
    void stepBackward();
    void stepForward();
    void speedDown();
    void speedUp();
    void speedReset();
    void relativeSeek(bool forwards, bool isSmall);
    void audioTrackSelected(int64_t id);
    void subtitleTrackSelected(int64_t id);
    void videoTrackSelected(int64_t id);
    void subtitlesEnabled(bool enabled);
    void nextSubtitleSelected();
    void previousSubtitleSelected();
    void volumeChanged(int64_t volume);
    void volumeMuteChanged(bool muted);
    void afterPlaybackOnce(Helpers::AfterPlayback action);
    void afterPlaybackAlways(Helpers::AfterPlayback action);
    void chapterPrevious();
    void chapterNext();
    void chapterSelected(int64_t id);
    void timeSelected(double time);
    void fullscreenModeChanged(bool fullscreen);
    void zoomPresetChanged(int which);
    void playCurrentItemRequested();
    void favoriteCurrentTrack();
    void organizeFavorites();

public slots:
    void httpQuickOpenFile();
    void httpOpenFileUrl();
    void httpSaveImage();
    void httpSaveImageAuto();
    void httpSaveThumbnails();
    void httpClose();
    void httpProperties();
    void httpExit();
    void httpPlay();
    void httpPause();
    void httpStop();
    void httpFrameStep();
    void httpFrameStepBack();
    void httpIncreaseRate();
    void httpDecreaseRate();
    void httpQuickAddFavorite();
    void httpOrganizeFavories();
    void httpToggleCaptionMenu();
    void httpToggleSeekBar();
    void httpToogleControls();
    void httpToggleInformation();
    void httpToggleStatistics();
    void httpToggleStatus();
    void httpTogglePlaylistBar();
    void httpViewMinimal();
    void httpViewCompact();
    void httpViewNormal();
    void httpFullscreen();
    void httpZoom25();
    void httpZoom50();
    void httpZoom100();
    void httpZoom200();
    void httpZoomAutoFit();
    void httpZoomAutoFitLarger();
    void httpVolumeUp();
    void httpVolumeDown();
    void httpVolumeMute();
    void httpNextSubtitle();
    void httpPrevSubtitle();
    void httpOnOffSubtitles();
    void httpAfterPlaybackNothing();
    void httpAfterPlaybackPlayNext();
    void httpAfterPlaybackExit();
    void httpAfterPlaybackStandBy();
    void httpAfterPlaybackHibernate();
    void httpAfterPlaybackShutdown();
    void httpAfterPlaybackLogOff();
    void httpAfterPlaybackLock();

    void setFreestanding(bool freestanding);
    void setFullscreenMode(bool fullscreenMode);
    void setNoVideoSize(const QSize &sz);
    void setWindowedMouseMap(const MouseStateMap &map);
    void setFullscreenMouseMap(const MouseStateMap &map);
    void setRecentDocuments(QList<TrackInfo> tracks);
    void setFavoriteTracks(QList<TrackInfo> files, QList<TrackInfo> streams);
    void setIconTheme(IconThemer::FolderMode mode, QString fallback, QString custom);
    void setHighContrastWidgets(bool yes);
    void setInfoColors(const QColor &foreground, const QColor &background);
    void setTime(double time, double length);
    void setMediaTitle(QString title);
    void setChapterTitle(QString title);
    void setVideoSize(QSize size);
    void setVolumeStep(int stepSize);
    void setSizeFactor(double factor);
    void setFitFactor(double fitFactor);
    void setZoomMode(MainWindow::ZoomMode mode);
    void setZoomPreset(int which, double fitFactor = -1.0);
    void setZoomCenter(bool yes);
    void setFullscreenName(QString screenName);
    void setMouseHideTimeFullscreen(int msec);
    void setMouseHideTimeWindowed(int msec);
    void setBottomAreaBehavior(Helpers::ControlHiding method);
    void setBottomAreaHideTime(int milliseconds);
    void setTimeTooltip(bool show, bool above);
    void setFullscreenHidePanels(bool hidden);
    void setPlaybackState(PlaybackManager::PlaybackState state);
    void setPlaybackType(PlaybackManager::PlaybackType type);
    void setChapters(QList<QPair<double,QString>> chapters);
    void setAudioTracks(QList<QPair<int64_t,QString>> tracks);
    void setVideoTracks(QList<QPair<int64_t,QString>> tracks);
    void setSubtitleTracks(QList<QPair<int64_t,QString>> tracks);
    void setSubtitleText(QString subText);
    void setVolume(int level);
    void setVolumeDouble(double level);
    void setVolumeMax(int level);
    void setTimeShortMode(bool shortened);
    void resetPlayAfterOnce();
    void setPlayAfterAlways(Helpers::AfterPlayback action);
    void setFps(double fps);
    void setAvsync(double sync);
    void setDisplayFramedrops(int64_t count);
    void setDecoderFramedrops(int64_t count);
    void setPlaylistVisibleState(bool yes);
    void setPlaylistQuickQueueMode(bool yes);
    void setAudioBitrate(double bitrate);
    void setVideoBitrate(double bitrate);
    void logWindowClosed();
    void libraryWindowClosed();

private slots:
    void on_actionFileOpenQuick_triggered();
    void on_actionFileOpen_triggered();
    void on_actionFileOpenDvdbd_triggered();
    void on_actionFileOpenDirectory_triggered();
    void on_actionFileOpenNetworkStream_triggered();
    void on_actionFileRecentClear_triggered();
    void on_actionFileSaveImage_triggered();
    void on_actionFileSaveImageAuto_triggered();
    void on_actionFileSavePlainImage_triggered();
    void on_actionFileSavePlainImageAuto_triggered();
    void on_actionFileSaveWindowImage_triggered();
    void on_actionFileSaveWindowImageAuto_triggered();
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
    void on_actionViewHideLog_toggled(bool checked);
    void on_actionViewHideLibrary_toggled(bool checked);

    void on_actionViewOSDNone_triggered();
    void on_actionViewOSDMessages_triggered();
    void on_actionViewOSDStatistics_triggered();
    void on_actionViewOSDFrameTimings_triggered();
    void on_actionViewOSDCycle_triggered();

    void on_actionViewPresetsMinimal_triggered();
    void on_actionViewPresetsCompact_triggered();
    void on_actionViewPresetsNormal_triggered();

    void on_actionViewFullscreen_toggled(bool checked);
    void on_actionViewFullscreenEscape_triggered();

    void on_actionViewZoom025_triggered();
    void on_actionViewZoom050_triggered();
    void on_actionViewZoom075_triggered();
    void on_actionViewZoom100_triggered();
    void on_actionViewZoom150_triggered();
    void on_actionViewZoom200_triggered();
    void on_actionViewZoom300_triggered();
    void on_actionViewZoom400_triggered();
    void on_actionViewZoomAutofit_triggered();
    void on_actionViewZoomAutofitLarger_triggered();
    void on_actionViewZoomAutofitSmaller_triggered();
    void on_actionViewZoomDisable_triggered();

    void on_actionViewOntopDefault_toggled(bool checked);
    void on_actionViewOntopAlways_toggled(bool checked);
    void on_actionViewOntopPlaying_toggled(bool checked);
    void on_actionViewOntopVideo_toggled(bool checked);

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

    void on_actionPlaySubtitlesEnabled_triggered(bool checked);
    void on_actionPlaySubtitlesNext_triggered();
    void on_actionPlaySubtitlesPrevious_triggered();
    void on_actionPlaySubtitlesCopy_triggered();

    void on_actionPlayLoopStart_triggered();
    void on_actionPlayLoopEnd_triggered();
    void on_actionPlayLoopUse_triggered(bool checked);
    void on_actionPlayLoopClear_triggered();

    void on_actionPlayVolumeUp_triggered();
    void on_actionPlayVolumeDown_triggered();
    void on_actionPlayVolumeMute_toggled(bool checked);

    void on_actionPlayAfterOnceExit_triggered();
    void on_actionPlayAfterOnceStandby_triggered();
    void on_actionPlayAfterOnceHibernate_triggered();
    void on_actionPlayAfterOnceShutdown_triggered();
    void on_actionPlayAfterOnceLogoff_triggered();
    void on_actionPlayAfterOnceLock_triggered();
    void on_actionPlayAfterOnceNothing_triggered();
    void on_actionPlayAfterOnceRepeat_triggered();
    void on_actionPlayAfterAlwaysRepeat_triggered();
    void on_actionPlayAfterAlwaysExit_triggered();
    void on_actionPlayAfterAlwaysNothing_triggered();
    void on_actionPlayAfterAlwaysNext_triggered();

    void on_actionNavigateChaptersPrevious_triggered();
    void on_actionNavigateChaptersNext_triggered();

    void on_actionHelpHomepage_triggered();
    void on_actionHelpAbout_triggered();

    void on_actionPlaylistSearch_triggered();

    void mpvw_customContextMenuRequested(const QPoint &pos);
    void position_sliderMoved(int position);
    void position_hoverValue(double value, QString text, double x);
    void on_play_clicked();
    void volume_sliderMoved(double position);
    void playlistWindow_windowDocked();
    void playlistWindow_playlistAddItem(const QUuid &playlistUuid);
    void hideTimer_timeout();

    void on_actionFileLoadSubtitle_triggered();

    void on_actionFavoritesAdd_triggered();

    void on_actionFavoritesOrganize_triggered();

private:
    Ui::MainWindow *ui = nullptr;
    QMainWindow *mpvHost_ = nullptr;
    MpvObject *mpvObject_ = nullptr;
    QWidget *mpvw = nullptr;
    //MpvGlCbWidget *mpvw = nullptr;
    MediaSlider *positionSlider_ = nullptr;
    VolumeSlider *volumeSlider_ = nullptr;
    StatusTime *timePosition = nullptr;
    StatusTime *timeDuration = nullptr;
    PlaylistWindow *playlistWindow_ = nullptr;
    QMenu *contextMenu = nullptr;
    QTimer hideTimer;

    bool freestanding_ = false;
    DecorationState decorationState_ = AllDecorations;
    QString fullscreenName;
    FullscreenMemory fullscreenMemory;
    bool fullscreenMaximized = false;
    bool fullscreenMode_ = false;
    bool fullscreenHidePanels = true;
    Helpers::ControlHiding bottomAreaBehavior = Helpers::ShowWhenHovering;
    int bottomAreaHeight = 0;
    int bottomAreaHideTime = 0;
    bool timeTooltipShown = true;
    bool timeTooltipAbove = true;

    QString previousOpenDir;
    QSize noVideoSize_ = QSize(500,270);
    bool isPlaying = false;
    bool isPaused = false;
    bool hasVideo = false;
    bool hasAudio = false;
    bool hasSubs = false;
    QString subtitleText;
    int volumeStep = 10;
    bool frozenWindow = true;
    double sizeFactor_ = 1;
    double fitFactor_ = 0.75;
    ZoomMode zoomMode = RegularZoom;
    bool zoomCenter = true;
    OnTopMode onTopMode = OnTopDefault;
    bool showOnTop = false;
    int mouseHideTimeWindowed = 1000;
    int mouseHideTimeFullscreen = 1000;
    int64_t displayDrops = 0;
    int64_t decoderDrops = 0;
    double audioBitrate = 0;
    double videoBitrate = 0;

    IconThemer themer;
    QList<QAction *> menuFavoritesTail;
    MouseStateMap mouseMapWindowed;
    MouseStateMap mouseMapFullscreen;
};

#endif // HOSTWINDOW_H
