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
#include "mpvwidget.h"

struct Track {
    int64_t id;
    QString title;
};

class MpvObject;
class PlaylistWindow;

class TrackData {
public:
    static TrackData fromMap(const QVariantMap &map);
    int64_t trackId = 0;
    QString type;
    QString codec;
    QString codecProfile;
    int width = 0;
    int height = 0;
    int samplerate = 0;
    QString channels;
    int bitrate = 0;
    QString lang;
    QString title;
    bool isExternal = false;
    bool isForced = false;
    bool isDefault = false;
    bool isImage = false;
    QString formatted(const QString &nowPlayingTitle);
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
    bool eofReached();

signals:
    void timeChanged(double time, double length);
    void playLengthChanged();
    void titleChanged(QString title);
    void titleChangedWithFilename(QString title, QString filename);
    void videoSizeChanged(QSize size);
    void playbackSpeedChanged(double speed);
    void stateChanged(PlaybackManager::PlaybackState state);
    void fileClosed();
    void typeChanged(PlaybackManager::PlaybackType type);
    // Transmit a map of chapter index to time,description pairs
    void chaptersAvailable(QList<Chapter> chapters);
    // These signals transmit a list of (id, description) pairs
    void audioTracksAvailable(QList<Track> tracks);
    void videoTracksAvailable(QList<Track> tracks);
    void subtitleTracksAvailable(QList<Track> tracks);
    void playingNextFile();
    void hasNoVideo(bool empty);
    void hasNoAudio(bool empty);
    void hasNoSubtitles(bool empty);
    void isVideo(bool isVideo);
    void subtitlesVisible(bool visible);
    void nowPlayingChanged(QUrl itemUrl, QUuid listUuid, QUuid itemUuid, bool clickedInPlaylist = false);
    void finishedPlaying(QUuid item);
    void afterPlaybackReset();
    void instanceShouldClose();
    void systemShouldShutdown();
    void systemShouldLogOff();
    void systemShouldLock();
    void systemShouldStandby();
    void systemShouldHibernate();
    void currentTrackInfo(TrackInfo track);
    void openingNewFile();
    void startingPlayingFile(QUrl url);
    void removePlaylistItemRequested(QUuid itemUuid);
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
    void playItem(QUuid playlist, QUuid item, bool clickedInPlaylist = false);  // called by playlistwindow
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
    bool playNext(bool forceFolderFallback, bool replaceMpvPlaylist = true);
    void playPrev(bool forceFolderFallback);
    void moveToRecycleBin();
    void repeatThisFile();
    void deltaExtraPlaytimes(int delta);
    void navigateToChapter(int64_t chapter);
    void navigateToTime(double time);
    void speedUp();
    void speedDown();
    void speedReset();
    void relativeSeek(bool forwards, bool isLarge);

    // output functions
    void setPlaybackSpeed(double speed);
    void setAppendToQuickPlaylist(bool isAppend);
    void setSpeedStep(double step);
    void setSpeedStepAdditive(bool isAdditive);
    void setStepTimeNormal(int normalMsec);
    void setStepTimeLarge(int largeMsec);
    void setSubtitleTrackPreference(QString langs);
    void setAudioTrackPreference(QString langs);
    void setAudioTrack(int64_t id, bool userSelected);
    void setSubtitleTrack(int64_t id, bool userSelected);
    void setVideoTrack(int64_t id, bool userSelected);
    void selectNextAudioTrack();
    void selectPrevAudioTrack();
    void setSubtitleEnabled(bool enabled, bool onInit = false);
    void selectNextSubtitle();
    void selectPrevSubtitle();
    void setVolume(int64_t volume, bool onInit = false);
    void setMute(bool muted, bool onInit);
    void setAfterPlaybackOnce(Helpers::AfterPlayback mode);
    void setAfterPlaybackAlways(Helpers::AfterPlayback mode);
    void setAfterPlaybackAlwaysDefault(Helpers::AfterPlayback mode);
    void setTimeShortMode(bool shortMode);
    void setSubtitlesPreferDefaultForced(bool forced);
    void setSubtitlesPreferExternal(bool external);
    void setSubtitlesIgnoreEmbedded(bool ignore);

