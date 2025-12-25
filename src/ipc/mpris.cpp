#include <QDBusConnection>
#include <QDBusMessage>
#include <QMetaObject>
#include <QMetaClassInfo>
#include <QRegularExpression>
#include <cmath>
#include <functional>
#include "ipc/mpris.h"

static constexpr char logModule[] =  "mpris";

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
    if (registered_)
        return;

    registered_ = dbus.registerService(dbusName_);
    if (!registered_) {
        // We make sure we're the only instance around... unless someone is
        // running a debugger on one instance and we've been started
        // afterwards.
        return;
    }
    dbus.registerObject("/org/mpris/MediaPlayer2", this);
    emit dbusRegistered(registered_);
}

void MprisInstance::unregisterDBus()
{
    if (!registered_)
        return;

    dbus.unregisterObject("/org/mpris/MediaPlayer2", QDBusConnection::UnregisterTree);
    dbus.unregisterService(dbusName_);
    registered_ = false;
    emit dbusRegistered(registered_);
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
    Q_UNUSED(listUuid)
    Q_UNUSED(itemUuid)
    player->instance_setNowPlayingUrl(itemUrl);
}

void MprisInstance::mpvObject_mediaTitleChanged(const QString &mediaTitle)
{
    player->instance_setMediaTitle(mediaTitle);
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
    mimeTypes_ = QString("application/ogg;application/x-ogg;application/mxf;application/sdp;application/smil;application/x-smil;application/streamingmedia;application/x-streamingmedia;application/vnd.rn-realmedia;application/vnd.rn-realmedia-vbr;audio/aac;audio/x-aac;audio/vnd.dolby.heaac.1;audio/vnd.dolby.heaac.2;audio/aiff;audio/x-aiff;audio/m4a;audio/x-m4a;application/x-extension-m4a;audio/mp1;audio/x-mp1;audio/mp2;audio/x-mp2;audio/mp3;audio/x-mp3;audio/mpeg;audio/mpeg2;audio/mpeg3;audio/mpegurl;audio/x-mpegurl;audio/mpg;audio/x-mpg;audio/rn-mpeg;audio/musepack;audio/x-musepack;audio/ogg;audio/scpls;audio/x-scpls;audio/vnd.rn-realaudio;audio/wav;audio/x-pn-wav;audio/x-pn-windows-pcm;audio/x-realaudio;audio/x-pn-realaudio;audio/x-ms-wma;audio/x-pls;audio/x-wav;video/mpeg;video/x-mpeg2;video/x-mpeg3;video/mp4v-es;video/x-m4v;video/mp4;application/x-extension-mp4;video/divx;video/vnd.divx;video/msvideo;video/x-msvideo;video/ogg;video/quicktime;video/vnd.rn-realvideo;video/x-ms-afs;video/x-ms-asf;audio/x-ms-asf;application/vnd.ms-asf;video/x-ms-wmv;video/x-ms-wmx;video/x-ms-wvxvideo;video/x-avi;video/avi;video/x-flic;video/fli;video/x-flc;video/flv;video/x-flv;video/x-theora;video/x-theora+ogg;video/x-matroska;video/mkv;audio/x-matroska;application/x-matroska;video/webm;audio/webm;audio/vorbis;audio/x-vorbis;audio/x-vorbis+ogg;video/x-ogm;video/x-ogm+ogg;application/x-ogm;application/x-ogm-audio;application/x-ogm-video;application/x-shorten;audio/x-shorten;audio/x-ape;audio/x-wavpack;audio/x-tta;audio/AMR;audio/ac3;audio/eac3;audio/amr-wb;video/mp2t;audio/flac;audio/mp4;application/x-mpegurl;video/vnd.mpegurl;application/vnd.apple.mpegurl;audio/x-pn-au;video/3gp;video/3gpp;video/3gpp2;audio/3gpp;audio/3gpp2;video/dv;audio/dv;audio/opus;audio/vnd.dts;audio/vnd.dts.hd;audio/x-adpcm;application/x-cue;audio/m3u;audio/vnd.wave;video/vnd.avi;inode/directory;").split(';');
}

