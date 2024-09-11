#include <QRegularExpression>
#include "manager.h"
#include "mpvwidget.h"
#include "playlistwindow.h"
#include "helpers.h"
#include "logger.h"

using namespace Helpers;

Q_GLOBAL_STATIC_WITH_ARGS(QRegularExpression, wordSplitter, ("\\W+"));


TrackData TrackData::fromMap(const QVariantMap &map)
{
    TrackData td;
    td.trackId = map["id"].toLongLong();
    if (map.contains("type"))
        td.type = map["type"].toString();
    if (map.contains("codec"))
        td.codec = map["codec"].toString();
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
    return td;
}

QString TrackData::formatted()
{
    QString output;
    output.append(QString("%1: ").arg(trackId));
    if (!codec.isEmpty())
        output.append(QString("[%1] ").arg(codec));
    if (!lang.isEmpty())
        output.append(QString("%1 ").arg(lang));
    if (!title.isEmpty())
        output.append(QString("- %1 ").arg(title));
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
        connect(mpvObject, &MpvObject::chapterDataChanged,
                this, &PlaybackManager::mpvw_chapterDataChanged);
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
        playlistWindow_->clearPlaylist(QUuid());
    }
    bool playAfterAdd = (playlistWindow_->isCurrentPlaylistEmpty()
                         && (important || nowPlayingItem == QUuid()))
                        || !playlistWindow_->isVisible();
    auto info = playlistWindow_->addToCurrentPlaylist(what);
    if (playAfterAdd && !info.second.isNull()) {
        QUrl urlToPlay = playlistWindow_->getUrlOf(info.first, info.second);
        startPlayWithUuid(urlToPlay, info.first, info.second, false);
    }
}

