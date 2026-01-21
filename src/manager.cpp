#include <QRegularExpression>
#include <QFile>
#include <QFileInfo>
#include "manager.h"
#include "playlistwindow.h"
#include "logger.h"

using namespace Helpers;
static constexpr char logModule[] =  "manager";
static constexpr char strTrue[] =  "true";
static constexpr char strFalse[] =  "false";

Q_GLOBAL_STATIC_WITH_ARGS(QRegularExpression, wordSplitter, ("\\W+"));


TrackData TrackData::fromMap(const QVariantMap &map)
{
    TrackData td;
    td.trackId = map["id"].toLongLong();
    if (map.contains("type"))
        td.type = map["type"].toString();
    if (map.contains("codec"))
        td.codec = map["codec"].toString();
    if (map.contains("codec-profile"))
        td.codecProfile = map["codec-profile"].toString();
    if (map.contains("demux-w"))
        td.width = map["demux-w"].toInt();
    if (map.contains("demux-h"))
        td.height = map["demux-h"].toInt();
    if (map.contains("demux-samplerate"))
        td.samplerate = map["demux-samplerate"].toInt();
    if (map.contains("demux-channels"))
        td.channels = map["demux-channels"].toString();
    if (map.contains("demux-bitrate"))
        td.bitrate = map["demux-bitrate"].toInt();
    else if (map.contains("metadata")) {
        QVariantMap metadata = map["metadata"].toMap();
        if (metadata.contains("BPS"))
            td.bitrate = metadata["BPS"].toInt();
    }
    if (map.contains("lang"))
        td.lang = map["lang"].toString();
    if (map.contains("title"))
        td.title = map["title"].toString();
    if (map.contains("forced"))
        td.isForced = map["forced"].toBool();
    if (map.contains("external"))
        td.isExternal = map["external"].toBool();
    if (map.contains("default"))
        td.isDefault = map["default"].toBool();
    if (map.contains("image"))
        td.isImage = map["image"].toBool();
    return td;
}

QString TrackData::formatted(const QString &nowPlayingTitle)
{
    QString output;
    output.append(QString("%1: ").arg(trackId));
    if (!lang.isEmpty()) {
#if QT_VERSION < QT_VERSION_CHECK(6,3,0)
        QString langName = QLocale::languageToString(QLocale(lang).language());
#else
        QString langName = QLocale::languageToString(QLocale::codeToLanguage(lang, QLocale::AnyLanguageCode));
#endif
        output.append(QString("%1 ").arg(langName));
        output.append(QString("[%1] ").arg(lang));
    }
    if (!codec.isEmpty() && !isImage) {
        if (!lang.isEmpty())
            output.append("(");
        output.append(QString("%1").arg(codec));
        if (type != "sub") {
            if (!codecProfile.isEmpty())
                output.append(QString(" %1").arg(codecProfile.toCaseFolded()));
            if (samplerate != 0)
                output.append(QString(", %1 kHz").arg(std::round(samplerate / 100) / 10.0));
            if (!channels.isEmpty())
                output.append(QString(", %1").arg(channels.contains("unknown") ?
                                                  channels.remove("unknown") + "ch" : channels));
            if (bitrate != 0)
                output.append(QString(", %1 kb/s").arg(std::round(bitrate / 1000.0)));
            if (width != 0 && height != 0)
                output.append(QString(", %1x%2").arg(width).arg(height));
        }
        if (!lang.isEmpty())
            output.append(")");
    }
    title.remove(nowPlayingTitle);
    if (!title.isEmpty())
        output.append(QString(" - %1 ").arg(title));
    return output;
}



PlaybackManager::PlaybackManager(QObject *parent) :
    QObject(parent)
{
}