    // playback options
    void setPlaybackPlayTimes(int times);
    void setPlaybackForever(bool yes);
    void setFastSeek(bool yes);
    void setFolderFallback(bool yes);

    // misc functions
    void sendCurrentTrackInfo();
    void getCurrentTrackInfo(QUrl &url, QUuid &listUuid, QUuid &itemUuid, QString &title,
                             double &length, double &position, int64_t &videoTrack,
                             int64_t &audioTrack, int64_t &subtitleTrack, bool &hasVideo);
    QString nowPlayingTitle();

private:
    enum AspectNameChanged { OnOpen, OnFirstPlay, Manually };
    void startPlayWithUuid(QUrl what, QUuid playlistUuid, QUuid itemUuid,
                           bool isRepeating, QUrl with = QUrl(),
                           bool clickedInPlaylist = false, bool replaceMpvPlaylist = true);
    void selectDesiredTracks();
    void updateSubtitleTrack();
    void updateChapters();
    void checkAfterPlayback();
    bool playNextTrack(bool replaceMpvPlaylist);
    void playPrevTrack();
    bool playNextFileUrl(QUrl url, int delta = 1, bool replaceMpvPlaylist = true);
    bool playNextFile(bool replaceMpvPlaylist = true, int delta = 1);
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
    void mpvw_playbackFinished();
    void mpvw_eofReachedChanged(QString eof);
    void mpvw_mediaTitleChanged(QString title);
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
    void mpvw_aspectNameChanged(QString newAspectName);
    void mpvw_hwdecCurrentChanged(QString newHwdecCurrent);

private:
    MpvObject *mpvObject_ = nullptr;
    PlaylistWindow *playlistWindow_ = nullptr;
    QUrl  nowPlaying_;
    QUuid nowPlayingList;
    QUuid nowPlayingItem;
    QString nowPlayingTitle_;
    int64_t currentMpvPlaylistItemId = 0;

    double mpvStartTime = -1.0;
    double mpvTime = 0.0;
    double mpvLength = 0.0;
    double mpvSpeed = 1.0;
    double speedStep = 1.25;
    bool speedStepAdditive = true;
    bool eofReached_ = false;
    bool fastHardwareDecoding = false;
    double stepTimeNormal = 5.0;
    double stepTimeLarge = 20.0;
    PlaybackState playbackState_ = StoppedState;
    PlaybackState playbackStartState = PlayingState;

    QList<Track> videoList;
    QList<Track> audioList;
    QList<Track> subtitleList;
    QMap<int64_t,TrackData> videoListData;
    QMap<int64_t,TrackData> audioListData;
    QMap<int64_t,TrackData> subtitleListData;
    QVariantList chapters;

    QStringList audioLangPref;
    QStringList subtitleLangPref;
    int64_t videoTrackSelected = -1;
    int64_t audioTrackSelected = -1;
    int64_t subtitleTrackSelected = -1;
    int64_t subtitleTrackActive = 0;
    bool subtitleEnabled = true;
    bool subtitlesIgnoreEmbedded = false;
    bool subtitlesPreferDefaultForced = false;
    bool subtitlesPreferExternal = false;
    int numChapters = 0;

    int playbackPlayTimes = 1;
    bool playbackStartPaused = false;
    bool playbackForever = false;
    bool fastSeek = true;
    bool folderFallback = false;
    bool appendToQuickPlaylist = false;

    bool timeShortMode = false;

    PlaybackManager::AspectNameChanged showAspectOsdTriggeredBy = AspectNameChanged::OnOpen;

    Helpers::AfterPlayback afterPlaybackOnce = Helpers::DoNothingAfter;
    Helpers::AfterPlayback afterPlaybackAlways = Helpers::DoNothingAfter;
};

#endif // MANAGER_H
