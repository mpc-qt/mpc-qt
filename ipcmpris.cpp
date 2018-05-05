#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QMetaObject>
#include <QMetaClassInfo>
#include <cmath>
#include "ipcmpris.h"
#include "mpvwidget.h"

class PlayerFlow;

int MprisInstance::dbusCount = 0;

MprisInstance::MprisInstance(QObject *parent) : QObject(parent),
  dbus(makeDBusConnection())
{
    dbusName_ = "org.mpris.MediaPlayer2.MpcQt";
    dbusId_ = dbusCount++;
    if (dbusId_ != 0)
        dbusName_.append(".instance" + QString::number(dbusId_));

    server = new MprisServer(this);
    player = new MprisPlayerServer(this);
}

void MprisInstance::registerDBus()
{
    registered_ = dbus.registerService(dbusName_);
    if (!registered_) {
        // We make sure we're the only instance around... unless someone is
        // running a debugger on one instance and we've been started
        // afterwards.
        return;
    }
    dbus.registerObject("/org/mpris/MediaPlayer2", this);
}

void MprisInstance::unregisterDBus()
{
    if (registered_) {
        dbus.unregisterObject("/org/mpris/MediaPlayer2", QDBusConnection::UnregisterTree);
        dbus.unregisterService(dbusName_);
    }
    registered_ = false;
}

void MprisInstance::setProtocolList(const QStringList &protocolList)
{
    server->instance_setProtocolList(protocolList);
}

int MprisInstance::dbusId()
{
    return dbusId_;
}

QString MprisInstance::dbusName()
{
    return dbusName_;
}

bool MprisInstance::registered()
{
    return registered_;
}

QDBusConnection MprisInstance::makeDBusConnection()
{
    // make unique connection so property changes don't conflict???
    return QDBusConnection::connectToBus(QDBusConnection::SessionBus,
                                         QString("_mpc_qt_%1").arg(QString::number(dbusCount)));
}

void MprisInstance::dbusPropertyChange(QDBusAbstractAdaptor *who, QVariantMap propertyMap)
{
    if (!registered_)
        return;

    QDBusMessage msg = QDBusMessage::createSignal("/org/mpris/MediaPlayer2",
                                                  "org.freedesktop.DBus.Properties",
                                                  "PropertiesChanged");
    msg << who->metaObject()->classInfo(0).value();
    msg << propertyMap;
    msg << QStringList();
    dbus.send(msg);
}

void MprisInstance::mainwindow_fullscreenModeChanged(bool yes)
{
    server->instance_setFullscreen(yes);
}

void MprisInstance::mainwindow_volumeChanged(int level)
{
    player->instance_setVolume(level/100.0);
}

void MprisInstance::manager_timeChanged(double time, double length)
{
    player->instance_timeChange(time, length);
}

void MprisInstance::manager_stateChanged(PlaybackManager::PlaybackState state)
{
    player->instance_setPlaybackState(state);
}

void MprisInstance::manager_nowPlayingChanged(QUrl itemUrl, QUuid listUuid, QUuid itemUuid)
{
    Q_UNUSED(listUuid);
    Q_UNUSED(itemUuid);
    player->instance_setNowPlayingUrl(itemUrl);
}

void MprisInstance::mpvObject_metaDataChanged(const QVariantMap &metadata)
{
    player->instance_setMetadata(metadata);
}

void MprisInstance::playlistwindow_currentPlaylistHasItems(bool yes)
{
    player->instance_setPlaylistHasItems(yes);
}



