#ifndef IPCMPRIS_H
#define IPCMPRIS_H

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QObject>
#include <QVariantMap>
#include "manager.h"

class MpvWidget;
class MprisServer;
class MprisPlayerServer;

class MprisInstance : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int dbusId READ dbusId)
    Q_PROPERTY(QString dbusName READ dbusName)
    Q_PROPERTY(bool registered READ registered)

public:
    // FIXME: don't require passing MpvWidget in this constructor
    explicit MprisInstance(QObject *parent = nullptr);
    void registerDBus();
    void unregisterDBus();

    int dbusId();
    QString dbusName();
    bool registered();

    void setProtocolList(const QStringList &protocolList);

signals:
    void fullscreenMode(bool yes);
    void raiseWindow();
    void closeInstance();
    void volumeChange(double volume);
    void playNextTrack();
    void playPreviousTrack();
    void pause();
    void playpause();
    void stop();
    void play();
    void relativeSeek(double amount);
    void absoluteSeek(double time);
    void openUrl(QUrl url);

public slots:
    void mainwindow_fullscreenModeChanged(bool yes);
    void manager_timeChanged(double time, double length);
    void manager_stateChanged(PlaybackManager::PlaybackState state);
    void manager_nowPlayingChanged(QUrl itemUrl, QUuid listUuid, QUuid itemUuid);
    void mpvwidget_metaDataChanged(const QVariantMap &metadata);
    void playlistwindow_currentPlaylistHasItems(bool yes);

private:
    static QDBusConnection makeDBusConnection();
    void dbusPropertyChange(QDBusAbstractAdaptor *who,
                            QVariantMap propertyMap);

    static int dbusCount;

    int dbusId_ = 0;
    QString dbusName_;
    bool registered_ = false;
    QDBusConnection dbus;

    MprisServer *server = nullptr;
    MprisPlayerServer *player = nullptr;

    friend MprisServer;
    friend MprisPlayerServer;
};

class MprisServer : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")
    Q_PROPERTY(bool CanQuit READ yes)
    Q_PROPERTY(bool Fullscreen READ fullscreen WRITE setFullscreen)
    Q_PROPERTY(bool CanSetFullscreen READ yes)
    Q_PROPERTY(bool CanRaise READ yes)
    Q_PROPERTY(bool HasTrackList READ no)
    Q_PROPERTY(QString Identity READ identity)
    Q_PROPERTY(QString DesktopEntry READ desktopEntry)
    Q_PROPERTY(QStringList SupportedUriSchemes READ uriSchemes)
    Q_PROPERTY(QStringList SupportedMimeTypes READ mimeTypes)

public:
    explicit MprisServer(QObject *parent = nullptr);
    MprisInstance *instance();
    bool yes() { return true; }
    bool no() { return false; }
    bool fullscreen();
    void setFullscreen(bool yes);
    QString identity();
    QString desktopEntry();
    QStringList uriSchemes();
    QStringList mimeTypes();

public slots:
    void Raise();
    void Quit();

private:
    void instance_setFullscreen(bool yes);
    void instance_setProtocolList(const QStringList &protocolList);

    bool fullscreen_;
    QStringList protocolList_;
    QStringList mimeTypes_;

    friend MprisInstance;
};

class MprisPlayerServer : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
    Q_PROPERTY(QString PlaybackStatus READ playbackStatus)
    //Q_PROPERTY(QString LoopStatus READ loopStatus WRITE setLoopStatus)
    Q_PROPERTY(double Rate READ playbackRate WRITE setPlaybackRate)
    //Q_PROPERTY(bool Shuffle READ shuffle WRITE setShuffle)
    Q_PROPERTY(QVariantMap Metadata READ metadata)
    Q_PROPERTY(double Volume READ volume WRITE setVolume)
    Q_PROPERTY(qlonglong Position READ position)
    Q_PROPERTY(double MinimumRate READ minimumRate)
    Q_PROPERTY(double MaximumRate READ maximumRate)
    Q_PROPERTY(bool CanGoNext READ canGoNext)
    Q_PROPERTY(bool CanGoPrevious READ canGoPrevious)
    Q_PROPERTY(bool CanPlay READ canPlay)
    Q_PROPERTY(bool CanPause READ canPause)
    Q_PROPERTY(bool CanSeek READ canSeek)
    Q_PROPERTY(bool CanControl READ yes)

signals:
    void Seeked(qlonglong time);

public:
    MprisPlayerServer(QObject *parent = nullptr);
    MprisInstance *instance();
    QString playbackStatus();
    double playbackRate();
    void setPlaybackRate(double rate);
    QVariantMap metadata();
    double volume();
    void setVolume(double volume);
    qlonglong position();
    double minimumRate();
    double maximumRate();
    bool canGoNext();
    bool canGoPrevious();
    bool canPlay();
    bool canPause();
    bool canSeek();
    bool yes() { return true; }

public slots:
    void Next();
    void Previous();
    void Pause();
    void PlayPause();
    void Stop();
    void Play();
    void Seek(qlonglong Offset);
    void SetPosition(const QDBusObjectPath& TrackId, qlonglong Position);
    void OpenUri(QString uri);

private slots:
    void instance_setNowPlayingUrl(const QUrl &nowPlayingUrl);
    void instance_setPlaybackState(PlaybackManager::PlaybackState state);
    void instance_setMetadata(const QVariantMap &metadata);
    void instance_setVolume(double volume);
    void instance_timeChange(double time, double length);
    void instance_setPlaylistHasItems(bool playlistHasItems);

private:
    bool maybeChangeCanPlay();

    PlaybackManager::PlaybackState playbackState = PlaybackManager::StoppedState;
    QVariantMap metadata_;
    double volume_ = 100;
    double playbackTime_ = -1;
    double playbackDuration_ = -1;
    bool canPlay_ = false;
    bool canPause_ = false;
    bool canSeek_ = false;
    bool playlistHasItems_ = false;
    QUrl nowPlayingUrl_;

    friend MprisInstance;
};

#endif // IPCMPRIS_H
