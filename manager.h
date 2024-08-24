#ifndef MANAGER_H
#define MANAGER_H
// A simple playback manager designed to control the MainWindow like it were a
// widget.  Due to needing to communicate with the playlists, it sort of takes
// control of the mpv widget away from its host.


#include <QObject>
#include <QList>
#include <QUrl>
#include <QUuid>
#include <QSize>
#include <QVariant>
#include "helpers.h"

class MpvObject;
class PlaylistWindow;

class TrackData {
public:
    static TrackData fromMap(const QVariantMap &map);
    int64_t trackId = 0;
    QString type;
    QString codec;
    QString lang;
    QString title;
    bool isExternal = false;
    bool isForced = false;
    bool isDefault = false;
    QString formatted();
};

class PlaybackManager : public QObject
{
    Q_OBJECT
public:
    enum PlaybackState { StoppedState, PausedState, PlayingState,
                         BufferingState, WaitingState };
    enum PlaybackType { None, File, Disc, Stream, Device };

    explicit PlaybackManager(QObject *parent = nullptr);
    void setMpvObject(MpvObject *mpvObject, bool makeConnections = false);
    void setPlaylistWindow(PlaylistWindow *playlistWindow);
    QUrl nowPlaying();
    PlaybackState playbackState();

signals:
    void playerSettingsRequested();
    void timeChanged(double time, double length);
    void titleChanged(QString title);
    void chapterTitleChanged(QString title);
    void videoSizeChanged(QSize size);
    void playbackSpeedChanged(double speed);
    void stateChanged(PlaybackManager::PlaybackState state);
    void typeChanged(PlaybackManager::PlaybackType type);
    // Transmit a map of chapter index to time,description pairs
    void chaptersAvailable(QList<QPair<double,QString>> chapters);
    // These signals transmit a list of (id, description) pairs
    void audioTracksAvailable(QList<QPair<int64_t,QString>> tracks);
    void videoTracksAvailable(QList<QPair<int64_t,QString>> tracks);
    void subtitleTracksAvailable(QList<QPair<int64_t,QString>> tracks);
    void hasNoVideo(bool empty);
    void hasNoAudio(bool empty);
    void hasNoSubtitles(bool empty);
    void subtitlesVisibile(bool visible);
    void nowPlayingChanged(QUrl itemUrl, QUuid listUuid, QUuid itemUuid);
    void finishedPlaying(QUuid item);
    void afterPlaybackReset();
    void instanceShouldClose();
    void systemShouldShutdown();
    void systemShouldLogOff();
    void systemShouldLock();
    void systemShouldStandby();
    void systemShouldHibernate();
    void currentTrackInfo(TrackInfo track);
    void startingPlayingFile(QUrl url);
    void stoppedPlaying();

    void fpsChanged(double fps);
    void avsyncChanged(double sync);
    void displayFramedropsChanged(int64_t count);
    void decoderFramedropsChanged(int64_t count);
    void audioBitrateChanged(double bitrate);
    void videoBitrateChanged(double bitrate);

public slots:
    // load functions
    void openSeveralFiles(QList<QUrl> what, bool important = false);
    void openFile(QUrl what, QUrl with = QUrl());

    void playDiscFiles(QUrl where);             // from dvd/bd open
    void playStream(QUrl stream);               // from menu
    void playItem(QUuid playlist, QUuid item);  // called by playlistwindow
    void playDevice(QUrl device);   // I don't have a device to test this

    void loadSubtitle(QUrl with);

    // control functions
    void playPlayer();
    void startPlayer();
    void playPausePlayer();
    void pausePlayer();
    void unpausePlayer();
    void stopPlayer();
    void stepBackward();
    void stepForward();
    void navigateToNextChapter();
    void navigateToPrevChapter();
    void playNext();
    void playPrev();
    void repeatThisFile();
    void deltaExtraPlaytimes(int delta);
    void navigateToChapter(int64_t chapter);
    void navigateToTime(double time);
    void speedUp();
    void speedDown();
    void speedReset();
    void relativeSeek(bool forwards, bool isSmall);