MprisServer::MprisServer(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    mimeTypes_ = QString("application/ogg;application/x-ogg;application/sdp;application/smil;application/x-smil;application/streamingmedia;application/x-streamingmedia;application/vnd.rn-realmedia;application/vnd.rn-realmedia-vbr;audio/aac;audio/x-aac;audio/m4a;audio/x-m4a;audio/mp1;audio/x-mp1;audio/mp2;audio/x-mp2;audio/mp3;audio/x-mp3;audio/mpeg;audio/x-mpeg;audio/mpegurl;audio/x-mpegurl;audio/mpg;audio/x-mpg;audio/rn-mpeg;audio/ogg;audio/scpls;audio/x-scpls;audio/vnd.rn-realaudio;audio/wav;audio/x-pn-windows-pcm;audio/x-realaudio;audio/x-pn-realaudio;audio/x-ms-wma;audio/x-pls;audio/x-wav;video/mpeg;video/x-mpeg;video/x-mpeg2;video/mp4;video/msvideo;video/x-msvideo;video/ogg;video/quicktime;video/vnd.rn-realvideo;video/x-ms-afs;video/x-ms-asf;video/x-ms-wmv;video/x-ms-wmx;video/x-ms-wvxvideo;video/x-avi;video/x-fli;video/x-flv;video/x-theora;video/x-matroska;video/webm;audio/x-flac;audio/x-vorbis+ogg;video/x-ogm+ogg;audio/x-shorten;audio/x-ape;audio/x-wavpack;audio/x-tta;audio/AMR;audio/ac3;video/mp2t;audio/flac;audio/mp4;").split(';');
}

MprisInstance *MprisServer::instance()
{
    return reinterpret_cast<MprisInstance*>(parent());
}

bool MprisServer::fullscreen()
{
    return fullscreen_;
}

void MprisServer::setFullscreen(bool yes)
{
    emit instance()->fullscreenMode(yes);
}

QString MprisServer::identity()
{
    return "Media Player Classic Qute Theater";
}

QString MprisServer::desktopEntry()
{
    return "mpc-qt";
}

QStringList MprisServer::uriSchemes()
{
    return protocolList_;
}

QStringList MprisServer::mimeTypes()
{
    return mimeTypes_;
}

void MprisServer::Raise()
{
    emit instance()->raiseWindow();
}

void MprisServer::Quit()
{
    emit instance()->closeInstance();
}

void MprisServer::instance_setFullscreen(bool yes)
{
    fullscreen_ = yes;
    QVariantMap change {
        { "Fullscreen", yes }
    };
    instance()->dbusPropertyChange(this, change);
}

void MprisServer::instance_setProtocolList(const QStringList &protocolList)
{
    protocolList_ = protocolList;
}



