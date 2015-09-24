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

class MpvWidget;
class PlaylistWindow;

class PlaybackManager : public QObject
{
    Q_OBJECT
public:
    enum PlaybackState { StoppedState, PausedState, PlayingState,
                         BufferingState };
    enum PlaybackType { None, File, Disc, Stream, Device };

    explicit PlaybackManager(QObject *parent = 0);
    void setMpvWidget(MpvWidget *mpvWidget, bool makeConnections = false);
    void setPlaylistWindow(PlaylistWindow *playlistWindow);

public slots:
    // load functions
    void openFiles(QList<QUrl> what);           // from quick-load dialog
    void playDisc(QUrl where);                  // from menu
    void playStream(QUrl stream);               // from menu
    void playItem(QUuid playlist, QUuid item);  // called by playlistwindow
    void playDevice(QUrl device);   // I don't have a device to test this

    // control functions
    void pausePlayer();
    void unpausePlayer();
    void stopPlayer();
    void stepBackward();
    void stepForward();
    void navigateToNextChapter();
    void navigateToPrevChapter();
    void navigateToChapter(int64_t chapter);
    void navigateToTime(double time);
    void speedUp();
    void speedDown();
    void speedReset();

    // output functions
    void setPlaybackSpeed(double speed);
    void setAudioTrack(int64_t id);
    void setSubtitleTrack(int64_t id);
    void setVideoTrack(int64_t id);
    void setVolume(int64_t volume);
    void setMute(bool muted);

private slots:
    void mpvw_playTimeChanged(double time);
    void mpvw_playLengthChanged(double length);
    void mpvw_playbackStarted();
    void mpvw_pausedChanged(bool yes);
    void mpvw_playbackFinished();
    void mpvw_mediaTitleChanged(QString title);
    void mpvw_chapterDataChanged(QVariantMap metadata);
    void mpvw_chaptersChanged(QVariantList chapters);
    void mpvw_tracksChanged(QVariantList tracks);
    void mpvw_videoSizeChanged(QSize size);
    void mpvw_fpsChanged(double fps);
    void mpvw_avsyncChanged(double sync);
    void mpvw_displayFramedropsChanged(int64_t count);
    void mpvw_decoderFramedropsChanged(int64_t count);

signals:
    void timeChanged(double time, double length);
    void titleChanged(QString title);
    void chapterTitleChanged(QString title);
    void videoSizeChanged(QSize size);
    void stateChanged(PlaybackState state);
    void typeChanged(PlaybackType type);
    // Transmit a map of chapter index to time,description pairs
    void chaptersAvailable(QList<QPair<double,QString>> chapters);
    // These signals transmit a list of (id, description) pairs
    void audioTracksAvailable(QList<QPair<int64_t,QString>> tracks);
    void videoTracksAvailable(QList<QPair<int64_t,QString>> tracks);
    void subtitleTracksAvailable(QList<QPair<int64_t,QString>> tracks);
    void nowPlayingChanged(QUuid item);
    void finishedPlaying(QUuid item);

    void fpsChanged(double fps);
    void avsyncChanged(double sync);
    void displayFramedropsChanged(int64_t count);
    void decoderFramedropsChanged(int64_t count);

private:
    MpvWidget *mpvWidget_;
    PlaylistWindow *playlistWindow_;
    QUuid nowPlayingList;
    QUuid nowPlayingItem;

    double mpvLength;
    double mpvSpeed;
};

#endif // MANAGER_H