void PlaybackManager::setMpvObject(MpvObject *mpvObject, bool makeConnections)
{
    mpvObject_ = mpvObject;

    if (makeConnections) {
        connect(mpvObject, &MpvObject::playTimeChanged,
                this, &PlaybackManager::mpvw_playTimeChanged);
        connect(mpvObject, &MpvObject::playLengthChanged,
                this, &PlaybackManager::mpvw_playLengthChanged);
        connect(mpvObject, &MpvObject::seekableChanged,
                this, &PlaybackManager::mpvw_seekableChanged);
        connect(mpvObject, &MpvObject::playbackLoading,
                this, &PlaybackManager::mpvw_playbackLoading);
        connect(mpvObject, &MpvObject::playbackStarted,
                this, &PlaybackManager::mpvw_playbackStarted);
        connect(mpvObject, &MpvObject::pausedChanged,
                this, &PlaybackManager::mpvw_pausedChanged);
        connect(mpvObject, &MpvObject::playbackIdling,
                this, &PlaybackManager::mpvw_playbackIdling);
        connect(mpvObject, &MpvObject::playbackFinished,
                this, &PlaybackManager::mpvw_playbackFinished);
        connect(mpvObject, &MpvObject::eofReachedChanged,
                this, &PlaybackManager::mpvw_eofReachedChanged);
        connect(mpvObject, &MpvObject::mediaTitleChanged,
                this, &PlaybackManager::mpvw_mediaTitleChanged);
        connect(mpvObject, &MpvObject::chaptersChanged,
                this, &PlaybackManager::mpvw_chaptersChanged);
        connect(mpvObject, &MpvObject::tracksChanged,
                this, &PlaybackManager::mpvw_tracksChanged);
        connect(mpvObject, &MpvObject::videoSizeChanged,
                this, &PlaybackManager::mpvw_videoSizeChanged);
        connect(mpvObject, &MpvObject::fpsChanged,
                this, &PlaybackManager::mpvw_fpsChanged);
        connect(mpvObject, &MpvObject::avsyncChanged,
                this, &PlaybackManager::mpvw_avsyncChanged);
        connect(mpvObject, &MpvObject::displayFramedropsChanged,
                this, &PlaybackManager::mpvw_displayFramedropsChanged);
        connect(mpvObject, &MpvObject::decoderFramedropsChanged,
                this, &PlaybackManager::mpvw_decoderFramedropsChanged);
        connect(mpvObject, &MpvObject::metaDataChanged,
                this, &PlaybackManager::mpvw_metadataChanged);
        connect(mpvObject, &MpvObject::playlistChanged,
                this, &PlaybackManager::mpvw_playlistChanged);
        connect(mpvObject, &MpvObject::audioBitrateChanged,
                this, &PlaybackManager::mpvw_audioBitrateChanged);
        connect(mpvObject, &MpvObject::videoBitrateChanged,
                this, &PlaybackManager::mpvw_videoBitrateChanged);
        connect(mpvObject, &MpvObject::aspectNameChanged,
                this, &PlaybackManager::mpvw_aspectNameChanged);
        connect(mpvObject, &MpvObject::hwdecCurrentChanged,
                this, &PlaybackManager::mpvw_hwdecCurrentChanged);

        connect(this, &PlaybackManager::hasNoVideo,
                mpvObject, &MpvObject::setDrawLogo);
    }
}

void PlaybackManager::setPlaylistWindow(PlaylistWindow *playlistWindow)
{
    playlistWindow_ = playlistWindow;
    connect(playlistWindow, &PlaylistWindow::itemDesired,
            this, &PlaybackManager::playItem);
    connect(this, &PlaybackManager::nowPlayingChanged,
            playlistWindow, &PlaylistWindow::changePlaylistSelection);
}

QUrl PlaybackManager::nowPlaying()
{
    return nowPlaying_;
}

bool PlaybackManager::eofReached()
{
    return eofReached_;
}

PlaybackManager::PlaybackState PlaybackManager::playbackState()
{
    return playbackState_;
}

void PlaybackManager::openSeveralFiles(QList<QUrl> what, bool important)
{
    if (!nowPlayingItem.isNull())
        emit openingNewFile();

    if (important) {
        playlistWindow_->setCurrentPlaylist(QUuid());
        if (!appendToQuickPlaylist)
            playlistWindow_->clearPlaylist(QUuid());
    }
    bool playAfterAdd = (playlistWindow_->isCurrentPlaylistEmpty()
                         && (important || nowPlayingItem == QUuid()))
                        || !playlistWindow_->isVisible()
                        || (appendToQuickPlaylist && (playbackState_ == StoppedState
                                                  || playbackState_ == WaitingState));
    PlaylistItem playlistItem = playlistWindow_->addToCurrentPlaylist(what);
    if (playAfterAdd && !playlistItem.item.isNull()) {
        QUrl urlToPlay = playlistWindow_->getUrlOf(playlistItem.list, playlistItem.item);
        startPlayWithUuid(urlToPlay, playlistItem.list, playlistItem.item, false);
    }
}

void PlaybackManager::openFile(QUrl what, QUrl with)
{
    if (!nowPlayingItem.isNull())
        emit openingNewFile();

    PlaylistItem playlistItem = playlistWindow_->urlToQuickPlaylist(what, appendToQuickPlaylist);
    if (!playlistItem.item.isNull()) {
        QUrl urlToPlay = playlistWindow_->getUrlOf(playlistItem.list, playlistItem.item);
        startPlayWithUuid(urlToPlay, playlistItem.list, playlistItem.item, false, with);
    }
}

void PlaybackManager::playDiscFiles(QUrl where)
{
    if (playbackState_ != StoppedState) {
        playbackState_ = StoppedState;
        emit stateChanged(playbackState_);
        mpvObject_->stopPlayback();
    }
    mpvStartTime = -1.0;
    mpvObject_->discFilesOpen(where.toLocalFile());
    mpvObject_->setPaused(false);
    playbackStartState = PlayingState;
    nowPlayingItem = QUuid();
    nowPlayingList = QUuid();
    emit nowPlayingChanged(where, QUuid(), QUuid());
}

void PlaybackManager::playStream(QUrl stream)
{
    PlaylistItem playlistItem = playlistWindow_->addToCurrentPlaylist({ stream });
    QUrl urlToPlay = playlistWindow_->getUrlOf(playlistItem.list, playlistItem.item);
    startPlayWithUuid(urlToPlay, playlistItem.list, playlistItem.item, false);
}

void PlaybackManager::playItem(QUuid playlist, QUuid item, bool clickedInPlaylist)
{
    auto url = playlistWindow_->getUrlOf(playlist, item);
    startPlayWithUuid(url, playlist, item, false, QUrl(), clickedInPlaylist);
}