    // output functions
    void setPlaybackSpeed(double speed);
    void setSpeedStep(double step);
    void setSpeedStepAdditive(bool isAdditive);
    void setStepTimeLarge(int largeMsec);
    void setStepTimeSmall(int smallMsec);
    void setSubtitleTrackPreference(QString langs);
    void setAudioTrackPreference(QString langs);
    void setAudioTrack(int64_t id);
    void setAudioTrackEx(int64_t id, bool softly);
    void setSubtitleTrack(int64_t id);
    void setSubtitleTrackEx(int64_t id, bool softly);
    void setVideoTrack(int64_t id);
    void setSubtitleEnabled(bool enabled);
    void selectNextSubtitle();
    void selectPrevSubtitle();
    void setVolume(int64_t volume);
    void setMute(bool muted);
    void setAfterPlaybackOnce(Helpers::AfterPlayback mode);
    void setAfterPlaybackAlways(Helpers::AfterPlayback mode);
    void setAfterPlaybackAlwaysDefault(Helpers::AfterPlayback mode);
    void setSubtitlesPreferDefaultForced(bool forced);
    void setSubtitlesPreferExternal(bool external);
    void setSubtitlesIgnoreEmbedded(bool ignore);

    // playback options
    void setPlaybackPlayTimes(int times);
    void setPlaybackForever(bool yes);
    void setFolderAutoplay(bool yes);

    // misc functions
    void sendCurrentTrackInfo();
    void getCurrentTrackInfo(QUrl &url, QUuid &listUuid, QUuid &itemUuid, QString title, double &length, double &position);

private:
    void startPlayWithUuid(QUrl what, QUuid playlistUuid, QUuid itemUuid,
                           bool isRepeating, QUrl with = QUrl());
    void selectDesiredTracks();
    void updateSubtitleTrack();
    void checkAfterPlayback(bool playlistMode);
    void playNextTrack();
    void playPrevTrack();
    bool playNextFileUrl(QUrl url, int delta = 1);
    void playNextFile(int delta = 1);
    void playPrevFile();
    void playHalt();

private slots:
    void mpvw_playTimeChanged(double time);
    void mpvw_playLengthChanged(double length);
    void mpvw_seekableChanged(bool yes);
    void mpvw_playbackLoading();
    void mpvw_playbackStarted();
    void mpvw_pausedChanged(bool yes);
    void mpvw_playbackIdling();
    void mpvw_mediaTitleChanged(QString title);
    void mpvw_chapterDataChanged(QVariantMap metadata);
    void mpvw_chaptersChanged(QVariantList chapters);
    void mpvw_tracksChanged(QVariantList tracks);
    void mpvw_videoSizeChanged(QSize size);
    void mpvw_fpsChanged(double fps);
    void mpvw_avsyncChanged(double sync);
    void mpvw_displayFramedropsChanged(int64_t count);
    void mpvw_decoderFramedropsChanged(int64_t count);
    void mpvw_metadataChanged(QVariantMap metadata);
    void mpvw_playlistChanged(const QVariantList &playlist);
    void mpvw_audioBitrateChanged(double bitrate);
    void mpvw_videoBitrateChanged(double bitrate);

private:
    MpvObject *mpvObject_ = nullptr;
    PlaylistWindow *playlistWindow_ = nullptr;
    QUrl  nowPlaying_;
    QUuid nowPlayingList;
    QUuid nowPlayingItem;
    QString nowPlayingTitle;

    double mpvStartTime = -1.0;
    double mpvTime = 0.0;
    double mpvLength = 0.0;
    double mpvSpeed = 1.0;
    double speedStep = 2.0;
    bool speedStepAdditive = true;
    double stepTimeLarge = 5.0;
    double stepTimeSmall = 1.0;
    PlaybackState playbackState_ = StoppedState;
    PlaybackState playbackStartState = PlayingState;

    QList<QPair<int64_t,QString>> videoList;
    QList<QPair<int64_t,QString>> audioList;
    QList<QPair<int64_t,QString>> subtitleList;
    QMap<int64_t,TrackData> audioListData;
    QMap<int64_t,TrackData> subtitleListData;

    QStringList audioLangPref;
    QStringList subtitleLangPref;
    QString videoListSelected;
    QString audioListSelected;
    QString subtitleListSelected;
    int64_t subtitleTrackSelected = 1;
    bool subtitleEnabled = true;
    bool subtitlesIgnoreEmbedded = false;
    bool subtitlesPreferDefaultForced = false;
    bool subtitlesPreferExternal = false;
    int numChapters = 0;

    int playbackPlayTimes = 1;
    bool playbackStartPaused = false;
    bool playbackForever = false;
    bool folderAutoplay = false;

    Helpers::AfterPlayback afterPlaybackOnce = Helpers::DoNothingAfter;
    Helpers::AfterPlayback afterPlaybackAlways = Helpers::DoNothingAfter;
};

#endif // MANAGER_H