MprisPlayerServer::MprisPlayerServer(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{

}

MprisInstance *MprisPlayerServer::instance()
{
    return reinterpret_cast<MprisInstance*>(parent());
}

QString MprisPlayerServer::playbackStatus()
{
    if (playbackState == PlaybackManager::PausedState)
        return "Paused";
    if (playbackState != PlaybackManager::StoppedState)
        return "Playing";
    return "Stopped";
}

double MprisPlayerServer::playbackRate()
{
    return 1.0;
}

void MprisPlayerServer::setPlaybackRate(double rate)
{
    Q_UNUSED(rate);
}

QVariantMap MprisPlayerServer::metadata()
{
    return metadata_;
}

double MprisPlayerServer::volume()
{
    return volume_;
}

void MprisPlayerServer::setVolume(double volume)
{
    emit instance()->volumeChange(volume);
}

qlonglong MprisPlayerServer::position()
{
    return qlonglong(playbackTime_ * 1000);
}

double MprisPlayerServer::minimumRate()
{
    return 1.0;
}

double MprisPlayerServer::maximumRate()
{
    return 1.0;
}

bool MprisPlayerServer::canGoNext()
{
    // return instance()->currentPlaylistHasItemsAfter();
    return true;
}

bool MprisPlayerServer::canGoPrevious()
{
    // return instance()->currentPlaylistHasItemsBefore();
    return true;
}

bool MprisPlayerServer::canPlay()
{
    return canPlay_;
}

bool MprisPlayerServer::canPause()
{
    return canPause_;
}

bool MprisPlayerServer::canSeek()
{
    return canSeek_;
}

void MprisPlayerServer::Next()
{
    emit instance()->playNextTrack();
}

void MprisPlayerServer::Previous()
{
    emit instance()->playPreviousTrack();
}

void MprisPlayerServer::Pause()
{
    if (canPause_)
        emit instance()->pause();
}

void MprisPlayerServer::PlayPause()
{
    if (canPause_)
        emit instance()->playpause();
}

void MprisPlayerServer::Stop()
{
    emit instance()->stop();
}

void MprisPlayerServer::Play()
{
    if (canPlay_)
        emit instance()->play();
}

void MprisPlayerServer::Seek(qlonglong Offset)
{
    Offset /= 1000000.0;
    if (canSeek_)
        emit instance()->relativeSeek(Offset);
}

void MprisPlayerServer::SetPosition(const QDBusObjectPath &TrackId, qlonglong Position)
{
    Q_UNUSED(TrackId);
    Position /= 1000000;
    if (canSeek_ && Position >=0 && Position <= playbackDuration_)
        emit instance()->absoluteSeek(Position);
}

void MprisPlayerServer::OpenUri(QString uri)
{
    emit instance()->openUrl(uri);
}

void MprisPlayerServer::instance_setNowPlayingUrl(const QUrl &nowPlayingUrl)
{
    if (nowPlayingUrl_ != nowPlayingUrl) {
        nowPlayingUrl_ = nowPlayingUrl;
        if (maybeChangeMetadata())
            instance()->dbusPropertyChange(this, {{"Metadata", metadata_}});
    }
}

void MprisPlayerServer::instance_setPlaybackState(PlaybackManager::PlaybackState state)
{
    if (playbackState == state)
        return;
    playbackState = state;
    QVariantMap propertyMap;
    propertyMap.insert("PlaybackStatus", playbackStatus());

    if (maybeChangeCanPlay())
        propertyMap.insert("CanPlay", canPlay_);

    bool canPause = state == PlaybackManager::PlayingState || PlaybackManager::PausedState;
    if (canPause_ != canPause) {
        canPause_ = canPause;
        canSeek_ = canPause; //FIXME: not every stream is seekable
        propertyMap.insert("CanPause", canPause_);
        propertyMap.insert("CanSeek", canSeek_);
    }
    instance()->dbusPropertyChange(this, propertyMap);
}

void MprisPlayerServer::instance_setMetadata(const QVariantMap &metadata)
{
    QVariantMap data;
    mpvMetadata = metadata;
    if (maybeChangeMetadata())
        instance()->dbusPropertyChange(this, {{"Metadata", metadata_}});
}

void MprisPlayerServer::instance_setVolume(double volume)
{
    if (volume_ != volume) {
        volume_ = volume;
        instance()->dbusPropertyChange(this, {{"Volume", volume_}});
    }
}

void MprisPlayerServer::instance_timeChange(double time, double length)
{
    QVariantMap propertyMap;
    time = std::trunc(time);
    length = std::trunc(length);
    if (playbackTime_ != time) {
        playbackTime_ = time;
        propertyMap.insert("Position", qlonglong(time * 1000000));
    }
    if (playbackDuration_ != length) {
        playbackDuration_ = length;
        if (maybeChangeMetadata())
            propertyMap.insert("Metadata", metadata_);
    }
    if (!propertyMap.isEmpty()) {
        instance()->dbusPropertyChange(this, propertyMap);
    }
}

void MprisPlayerServer::instance_setPlaylistHasItems(bool playlistHasItems)
{
    playlistHasItems_ = playlistHasItems;
    if (maybeChangeCanPlay())
        instance()->dbusPropertyChange(this, {{"CanPlay", canPlay_}});
}

bool MprisPlayerServer::maybeChangeCanPlay()
{
    bool canPlay = playlistHasItems_ || playbackState != PlaybackManager::StoppedState;
    if (canPlay_ != canPlay) {
        canPlay_ = canPlay;
        return true;
    }
    return false;
}

bool MprisPlayerServer::maybeChangeMetadata()
{
    bool haveInfo = !(mpvMetadata.isEmpty() || playbackDuration_ < 0 || !nowPlayingUrl_.isValid());
    bool hadData = !metadata_.isEmpty();
    bool changed = (haveInfo && !hadData) || (!haveInfo && hadData);
    if (!changed)
        return false;

    metadata_.clear();
    if (!haveInfo)
        return true;

    metadata_.insert("mpris:trackid", "/no/text"); //FIXME
    metadata_.insert("mpris:length", qlonglong(playbackDuration_ * 1000000));
    metadata_.insert("xesam:url", nowPlayingUrl_.toString());

    for (auto it = mpvMetadata.constBegin(); it != mpvMetadata.constEnd(); it++)
        metadata_.insert("xesam:" + it.key(), it.value());
    return true;
}