void PlaybackManager::playDevice(QUrl device)
{
    Q_UNUSED(device)
    //FIXME: detect dvb dongles, and use a channel map (or make one?)
}

void PlaybackManager::loadSubtitle(QUrl with)
{
    QString f = with.isLocalFile() ? with.toLocalFile()
                                   : QUrl::fromPercentEncoding(with.toEncoded());
    mpvObject_->addSubFile(f);
}

void PlaybackManager::playPlayer()
{
    unpausePlayer();
    if (playbackState_ == StoppedState) {
        startPlayer();
    }
}

void PlaybackManager::startPlayer()
{
    if (!playlistWindow_->playActiveItem())
        playlistWindow_->playCurrentItem();
}

void PlaybackManager::playPausePlayer()
{
    switch (playbackState_) {
    case PausedState:
        unpausePlayer();
        break;
    case StoppedState:
        startPlayer();
        break;
    default:
        pausePlayer();
    }
}

void PlaybackManager::pausePlayer()
{
    if (playbackState_ == PlayingState)
        mpvObject_->setPaused(true);
}

void PlaybackManager::unpausePlayer()
{
    if (playbackState_ == PausedState) {
        if (eofReached_)
            navigateToTime(0);
        mpvObject_->setPaused(false);
    }
}

void PlaybackManager::stopPlayer()
{
    nowPlayingItem = QUuid();
    mpvObject_->stopPlayback();
}

void PlaybackManager::stepBackward()
{
    mpvObject_->stepBackward();
}

void PlaybackManager::stepForward()
{
    mpvObject_->stepForward();
}

void PlaybackManager::navigateToNextChapter()
{
    int64_t nextChapter = mpvObject_->chapter() + 1;
    if (nextChapter >= numChapters)
        playNext(false);
    else
        navigateToChapter(nextChapter);
}

void PlaybackManager::navigateToPrevChapter()
{
    int64_t chapter = mpvObject_->chapter();
    if (chapter > 0)
        navigateToChapter(std::max(int64_t(0), chapter - 1));
    else
        playPrev(false);
}

bool PlaybackManager::playNext(bool forceFolderFallback, bool replaceMpvPlaylist)
{
    if ((folderFallback || forceFolderFallback) && playlistWindow_->isPlaylistSingularFile(nowPlayingList))
        return playNextFile(replaceMpvPlaylist);
    else
        return playNextTrack(replaceMpvPlaylist);
}

void PlaybackManager::playPrev(bool forceFolderFallback)
{
    if ((folderFallback || forceFolderFallback) && playlistWindow_->isPlaylistSingularFile(nowPlayingList)) {
        playPrevFile();
    } else {
        playPrevTrack();
    }
}

void PlaybackManager::moveToRecycleBin()
{
    if (nowPlayingItem.isNull())
        return;

    QUuid oldPlayingItem = nowPlayingItem;
    QUrl currentFileUrl;
    if (nowPlayingList.isNull())
        currentFileUrl = nowPlaying_;
    else
        currentFileUrl = playlistWindow_->getUrlOf(nowPlayingList, nowPlayingItem);

    if (currentFileUrl.isEmpty() || !currentFileUrl.isLocalFile())
        return;

    QString filePath = currentFileUrl.toLocalFile();
    QFileInfo fileInfo(filePath);

    bool hasNextFile = playNext(folderFallback);
    emit removePlaylistItemRequested(oldPlayingItem);

    if (fileInfo.exists()) {
        QFile file(filePath);
        if (file.moveToTrash())
            mpvObject_->showMessage(tr("File moved to recycle bin: %1").arg(fileInfo.fileName()));
        else
            mpvObject_->showMessage(tr("Failed to move file to recycle bin: %1").arg(fileInfo.fileName()));
    }

    if (!hasNextFile)
        stopPlayer();
}

void PlaybackManager::repeatThisFile()
{
    startPlayWithUuid(nowPlaying_, nowPlayingList, nowPlayingItem,
                      true, QUrl(), false);
}

void PlaybackManager::deltaExtraPlaytimes(int delta)
{
    playlistWindow_->deltaExtraPlayTimes(nowPlayingList, nowPlayingItem, delta);
}

void PlaybackManager::navigateToChapter(int64_t chapter)
{
    if (!mpvObject_->setChapter(chapter)) {
        // Out-of-bounds chapter navigation request. i.e. unseekable chapter
        // from either past-the-end or invalid.  So stop playback and continue
        // on the next via the playback finished slot.
        Logger::log(logModule, "navigateToChapter: setChapter failed");
        if (folderFallback)
            playNext(false);
        else {
            mpvObject_->setPaused(false);
            mpvObject_->stopPlayback();
        }
    }
}

void PlaybackManager::navigateToTime(double time)
{
    if (playbackState_ == WaitingState || playbackState_ == StoppedState)
        mpvStartTime = time;
    else
        mpvObject_->setTime(time);
}

void PlaybackManager::speedUp()
{
    double speed = speedStepAdditive ? mpvSpeed + speedStep - 1.0
                                     : mpvSpeed * speedStep;
    setPlaybackSpeed(std::min(100.0, speed));
}