void PlaybackManager::openFile(QUrl what, QUrl with)
{
    if (!nowPlayingItem.isNull())
        emit openingNewFile();

    auto info = playlistWindow_->urlToQuickPlaylist(what);
    if (!info.second.isNull()) {
        QUrl urlToPlay = playlistWindow_->getUrlOf(info.first, info.second);
        startPlayWithUuid(urlToPlay, info.first, info.second, false, with);
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
    auto info = playlistWindow_->addToCurrentPlaylist({ stream });
    QUrl urlToPlay = playlistWindow_->getUrlOf(info.first, info.second);
    startPlayWithUuid(urlToPlay, info.first, info.second, false);
}

void PlaybackManager::playItem(QUuid playlist, QUuid item)
{
    auto url = playlistWindow_->getUrlOf(playlist, item);
    startPlayWithUuid(url, playlist, item, false);
}

void PlaybackManager::playDevice(QUrl device)
{
    Q_UNUSED(device)
    //FIXME: detect dvb dongles, and use a channel map (or make one?)
}

void PlaybackManager::loadSubtitle(QUrl with)
{
    QString f = with.isLocalFile() ? with.toLocalFile()
                                   : with.fromPercentEncoding(with.toEncoded());
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
        if (mpvObject_->eofReached())
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

void PlaybackManager::playNext(bool forceFolderFallback)
{
    if ((folderFallback || forceFolderFallback) && playlistWindow_->isPlaylistSingularFile(nowPlayingList)) {
        playNextFile();
    } else {
        playNextTrack();
    }
}

void PlaybackManager::playPrev(bool forceFolderFallback)
{
    if ((folderFallback || forceFolderFallback) && playlistWindow_->isPlaylistSingularFile(nowPlayingList)) {
        playPrevFile();
    } else {
        playPrevTrack();
    }
}

void PlaybackManager::repeatThisFile()
{
    startPlayWithUuid(nowPlaying_, nowPlayingList, nowPlayingItem, true);
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
        LogStream("manager") << "navigateToChapter: setChapter failed";
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
    setPlaybackSpeed(std::min(8.0, speed));
}

void PlaybackManager::speedDown()
{
    double speed = speedStepAdditive ? mpvSpeed - speedStep + 1.0
                                     : mpvSpeed / speedStep;
    setPlaybackSpeed(std::max(0.125, speed));
}

void PlaybackManager::speedReset()
{
    setPlaybackSpeed(1.0);
}

void PlaybackManager::relativeSeek(bool forwards, bool isSmall)
{
    mpvObject_->seek((forwards ? 1.0 : -1.0) *
                     (isSmall ? stepTimeSmall : stepTimeLarge), isSmall);
}

void PlaybackManager::setPlaybackSpeed(double speed)
{
    mpvSpeed = speed;
    mpvObject_->setSpeed(speed);
    mpvObject_->showMessage(tr("Speed: %1%").arg(speed*100));
    emit playbackSpeedChanged(speed);
}

void PlaybackManager::setSpeedStep(double step)
{
    speedStep = step;
}

void PlaybackManager::setSpeedStepAdditive(bool isAdditive)
{
    speedStepAdditive = isAdditive;
}

void PlaybackManager::setStepTimeLarge(int largeMsec)
{
    stepTimeLarge = largeMsec / 1000.0;
}

void PlaybackManager::setStepTimeSmall(int smallMsec)
{
    stepTimeSmall = smallMsec / 1000.0;
}

void PlaybackManager::setSubtitleTrackPreference(QString langs)
{
    subtitleLangPref = langs.split(*wordSplitter, Qt::SkipEmptyParts);
}

void PlaybackManager::setAudioTrackPreference(QString langs)
{
    audioLangPref = langs.split(*wordSplitter, Qt::SkipEmptyParts);
}

static QString findSecondById(QList<QPair<int64_t,QString>> list, int64_t id) {
    // this *should* return the string at id-1
    for (int i = 0; i < list.count(); ++i)
        if (list.value(i).first == id)
            return list.value(i).second;
    return QString();
}

void PlaybackManager::setAudioTrack(int64_t id)
{
    setAudioTrackEx(id, false);
}

void PlaybackManager::setAudioTrackEx(int64_t id, bool softly)
{
    if (softly)
        audioListSelected.clear();
    else
        audioListSelected = findSecondById(audioList, id);
    mpvObject_->setAudioTrack(id);
}

void PlaybackManager::setSubtitleTrack(int64_t id)
{
    setSubtitleTrackEx(id, false);
}

void PlaybackManager::setSubtitleTrackEx(int64_t id, bool softly)
{
    if (softly)
        subtitleListSelected.clear();
    else
        subtitleListSelected = findSecondById(subtitleList, id);
    subtitleTrackSelected = id;
    updateSubtitleTrack();
}

void PlaybackManager::setVideoTrack(int64_t id)
{
    videoListSelected = findSecondById(videoList, id);
    mpvObject_->setVideoTrack(id);
}

void PlaybackManager::setSubtitleEnabled(bool enabled)
{
    subtitleEnabled = enabled;
    updateSubtitleTrack();
}

void PlaybackManager::selectNextSubtitle()
{
    if (subtitleList.isEmpty())
        return;
    int64_t nextSubs = subtitleTrackSelected + 1;
    if (nextSubs >= subtitleList.count())
        nextSubs = 0;
    setSubtitleTrack(nextSubs);
}

void PlaybackManager::selectPrevSubtitle()
{
    if (subtitleList.isEmpty())
        return;
    int64_t previousSubs = subtitleTrackSelected - 1;
    if (previousSubs < 0)
        previousSubs = subtitleList.count() - 1;
    setSubtitleTrack(previousSubs);
}

void PlaybackManager::setVolume(int64_t volume)
{
    static int64_t lastVol = -1;
    if (lastVol == volume)
        return;
    lastVol = volume;
    mpvObject_->setVolume(volume);
    mpvObject_->showMessage(tr("Volume: %1%").arg(volume));
}

void PlaybackManager::setMute(bool muted)
{
    mpvObject_->setMute(muted);
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

void PlaybackManager::setFolderFallback(bool yes)
{
    folderFallback = yes;
}

void PlaybackManager::sendCurrentTrackInfo()
{
    QUrl url(playlistWindow_->getUrlOf(nowPlayingList, nowPlayingItem));
    emit currentTrackInfo({url, nowPlayingList, nowPlayingItem,
                           nowPlayingTitle, mpvLength, mpvTime});
}

void PlaybackManager::getCurrentTrackInfo(QUrl& url, QUuid& listUuid, QUuid& itemUuid, QString title,
                                          double& length, double& position)
{
    url = playlistWindow_->getUrlOf(nowPlayingList, nowPlayingItem);
    listUuid = nowPlayingList;
    itemUuid = nowPlayingItem;
    title = nowPlayingTitle;
    length = mpvLength;
    position = mpvTime;
}

void PlaybackManager::startPlayWithUuid(QUrl what, QUuid playlistUuid,
                                        QUuid itemUuid, bool isRepeating,
                                        QUrl with)
{
    LogStream("manager") << "startPlayWithUuid";
    if (playbackState_ == WaitingState || what.isEmpty())
        return;
    emit stateChanged(playbackState_ = WaitingState);

    mpvStartTime = -1.0;

    if (!isRepeating)
        emit startingPlayingFile(what);

    nowPlaying_ = what;
    mpvObject_->fileOpen(what.isLocalFile() ? what.toLocalFile()
                                            : what.fromPercentEncoding(what.toEncoded()));
    mpvObject_->setSubFile(with.toString());
    mpvObject_->setPaused(playbackStartPaused);
    playbackStartState = playbackStartPaused ? PausedState : PlayingState;
    nowPlayingList = playlistUuid;
    nowPlayingItem = itemUuid;

    if (!isRepeating && playbackPlayTimes > 1
            && playlistWindow_->extraPlayTimes(playlistUuid, itemUuid) <= 0) {
        // On first play, when playing more than once, and when the extra
        // play times has not been set, set the extra play times to the one
        // configured in the settings dialog.
        playlistWindow_->setExtraPlayTimes(playlistUuid, itemUuid, playbackPlayTimes - 1);
    }
    emit nowPlayingChanged(nowPlaying_, nowPlayingList, nowPlayingItem);
}

void PlaybackManager::selectDesiredTracks()
{
    // search current tracks by mangled string of no id and no spaces
    auto mangle = [](QString s) {
        return QStringList(s.split(' ').mid(1)).join("");
    };
    auto findIdBySecond = [&](QList<QPair<int64_t,QString>> list,
                              QString needle) -> int64_t {
        if (list.isEmpty() || (needle = mangle(needle)).isEmpty())
            return -1;
        for (int i = 0; i < list.count(); i++) {
            if (mangle(list[i].second) == needle) {
                return list[i].first;
            }
        }
        return -1;
    };
    auto findSubIdByPreference = [&](void) -> int64_t {
        if (subtitlesPreferExternal) {
            for (auto it = subtitleListData.constBegin();
                 it != subtitleListData.constEnd(); it++) {
                if (it.value().isExternal)
                    return it.key();
            }
        }
        if (subtitlesPreferDefaultForced) {
            for (auto it = subtitleListData.constBegin();
                 it != subtitleListData.constEnd(); it++)
                if (it.value().isForced
                    || it.value().isDefault)
                    return it.key();
        }
        return -1;
    };
    auto findTrackByLangPreference = [&](const QStringList &langPref,
                                         const QMap<int64_t,TrackData> &tracks) -> int64_t {
        for (const QString &lang : langPref) {
            for (auto it = tracks.constBegin();
                 it != tracks.constEnd(); it++)
                if (it.value().lang == lang)
                    return it.key();
        }
        return -1;
    };
    bool audioSoftly = false;
    bool subsSoftly = false;

    int64_t videoId = findIdBySecond(videoList, videoListSelected);
    int64_t audioId = findIdBySecond(audioList, audioListSelected);
    if (audioId < 0) {
        audioId = findTrackByLangPreference(audioLangPref, audioListData);
        if (audioId)
            audioSoftly = true;
    }
    int64_t subsId = findIdBySecond(subtitleList, subtitleListSelected);
    if (subsId < 0) subsId = findSubIdByPreference();
    if (subsId < 0) {
        subsId = findTrackByLangPreference(subtitleLangPref, subtitleListData);
        if (subsId)
            subsSoftly = true;
    }

    // Set detected tracks; if no preferred track from a list could be found,
    // clear user selection
    if (videoId >= 0)
        setVideoTrack(videoId);
    else if (!videoList.isEmpty())
        videoListSelected.clear();
    if (audioId >= 0)
        setAudioTrackEx(audioId, audioSoftly);
    else if (!audioList.isEmpty())
        audioListSelected.clear();
    if (subsId >= 0)
        setSubtitleTrackEx(subsId, subsSoftly);
    else if (!subtitleList.isEmpty()) {
        subtitleListSelected.clear();
        setSubtitleTrack(1);
    }
}

void PlaybackManager::updateSubtitleTrack()
{
    emit subtitlesVisible(subtitleEnabled && subtitleTrackSelected != 0);
    mpvObject_->setSubtitleTrack(subtitleEnabled ? subtitleTrackSelected : 0);
}

void PlaybackManager::checkAfterPlayback()
{
    Helpers::AfterPlayback action = afterPlaybackOnce;
    if (afterPlaybackOnce == Helpers::DoNothingAfter)
        action = afterPlaybackAlways;

    afterPlaybackOnce = Helpers::DoNothingAfter;
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
    case Helpers::RepeatAfter:
        repeatThisFile();
        break;
    case Helpers::PlayNextAfter:
        playNext(true);
        break;
    case Helpers::DoNothingAfter:
        playNextTrack();
    }
}

void PlaybackManager::updateChapters()
{
    if (chapters.isEmpty())
        return;

    QList<QPair<double,QString>> list;
    for (QVariant &v : chapters) {
        QMap<QString, QVariant> node = v.toMap();
        QString text = QString("[%1] - %2").arg(
                toDateFormatFixed(node["time"].toDouble(),
                                  timeShortMode ? Helpers::ShortFormat : Helpers::LongFormat),
                node["title"].toString());
        QPair<double,QString> item(node["time"].toDouble(), text);
        list.append(item);
    }
    numChapters = list.count();
    emit chaptersAvailable(list);
}

void PlaybackManager::playNextTrack()
{
    QPair<QUuid, QUuid> next;
    next = playlistWindow_->getItemAfter(nowPlayingList, nowPlayingItem);
    QUrl url = playlistWindow_->getUrlOf(next.first, next.second);
    if (!url.isEmpty())
        startPlayWithUuid(url, next.first, next.second, false);
}

void PlaybackManager::playPrevTrack()
{
    QUuid uuid = playlistWindow_->getItemBefore(nowPlayingList, nowPlayingItem);
    QUrl url = playlistWindow_->getUrlOf(nowPlayingList, uuid);
    if (!url.isEmpty())
        startPlayWithUuid(url, nowPlayingList, uuid, false);
}

bool PlaybackManager::playNextFileUrl(QUrl url, int delta)
{
    LogStream("manager") << "playNextFileUrl";
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
    files = dir.entryList(QDir::Files, QDir::Name);
    index = files.indexOf(info.fileName());
    if (index == -1)
        return false;
    do {
        index += delta;
        if (index < 0 || index >= files.count())
            return false;
        nextFile = dir.filePath(files.value(index));
        url = QUrl::fromLocalFile(nextFile);
    } while (!Helpers::urlSurvivesFilter(url));
    playlistWindow_->replaceItem(nowPlayingList, nowPlayingItem, { url });
    startPlayWithUuid(url, nowPlayingList, nowPlayingItem, false);
    return true;
}

void PlaybackManager::playNextFile(int delta)
{
    QUrl url = playlistWindow_->getUrlOfFirst(nowPlayingList);
    playNextFileUrl(url, delta);
}

void PlaybackManager::playPrevFile()
{
    playNextFile(-1);
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
    emit playerSettingsRequested();
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
    if (playbackState_ == BufferingState || playbackState_ == WaitingState) {
        playbackState_ = StoppedState;
        emit stateChanged(playbackState_);
        mpvw_eofReachedChanged(true);
    }
}

void PlaybackManager::mpvw_eofReachedChanged(bool eof) {
    LogStream("manager") << "mpvw_eofReachedChanged";
    if (!eof) {
        emit fileOpenedOrClosed();
        return;
    }

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
    nowPlayingTitle = title;
    emit titleChanged(title);
}

void PlaybackManager::mpvw_chapterDataChanged(QVariantMap metadata)
{
    emit chapterTitleChanged(metadata.value("title").toString());
}

void PlaybackManager::mpvw_chaptersChanged(QVariantList chapters)
{
    this->chapters = chapters;
    updateChapters();
}

void PlaybackManager::mpvw_tracksChanged(QVariantList tracks)
{
    videoList.clear();
    audioList.clear();
    audioListData.clear();
    subtitleList.clear();
    subtitleListData.clear();
    QPair<int64_t,QString> item;

    for (QVariant &track : tracks) {
        QVariantMap t = track.toMap();
        TrackData td = TrackData::fromMap(t);
        item.first = td.trackId;
        item.second = td.formatted();
        if (td.type == "video") {
            videoList.append(item);
        } else if (td.type == "audio") {
            audioList.append(item);
            audioListData.insert(td.trackId, td);
        } else if (td.type == "sub") {
            if (!subtitlesIgnoreEmbedded || td.isExternal) {
                subtitleList.append(item);
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

void PlaybackManager::mpvw_metadataChanged(QVariantMap metadata)
{
    playlistWindow_->setMetadata(nowPlayingList, nowPlayingItem, metadata);
}

void PlaybackManager::mpvw_playlistChanged(const QVariantList &playlist)
{
    // replace current item with whatever we got, and trigger its playback
    QList<QUrl> urls;
    for (auto i : playlist)
        urls.append(QUrl::fromUserInput(i.toMap()["filename"].toString()));
    playlistWindow_->replaceItem(nowPlayingList, nowPlayingItem, urls);
    playItem(nowPlayingList, nowPlayingItem);
}