MprisInstance *MprisServer::instance()
{
    return static_cast<MprisInstance*>(parent());
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
    return "io.github.mpc_qt.mpc-qt";
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
    return static_cast<MprisInstance*>(parent());
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
    Q_UNUSED(rate)
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
    return qlonglong(playbackTime_ * 1000000);
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
    emit instance()->playNextTrack(false);
}

void MprisPlayerServer::Previous()
{
    emit instance()->playPreviousTrack(false);
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
    Offset /= 1000000;
    if (canSeek_)
        emit instance()->relativeSeek(Offset);
}

void MprisPlayerServer::SetPosition(const QDBusObjectPath &TrackId, qlonglong Position)
{
    Q_UNUSED(TrackId)
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

void MprisPlayerServer::instance_setMediaTitle(const QString &mediaTitle)
{
    mpvMediaTitle = mediaTitle;
    if (maybeChangeMetadata())
        instance()->dbusPropertyChange(this, {{"Metadata", metadata_}});
}

void MprisPlayerServer::instance_setMetadata(const QVariantMap &metadata)
{
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
    // arbitrary bitfield used to detect any changes
    int infoLevel = (!mpvMetadata.isEmpty()     ? 1<<0 : 0)
                  + (!mpvMediaTitle.isEmpty()   ? 1<<1 : 0)
                  + (playbackDuration_ >= 0     ? 1<<2 : 0)
                  + (nowPlayingUrl_.isValid()   ? 1<<3 : 0);

    if (infoLevel == metadataInfoLevel)
        return false;
    metadataInfoLevel = infoLevel;

    metadata_.clear();
    if (infoLevel == 0)
        return true;

    if (mpvMediaTitle.isEmpty())
        mpvMediaTitle = nowPlayingUrl_.fileName();
    if (!mpvMetadata.contains("title"))
        metadata_.insert("xesam:title", mpvMediaTitle);
    else if (!mpvMetadata.contains("mediaTitle"))
        metadata_.insert("xesam:mediaTitle", mpvMediaTitle);

    metadata_.insert("mpris:trackid", "/no/text");
    metadata_.insert("mpris:length", qlonglong(playbackDuration_ * 1000000));
    metadata_.insert("xesam:url", nowPlayingUrl_.toString());

    auto trackMangle = [](QString &key, QVariant &value) -> bool {
        key = "track";
        static QRegularExpression re("\\d+");
        value = re.match(value.toString()).captured().toInt();
        return true;
    };
    auto dateMangle = [](QString &key, QVariant &value) -> bool {
        key = "date";
        bool ok;
        int i = value.toInt(&ok);
        if (ok) {
           value = QString::number(i) + "-01-01T00:00:00Z";
           return true;
        }
        QDateTime t(QDateTime::fromString(value.toString().replace('/','-'), "yyyy-MM-dd"));
        value = t.toString("yyyy-MM-ddThh:mm:ssZ");
        return t.isValid();
    };
    auto noMangle = [](QString &key, QVariant &value) -> bool {
        Q_UNUSED(key)
        Q_UNUSED(value)
        return true;
    };
    QHash<QString, std::function<bool(QString &key, QVariant &value)>> manglers {
        { "track", trackMangle },
        { "trackNumber", trackMangle },
        { "tdat", dateMangle },
        { "tyer", dateMangle },
        { "year", dateMangle },
        { "date", dateMangle },
        { "title", noMangle },
        { "album", noMangle }
    };
    auto sanitiser = [&](QString &key, QVariant &value) -> bool {
        if (manglers.contains(key))
            return manglers[key](key,value);
        else if (value.metaType().id() == QMetaType::QString)
            value = QStringList({value.toString()});
        return true;
    };

    for (auto it = mpvMetadata.constBegin(); it != mpvMetadata.constEnd(); it++) {
        QString key = it.key();
        QVariant value = it.value();
        if (sanitiser(key, value))
            metadata_.insert("xesam:" + key, value);
    }
    return true;
}