void PlaybackManager::speedDown()
{
    double speed = speedStepAdditive ? mpvSpeed - speedStep + 1.0
                                     : mpvSpeed / speedStep;
    double minSpeed = std::max(0.01, speedStep - 1.0);
    setPlaybackSpeed(std::max(minSpeed, speed));
}

void PlaybackManager::speedReset()
{
    setPlaybackSpeed(1.0);
}

void PlaybackManager::relativeSeek(bool forwards, bool isLarge)
{
    mpvObject_->seek((forwards ? 1.0 : -1.0) *
                     (isLarge ? stepTimeLarge : stepTimeNormal), fastHardwareDecoding || !fastSeek);
}

void PlaybackManager::setPlaybackSpeed(double speed)
{
    mpvSpeed = speed;
    mpvObject_->setSpeed(speed);
    mpvObject_->showMessage(tr("Speed: %1%").arg(speed*100));
    emit playbackSpeedChanged(speed);
}

void PlaybackManager::setAppendToQuickPlaylist(bool isAppend)
{
    appendToQuickPlaylist = isAppend;
}

void PlaybackManager::setSpeedStep(double step)
{
    speedStep = step;
}

void PlaybackManager::setSpeedStepAdditive(bool isAdditive)
{
    speedStepAdditive = isAdditive;
}

void PlaybackManager::setStepTimeNormal(int normalMsec)
{
    stepTimeNormal = normalMsec / 1000.0;
}

void PlaybackManager::setStepTimeLarge(int largeMsec)
{
    stepTimeLarge = largeMsec / 1000.0;
}

void PlaybackManager::setSubtitleTrackPreference(QString langs)
{
    subtitleLangPref = langs.split(*wordSplitter, Qt::SkipEmptyParts);
}

void PlaybackManager::setAudioTrackPreference(QString langs)
{
    audioLangPref = langs.split(*wordSplitter, Qt::SkipEmptyParts);
}

void PlaybackManager::setAudioTrack(int64_t id, bool userSelected)
{
    if (userSelected) {
        audioTrackSelected = id;
        audioTrackSelectedFor = nowPlayingItem;
    }
    mpvObject_->setAudioTrack(id);
}

void PlaybackManager::setSubtitleTrack(int64_t id, bool userSelected)
{
    if (userSelected) {
        subtitleTrackSelected = id;
        subtitleTrackSelectedFor = nowPlayingItem;
    }
    subtitleTrackActive = id;
    updateSubtitleTrack();
}

void PlaybackManager::setVideoTrack(int64_t id, bool userSelected)
{
    if (userSelected) {
        videoTrackSelected = id;
        videoTrackSelectedFor = nowPlayingItem;
    }
    mpvObject_->setVideoTrack(id);
}

void PlaybackManager::selectNextAudioTrack()
{
    if (audioList.isEmpty())
        return;
    int64_t nextAudioTrack = audioTrackSelected + 1;
    if (nextAudioTrack > audioList.count() || nextAudioTrack < 1)
        nextAudioTrack = 1;
    setAudioTrack(nextAudioTrack, true);
    mpvObject_->showMessage(tr("Audio track: ") + audioList[nextAudioTrack - 1].title);
}

void PlaybackManager::selectPrevAudioTrack()
{
    if (audioList.isEmpty())
        return;
    int64_t previousAudioTrack = audioTrackSelected - 1;
    if (previousAudioTrack < 1)
        previousAudioTrack = audioList.count();
    setAudioTrack(previousAudioTrack, true);
    mpvObject_->showMessage(tr("Audio track: ") + audioList[previousAudioTrack - 1].title);
}

void PlaybackManager::setSubtitleEnabled(bool enabled, bool onInit)
{
    subtitleEnabled = enabled;
    updateSubtitleTrack();
    if (!onInit)
        mpvObject_->showMessage(subtitleEnabled ? tr("Subtitles: on") : tr("Subtitles: off"));
}

void PlaybackManager::selectNextSubtitle()
{
    if (subtitleList.isEmpty())
        return;
    int64_t nextSubs = subtitleTrackSelected + 1;
    if (nextSubs >= subtitleList.count())
        nextSubs = 0;
    setSubtitleTrack(nextSubs, true);
    int64_t nextSubsRealIdx = nextSubs - 1;
    if (nextSubsRealIdx < 0)
        nextSubsRealIdx = subtitleList.count() - 1;
    mpvObject_->showMessage(tr("Subtitles track: ") + subtitleList[nextSubsRealIdx].title);
}

void PlaybackManager::selectPrevSubtitle()
{
    if (subtitleList.isEmpty())
        return;
    int64_t previousSubs = subtitleTrackSelected - 1;
    if (previousSubs < 0)
        previousSubs = subtitleList.count() - 1;
    setSubtitleTrack(previousSubs, true);
    int64_t previousSubsRealIdx = previousSubs - 1;
    if (previousSubsRealIdx < 0)
        previousSubsRealIdx = subtitleList.count() - 1;
    mpvObject_->showMessage(tr("Subtitles track: ") + subtitleList[previousSubsRealIdx].title);
}

void PlaybackManager::setVolume(int64_t volume, bool onInit)
{
    static int64_t lastVol = -1;
    if (lastVol == volume)
        return;
    lastVol = volume;
    mpvObject_->setVolume(volume);
    if (!onInit)
        mpvObject_->showMessage(tr("Volume: %1%").arg(volume));
}

void PlaybackManager::setMute(bool muted, bool onInit)
{
    mpvObject_->setMute(muted);
    if (!onInit)
        mpvObject_->showMessage(muted ? tr("Mute: on") : tr("Mute: off"));
}

void PlaybackManager::setAfterPlaybackOnce(AfterPlayback mode)
{
    afterPlaybackOnce = mode;
}

void PlaybackManager::setAfterPlaybackAlways(AfterPlayback mode)
{
    afterPlaybackAlways = mode;
}

void PlaybackManager::setAfterPlaybackAlwaysDefault(Helpers::AfterPlayback mode)
{
    static bool setOnce = false;
    if (!setOnce) {
        setOnce = true;
        setAfterPlaybackAlways(mode);
    }
}

void PlaybackManager::setTimeShortMode(bool shortMode)
{
    timeShortMode = shortMode;
    updateChapters();
}

void PlaybackManager::setSubtitlesPreferDefaultForced(bool forced)
{
    subtitlesPreferDefaultForced = forced;
}

void PlaybackManager::setSubtitlesPreferExternal(bool external)
{
    subtitlesPreferExternal = external;
}

void PlaybackManager::setSubtitlesIgnoreEmbedded(bool ignore)
{
    subtitlesIgnoreEmbedded = ignore;
}

void PlaybackManager::setPlaybackPlayTimes(int times)
{
    this->playbackPlayTimes = times > 1 ? times : 1;
    this->playbackStartPaused = times < 1;
}

void PlaybackManager::setPlaybackForever(bool yes)
{
    this->playbackForever = yes;
}

void PlaybackManager::setFastSeek(bool yes)
{
    fastSeek = yes;
}

void PlaybackManager::setFolderFallback(bool yes)
{
    folderFallback = yes;
}

void PlaybackManager::sendCurrentTrackInfo()
{
    QUrl url(playlistWindow_->getUrlOf(nowPlayingList, nowPlayingItem));
    emit currentTrackInfo({url, nowPlayingList, nowPlayingItem,
                           nowPlayingTitle_, mpvLength, mpvTime,
                           videoTrackSelected, audioTrackSelected, subtitleTrackSelected});
}

void PlaybackManager::getCurrentTrackInfo(QUrl& url, QUuid& listUuid, QUuid& itemUuid, QString& title,
                                          double& length, double& position, int64_t& videoTrack,
                                          int64_t& audioTrack, int64_t& subtitleTrack, bool& hasVideo)
{
    url = playlistWindow_->getUrlOf(nowPlayingList, nowPlayingItem);
    listUuid = nowPlayingList;
    itemUuid = nowPlayingItem;
    title = nowPlayingTitle_;
    length = mpvLength;
    position = mpvTime;
    videoTrack = videoTrackSelected;
    audioTrack = audioTrackSelected;
    subtitleTrack = subtitleTrackSelected;
    hasVideo = !videoList.isEmpty() && !videoListData[1].isImage;
}

QString PlaybackManager::nowPlayingTitle()
{
    return nowPlayingTitle_;
}

void PlaybackManager::startPlayWithUuid(QUrl what, QUuid playlistUuid,
                                        QUuid itemUuid, bool isRepeating,
                                        QUrl with, bool clickedInPlaylist,
                                        bool replaceMpvPlaylist)
{
    Logger::log(logModule, "startPlayWithUuid");
    if (playbackState_ == WaitingState || what.isEmpty())
        return;
    emit stateChanged(playbackState_ = WaitingState);

    mpvStartTime = -1.0;

    if (!isRepeating)
        emit aboutToStartPlayingFile(what);

    nowPlaying_ = what;
    nowPlayingList = playlistUuid;
    nowPlayingItem = itemUuid;
    if (!isRepeating)
        emit startedPlayingFile(what);
    mpvObject_->fileOpen(what.isLocalFile() ? what.toLocalFile()
                                            : QUrl::fromPercentEncoding(what.toEncoded()),
                         replaceMpvPlaylist);
    mpvObject_->setSubFile(with.toString());
    mpvObject_->setPaused(playbackStartPaused);
    playbackStartState = playbackStartPaused ? PausedState : PlayingState;

    if (!isRepeating && playbackPlayTimes > 1
            && playlistWindow_->extraPlayTimes(playlistUuid, itemUuid) <= 0) {
        // On first play, when playing more than once, and when the extra
        // play times has not been set, set the extra play times to the one
        // configured in the settings dialog.
        playlistWindow_->setExtraPlayTimes(playlistUuid, itemUuid, playbackPlayTimes - 1);
    }
    emit nowPlayingChanged(nowPlaying_, nowPlayingList, nowPlayingItem, clickedInPlaylist);
}

void PlaybackManager::selectDesiredTracks()
{
    Logger::log(logModule, "selectDesiredTracks");
    auto findSubIdByPreference = [&](void) -> int64_t {
        if (subtitlesPreferExternal) {
            for (auto it = subtitleListData.constBegin();
                 it != subtitleListData.constEnd(); it++) {
                if (it.value().isExternal) {
                    Logger::log(logModule,
                                "external subtitle track auto selected: " + QString::number(it.key()));
                    return it.key();
                }
            }
        }
        if (subtitlesPreferDefaultForced) {
            for (auto it = subtitleListData.constBegin();
                 it != subtitleListData.constEnd(); it++)
                if (it.value().isForced || it.value().isDefault) {
                    Logger::log(logModule,
                                "default/forced subtitle track auto selected: " + QString::number(it.key()));
                    return it.key();
                }
        }
        return -1;
    };
    auto findTrackByLangPreference = [&](const QStringList &langPref,
                                         const QMap<int64_t,TrackData> &tracks) -> int64_t {
        int64_t lastGoodTrack = -1;
        for (const QString &lang : langPref) {
            for (auto it = tracks.constBegin();
                 it != tracks.constEnd(); it++)
                if (it.value().lang == lang) {
                    if (it.value().isForced || it.value().isDefault)
                        lastGoodTrack = it.key();
                    else{
                        Logger::log(logModule,
                                    "lang track auto selected: " + QString::number(it.key()));
                        return it.key();
                    }
                }
        }
        Logger::log(logModule,
                    "lastGoodTrack track auto selected: " + QString::number(lastGoodTrack));
        return lastGoodTrack;
    };
    int64_t videoId = videoTrackSelectedFor == nowPlayingItem ? videoTrackSelected : -1;
    int64_t audioId = audioTrackSelectedFor == nowPlayingItem ? audioTrackSelected : -1;
    int64_t subsId = subtitleTrackSelectedFor == nowPlayingItem ? subtitleTrackSelected : -1;

    if (audioId < 0)
        audioId = findTrackByLangPreference(audioLangPref, audioListData);
    if (subsId < 0) subsId = findSubIdByPreference();
    if (subsId < 0)
        subsId = findTrackByLangPreference(subtitleLangPref, subtitleListData);
    if (subsId < 0)
        subsId = 1;
    // Set detected tracks
    setVideoTrack(videoId, false);
    setAudioTrack(audioId, false);
    setSubtitleTrack(subsId, false);
}

void PlaybackManager::updateSubtitleTrack()
{
    emit subtitlesVisible(subtitleEnabled && subtitleTrackActive != 0);
    mpvObject_->setSubtitleTrack(subtitleEnabled ? subtitleTrackActive : 0);
}

void PlaybackManager::restoreAudioTrack(int64_t id)
{
    audioTrackSelected = id;
    audioTrackSelectedFor = nowPlayingItem;
}

void PlaybackManager::restoreSubtitleTrack(int64_t id)
{
    subtitleTrackSelected = id;
    subtitleTrackSelectedFor = nowPlayingItem;
}

void PlaybackManager::restoreVideoTrack(int64_t id)
{
    videoTrackSelected = id;
    videoTrackSelectedFor = nowPlayingItem;
}

void PlaybackManager::checkAfterPlayback()
{
    Helpers::AfterPlayback action = afterPlaybackOnce;
    if (afterPlaybackOnce == Helpers::DoNothingAfter)
        action = afterPlaybackAlways;

    afterPlaybackOnce = Helpers::DoNothingAfter;
    if (action != Helpers::RepeatAfter && action != Helpers::PlayNextAfter)
        emit afterPlaybackReset();

    switch (action) {
    case Helpers::ExitAfter:
        emit instanceShouldClose();
        break;
    case Helpers::StandByAfter:
        emit systemShouldStandby();
        break;
    case Helpers::HibernateAfter:
        emit systemShouldHibernate();
        break;
    case Helpers::ShutdownAfter:
        emit systemShouldShutdown();
        break;
    case Helpers::LogOffAfter:
        emit systemShouldLogOff();
        break;
    case Helpers::LockAfter:
        emit systemShouldLock();
        break;
    case Helpers::PlayNextAfter:
        playNext(true, false);
        break;
    case Helpers::DoNothingAfter:
        playNextTrack(false);
        break;
    case Helpers::RepeatAfter:
        break;
    }
}

void PlaybackManager::updateChapters()
{
    QList<Chapter> list;

    if (!chapters.isEmpty()) {
        for (QVariant const &v : chapters) {
            QMap<QString, QVariant> node = v.toMap();
            QString text = QString("[%1] - %2").arg(
                    toDateFormatFixed(node["time"].toDouble(),
                                    timeShortMode ? Helpers::ShortFormat : Helpers::LongFormat),
                    node["title"].toString());
            Chapter item { node["time"].toDouble(), text };
            list.append(item);
        }
    }
    numChapters = list.count();
    emit chaptersAvailable(list);
}

bool PlaybackManager::playNextTrack(bool replaceMpvPlaylist)
{
    PlaylistItem next;
    next = playlistWindow_->getItemAfter(nowPlayingList, nowPlayingItem);
    if (next.item.isNull() && playlistWindow_->isPlaylistRepeat(nowPlayingList)) {
        if (playlistWindow_->isPlaylistShuffle(nowPlayingList))
            playlistWindow_->reshufflePlaylist(nowPlayingList);
        next = playlistWindow_->getFirstItem(nowPlayingList);
    }
    QUrl url = playlistWindow_->getUrlOf(next.list, next.item);
    if (!url.isEmpty()) {
        startPlayWithUuid(url, next.list, next.item,
                          false, QUrl(), false, replaceMpvPlaylist);
        return true;
    }
    return false;
}

void PlaybackManager::playPrevTrack()
{
    QUuid itemUuid = playlistWindow_->getItemBefore(nowPlayingList, nowPlayingItem);
    QUrl url = playlistWindow_->getUrlOf(nowPlayingList, itemUuid);
    if (!url.isEmpty())
        startPlayWithUuid(url, nowPlayingList, itemUuid, false);
}

bool PlaybackManager::playNextFileUrl(QUrl url, int delta, bool replaceMpvPlaylist)
{
    Logger::log(logModule, "playNextFileUrl");
    QFileInfo info;
    QDir dir;
    QStringList files;
    int index;
    QString nextFile;

    if (url.isEmpty())
        return false;
    info = QFileInfo(url.toLocalFile());
    if (!info.exists())
        return false;
    dir = info.dir();
    files = dir.entryList(QDir::Files);

    // TODO: Use refactored Helpers::compareUrls as comparator and a sort function in Helpers
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    collator.setNumericMode(true);
    std::sort(files.begin(), files.end(), collator);

    index = files.indexOf(info.fileName());
    if (index == -1)
        return false;
    do {
        index += delta;
        if (index < 0 || index >= files.count())
            return false;
        nextFile = dir.filePath(files.value(index));
        url = QUrl::fromLocalFile(nextFile);
    } while (!Helpers::urlSurvivesFilter(url, true));
    emit playingNextFile();
    playlistWindow_->clearPlaylist(nowPlayingList);
    QUuid nowPlayingItemLocal = playlistWindow_->addToPlaylist(nowPlayingList, { url }).item;
    startPlayWithUuid(url, nowPlayingList, nowPlayingItemLocal,
                      false, QUrl(), false, replaceMpvPlaylist);
    return true;
}

bool PlaybackManager::playNextFile(bool replaceMpvPlaylist, int delta)
{
    QUrl url = playlistWindow_->getUrlOfFirst(nowPlayingList);
    return playNextFileUrl(url, delta, replaceMpvPlaylist);
}

void PlaybackManager::playPrevFile()
{
    playNextFile(true, -1);
}

void PlaybackManager::mpvw_playTimeChanged(double time)
{
    // in case the duration property is not available, update the play length
    // to indicate that the time is in fact available.
    if (mpvLength < time)
        mpvLength = time;
    mpvTime = time;
    emit timeChanged(time, mpvLength);
}

void PlaybackManager::mpvw_playLengthChanged(double length)
{
    mpvLength = length;
    emit playLengthChanged();
}

void PlaybackManager::mpvw_seekableChanged(bool yes)
{
    if (yes && mpvStartTime > 0) {
        mpvObject_->setTimeSync(mpvStartTime);
        mpvStartTime = -1;
    }
}

void PlaybackManager::mpvw_playbackLoading()
{
    playbackState_ = BufferingState;
    emit stateChanged(playbackState_);
}

void PlaybackManager::mpvw_playbackStarted()
{
    playbackState_ = playbackStartState;
    emit stateChanged(playbackState_);
}

void PlaybackManager::mpvw_pausedChanged(bool yes)
{
    if (playbackState_ == StoppedState)
        return;

    playbackState_ = yes ? PausedState : PlayingState;
    emit stateChanged(playbackState_);
}

void PlaybackManager::mpvw_playbackIdling()
{
    if (nowPlayingItem.isNull()) {
        nowPlaying_.clear();
        playbackState_ = StoppedState;
        emit stateChanged(playbackState_);
        return;
    }
}

void PlaybackManager::mpvw_playbackFinished() {
    if (playbackState_ == BufferingState) {
        playbackState_ = StoppedState;
        emit stateChanged(playbackState_);
        mpvw_eofReachedChanged(strTrue);
    }
}

void PlaybackManager::mpvw_eofReachedChanged(QString eof) {
    LogStream(logModule) << "mpvw_eofReachedChanged eof: " << eof;
    if (eof == strFalse) {
        eofReached_ = false;
        return;
    }
    else if (eof.isEmpty()) {
        emit fileClosed();
        showAspectOsdTriggeredBy = AspectNameChanged::OnOpen;
        return;
    }
    eofReached_ = true;

    emit stoppedPlaying();

    int extraTimes = playlistWindow_->extraPlayTimes(nowPlayingList, nowPlayingItem);
    playlistWindow_->setExtraPlayTimes(nowPlayingList, nowPlayingItem, extraTimes - 1);

    bool isRepeating = playbackForever || extraTimes > 0;
    if (isRepeating)
        repeatThisFile();
    else
        checkAfterPlayback();
}

void PlaybackManager::mpvw_mediaTitleChanged(QString title)
{
    nowPlayingTitle_ = title;
    emit titleChanged(title);
    emit titleChangedWithFilename(title,
                                  nowPlaying().isLocalFile() ? nowPlaying().fileName() : QString());
}

void PlaybackManager::mpvw_chaptersChanged(QVariantList chapters)
{
    this->chapters = chapters;
    updateChapters();
}

void PlaybackManager::mpvw_tracksChanged(QVariantList tracks)
{
    Logger::log(logModule, "mpvw_tracksChanged");
    videoList.clear();
    audioList.clear();
    videoListData.clear();
    audioListData.clear();
    subtitleList.clear();
    subtitleListData.clear();
    Track track;

    for (QVariant const &trackItem : tracks) {
        TrackData td = TrackData::fromMap(trackItem.toMap());
        track.id = td.trackId;
        track.title = td.formatted(nowPlayingTitle_);
        if (td.type == "video") {
            videoList.append(track);
            videoListData.insert(td.trackId, td);
        } else if (td.type == "audio") {
            audioList.append(track);
            audioListData.insert(td.trackId, td);
        } else if (td.type == "sub") {
            if (!subtitlesIgnoreEmbedded || td.isExternal) {
                subtitleList.append(track);
                subtitleListData.insert(td.trackId, td);
            }
        }
    }
    if (!subtitleList.isEmpty()) {
        subtitleList.append({0, tr("0: None")});
    }

    emit videoTracksAvailable(videoList);
    emit audioTracksAvailable(audioList);
    emit subtitleTracksAvailable(subtitleList);

    selectDesiredTracks();

    emit hasNoVideo(videoList.empty());
    emit hasNoAudio(audioList.empty());
    emit hasNoSubtitles(subtitleList.empty());
    emit isVideo(!videoList.isEmpty() && !videoListData[1].isImage);
}

void PlaybackManager::mpvw_videoSizeChanged(QSize size)
{
    emit videoSizeChanged(size);
}

void PlaybackManager::mpvw_fpsChanged(double fps)
{
    emit fpsChanged(fps);
}

void PlaybackManager::mpvw_avsyncChanged(double sync)
{
    emit avsyncChanged(sync);
}

void PlaybackManager::mpvw_displayFramedropsChanged(int64_t count)
{
    emit displayFramedropsChanged(count);
}

void PlaybackManager::mpvw_decoderFramedropsChanged(int64_t count)
{
    emit decoderFramedropsChanged(count);
}

void PlaybackManager::mpvw_audioBitrateChanged(double bitrate)
{
    emit audioBitrateChanged(bitrate);
}

void PlaybackManager::mpvw_videoBitrateChanged(double bitrate)
{
    emit videoBitrateChanged(bitrate);
}

void PlaybackManager::mpvw_aspectNameChanged(QString newAspectName)
{
    // If it's the first file opened with mpc-qt, OnFirstPlay gets skipped
    if (showAspectOsdTriggeredBy == AspectNameChanged::OnOpen) {
        if (newAspectName.isEmpty())
            showAspectOsdTriggeredBy = AspectNameChanged::OnFirstPlay;
        else
            showAspectOsdTriggeredBy = AspectNameChanged::Manually;
    }
    else if (showAspectOsdTriggeredBy == AspectNameChanged::OnFirstPlay)
        showAspectOsdTriggeredBy = AspectNameChanged::Manually;
    else if (!newAspectName.isEmpty())
        mpvObject_->showMessage(tr("Aspect ratio: %1").arg(newAspectName));
}

void PlaybackManager::mpvw_hwdecCurrentChanged(QString newHwdecCurrent)
{
    LogStream(logModule) << "Hardware decoder used: " << newHwdecCurrent;
    fastHardwareDecoding = !(newHwdecCurrent.isEmpty() ||
                             newHwdecCurrent == "no" ||
                             newHwdecCurrent.contains("copy"));
}

void PlaybackManager::mpvw_metadataChanged(QVariantMap metadata)
{
    // yt-dlp metadata doesn't include the title, so let's add it
    if (!nowPlaying_.isLocalFile() && !nowPlayingTitle_.isEmpty() && !metadata.contains("title"))
        metadata.insert("title", nowPlayingTitle_);
    playlistWindow_->setMetadata(nowPlayingList, nowPlayingItem, metadata);
}

void PlaybackManager::mpvw_playlistChanged(const QVariantList &playlist)
{
    Logger::log(logModule, "mpvw_playlistChanged");
    // replace current item with the content we got from the archive or playlist file, and trigger its playback
    QList<QUrl> urls;
    bool currentItemFound = false;
    for (auto const& i : playlist) {
        if (i.toMap()["current"].toBool()) {
            currentItemFound = true;
            if (i.toMap()["id"].toInt() != currentMpvPlaylistItemId)
                currentMpvPlaylistItemId = i.toMap()["id"].toInt();
            else
                return;
            if (i.toMap()["playlist-path"].toString().isEmpty())
                return;
        }
        if (!i.toMap()["playlist-path"].toString().isEmpty() &&
                Helpers::urlSurvivesFilter(QUrl::fromUserInput(i.toMap()["filename"].toString()), false))
            urls.append(QUrl::fromUserInput(i.toMap()["filename"].toString()));
    }
    if (!currentItemFound)
        return;
    playlistWindow_->replaceItem(nowPlayingList, nowPlayingItem, urls);
    playItem(nowPlayingList, nowPlayingItem);
}
