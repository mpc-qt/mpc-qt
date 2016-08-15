#include <QDebug>
#include <cmath>
#include "manager.h"
#include "mainwindow.h"
#include "mpvwidget.h"
#include "helpers.h"

using namespace Helpers;


PlaybackManager::PlaybackManager(QObject *parent) :
    QObject(parent), mpvSpeed(1.0), playbackState_(StoppedState),
    playbackPlayTimes(1)
{
}

void PlaybackManager::setMpvWidget(MpvWidget *mpvWidget, bool makeConnections)
{
    mpvWidget_ = mpvWidget;

    if (makeConnections) {
        connect(mpvWidget, &MpvWidget::playTimeChanged,
                this, &PlaybackManager::mpvw_playTimeChanged);
        connect(mpvWidget, &MpvWidget::playLengthChanged,
                this, &PlaybackManager::mpvw_playLengthChanged);
        connect(mpvWidget, &MpvWidget::playbackLoading,
                this, &PlaybackManager::mpvw_playbackLoading);
        connect(mpvWidget, &MpvWidget::playbackStarted,
                this, &PlaybackManager::mpvw_playbackStarted);
        connect(mpvWidget, &MpvWidget::pausedChanged,
                this, &PlaybackManager::mpvw_pausedChanged);
        connect(mpvWidget, &MpvWidget::playbackIdling,
                this, &PlaybackManager::mpvw_playbackIdling);
        connect(mpvWidget, &MpvWidget::mediaTitleChanged,
                this, &PlaybackManager::mpvw_mediaTitleChanged);
        connect(mpvWidget, &MpvWidget::chapterDataChanged,
                this, &PlaybackManager::mpvw_chapterDataChanged);
        connect(mpvWidget, &MpvWidget::chaptersChanged,
                this, &PlaybackManager::mpvw_chaptersChanged);
        connect(mpvWidget, &MpvWidget::tracksChanged,
                this, &PlaybackManager::mpvw_tracksChanged);
        connect(mpvWidget, &MpvWidget::videoSizeChanged,
                this, &PlaybackManager::mpvw_videoSizeChanged);
        connect(mpvWidget, &MpvWidget::fpsChanged,
                this, &PlaybackManager::mpvw_fpsChanged);
        connect(mpvWidget, &MpvWidget::avsyncChanged,
                this, &PlaybackManager::mpvw_avsyncChanged);
        connect(mpvWidget, &MpvWidget::displayFramedropsChanged,
                this, &PlaybackManager::mpvw_displayFramedropsChanged);
        connect(mpvWidget, &MpvWidget::decoderFramedropsChanged,
                this, &PlaybackManager::mpvw_decoderFramedropsChanged);
        connect(mpvWidget, &MpvWidget::metaDataChanged,
                this, &PlaybackManager::mpvw_metadataChanged);
        connect(mpvWidget, &MpvWidget::playlistChanged,
                this, &PlaybackManager::mpvw_playlistChanged);

        connect(this, &PlaybackManager::hasNoVideo,
                mpvWidget, &MpvWidget::setDrawLogo);
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

void PlaybackManager::startPlayWithUuid(QUrl what, QUuid playlistUuid,
                                        QUuid itemUuid, bool isRepeating)
{
    if (playbackState_ == WaitingState || what.isEmpty())
        return;
    emit stateChanged(playbackState_ = WaitingState);

    nowPlaying_ = what;
    mpvWidget_->fileOpen(what.isLocalFile() ? what.toLocalFile()
                                            : what.fromPercentEncoding(what.toEncoded()));
    this->nowPlayingList = playlistUuid;
    this->nowPlayingItem = itemUuid;

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
    int64_t videoId = findIdBySecond(videoList, videoListSelected);
    int64_t audioId = findIdBySecond(audioList, audioListSelected);
    int64_t subsId = findIdBySecond(subtitleList, subtitleListSelected);
    // Set detected tracks; if no preferred track from a list could be found,
    // clear user selection
    if (videoId >= 0)
        setVideoTrack(videoId);
    else if (!videoList.isEmpty())
        videoListSelected.clear();
    if (audioId >= 0)
        setAudioTrack(audioId);
    else if (!audioList.isEmpty())
        audioListSelected.clear();
    if (subsId >= 0)
        setSubtitleTrack(subsId);
    else if (!subtitleList.isEmpty())
        subtitleListSelected.clear();
}

void PlaybackManager::openSeveralFiles(QList<QUrl> what, bool important)
{
    if (important) {
        playlistWindow_->setCurrentPlaylist(QUuid());
        playlistWindow_->clearPlaylist(QUuid());
    }
    bool playAfterAdd = (playlistWindow_->isCurrentPlaylistEmpty()
                         && (important || nowPlayingItem == QUuid()))
                        || !playlistWindow_->isVisible();
    auto info = playlistWindow_->addToCurrentPlaylist(what);
    if (playAfterAdd) {
        startPlayWithUuid(what.front(), info.first, info.second, false);
    }
}

void PlaybackManager::openFile(QUrl what)
{
    auto info = playlistWindow_->urlToQuickPlaylist(what);
    startPlayWithUuid(what, info.first, info.second, false);
}

void PlaybackManager::playDiscFiles(QUrl where)
{
    if (playbackState_ != StoppedState) {
        playbackState_ = StoppedState;
        emit stateChanged(playbackState_);
        mpvWidget_->stopPlayback();
    }
    mpvWidget_->discFilesOpen(where.toLocalFile());
    nowPlayingItem = QUuid();
    nowPlayingList = QUuid();
    emit nowPlayingChanged(where, QUuid(), QUuid());
}

void PlaybackManager::playDisc(QUrl where)
{
    Q_UNUSED(where);
    //FIXME: use udisks2 to determine possible disc locations
}

void PlaybackManager::playStream(QUrl stream)
{
    openFile(stream);
}

void PlaybackManager::playItem(QUuid playlist, QUuid item)
{
    auto url = playlistWindow_->getUrlOf(playlist, item);
    startPlayWithUuid(url, playlist, item, false);
}

void PlaybackManager::playDevice(QUrl device)
{
    Q_UNUSED(device);
    //FIXME: detect dvb dongles, and use a channel map (or make one?)
}

void PlaybackManager::pausePlayer()
{
    if (playbackState_ == PlayingState)
        mpvWidget_->setPaused(true);
}

void PlaybackManager::unpausePlayer()
{
    if (playbackState_ == PausedState)
        mpvWidget_->setPaused(false);
}

void PlaybackManager::stopPlayer()
{
    nowPlayingItem = QUuid();
    mpvWidget_->stopPlayback();
}

void PlaybackManager::stepBackward()
{
    mpvWidget_->stepBackward();
}

void PlaybackManager::stepForward()
{
    mpvWidget_->stepForward();
}

void PlaybackManager::navigateToNextChapter()
{
    int64_t nextChapter = mpvWidget_->chapter() + 1;
    if (nextChapter >= numChapters)
        playNextFile();
    else
        navigateToChapter(nextChapter);
}

void PlaybackManager::navigateToPrevChapter()
{
    int64_t chapter = mpvWidget_->chapter();
    if (chapter > 0)
        navigateToChapter(std::max((int64_t)0, chapter - 1));
    else
        playPrevFile();
}

void PlaybackManager::playNextFile()
{
    QPair<QUuid, QUuid> next;
    next = playlistWindow_->getItemAfter(nowPlayingList, nowPlayingItem);
    QUrl url = playlistWindow_->getUrlOf(next.first, next.second);
    if (url.isEmpty()) {
        mpvWidget_->stopPlayback();
        nowPlaying_.clear();
        nowPlayingList = QUuid();
        nowPlayingItem = QUuid();
        playbackState_ = StoppedState;
        emit stateChanged(playbackState_);
        return;
    }
    startPlayWithUuid(url, next.first, next.second, false);
}

void PlaybackManager::playPrevFile()
{
    QUuid uuid = playlistWindow_->getItemBefore(nowPlayingList, nowPlayingItem);
    QUrl url = playlistWindow_->getUrlOf(nowPlayingList, uuid);
    if (url.isEmpty()) {
        mpvWidget_->stopPlayback();
        nowPlaying_.clear();
        nowPlayingItem = QUuid();
        playbackState_ = StoppedState;
        emit stateChanged(playbackState_);
        return;
    }
    startPlayWithUuid(url, nowPlayingList, uuid, false);
}

void PlaybackManager::repeatThisFile()
{
    startPlayWithUuid(nowPlaying_, nowPlayingList, nowPlayingItem, true);
}

void PlaybackManager::navigateToChapter(int64_t chapter)
{
    if (!mpvWidget_->setChapter(chapter)) {
        // Out-of-bounds chapter navigation request. i.e. unseekable chapter
        // from either past-the-end or invalid.  So stop playback and continue
        // on the next via the playback finished slot.
        mpvWidget_->setPaused(false);
        mpvWidget_->stopPlayback();
    }
}

void PlaybackManager::navigateToTime(double time)
{
    mpvWidget_->setTime(time);
}

void PlaybackManager::speedUp()
{
    setPlaybackSpeed(std::min(8.0, mpvSpeed * 2.0));
}

void PlaybackManager::speedDown()
{
    setPlaybackSpeed(std::max(0.125, mpvSpeed / 2.0));
}

void PlaybackManager::speedReset()
{
    setPlaybackSpeed(1.0);
}

void PlaybackManager::relativeSeek(bool forwards, bool small)
{
    mpvWidget_->seek((forwards ? 1 : -1) * (small ? 1 : 5), small);
}

void PlaybackManager::setPlaybackSpeed(double speed)
{
    mpvSpeed = speed;
    mpvWidget_->setSpeed(speed);
    mpvWidget_->showMessage(tr("Speed: %1%").arg(speed*100));
}

static QString findSecondById(QList<QPair<int64_t,QString>> list, int64_t id) {
    // this *should* return the string at id-1
    for (int i = 0; i < list.count(); ++i)
        if (list[i].first == id)
            return list[i].second;
    return QString();
}

void PlaybackManager::setAudioTrack(int64_t id)
{
    audioListSelected = findSecondById(audioList, id);
    mpvWidget_->setAudioTrack(id);
}

void PlaybackManager::setSubtitleTrack(int64_t id)
{
    subtitleListSelected = findSecondById(subtitleList, id);
    mpvWidget_->setSubtitleTrack(id);
}

void PlaybackManager::setVideoTrack(int64_t id)
{
    videoListSelected = findSecondById(videoList, id);
    mpvWidget_->setVideoTrack(id);
}

void PlaybackManager::setVolume(int64_t volume)
{
    mpvWidget_->setVolume(volume);
    mpvWidget_->showMessage(tr("Volume: %1%").arg(volume));
}

void PlaybackManager::setMute(bool muted)
{
    mpvWidget_->setMute(muted);
    mpvWidget_->showMessage(muted ? tr("Mute: on") : tr("Mute: off"));
}

void PlaybackManager::setPlaybackPlayTimes(int times)
{
    this->playbackPlayTimes = std::max(0, times);
}

void PlaybackManager::mpvw_playTimeChanged(double time)
{
    // in case the duration property is not available, update the play length
    // to indicate that the time is in fact available.
    if (mpvLength < time)
        mpvLength = time;
    emit timeChanged(time, mpvLength);
}

void PlaybackManager::mpvw_playLengthChanged(double length)
{
    mpvLength = length;
}

void PlaybackManager::mpvw_playbackLoading()
{
    playbackState_ = BufferingState;
    emit stateChanged(playbackState_);
}

void PlaybackManager::mpvw_playbackStarted()
{
    playbackState_ = PlayingState;
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

    int extraTimes = playlistWindow_->extraPlayTimes(nowPlayingList, nowPlayingItem);
    playlistWindow_->setExtraPlayTimes(nowPlayingList, nowPlayingItem, extraTimes - 1);

    bool isRepeating = playbackPlayTimes < 1 || extraTimes > 0;
    if (isRepeating)
        repeatThisFile();
    else
        playNextFile();
}

void PlaybackManager::mpvw_mediaTitleChanged(QString title)
{
    emit titleChanged(title);
}

void PlaybackManager::mpvw_chapterDataChanged(QVariantMap metadata)
{
    emit chapterTitleChanged(metadata.value("title").toString());
}

void PlaybackManager::mpvw_chaptersChanged(QVariantList chapters)
{
    QList<QPair<double,QString>> list;
    for (QVariant v : chapters) {
        QMap<QString, QVariant> node = v.toMap();
        QString text = QString("[%1] - %2").arg(
                toDateFormat(node["time"].toDouble()),
                node["title"].toString());
        QPair<double,QString> item(node["time"].toDouble(), text);
        list.append(item);
    }
    numChapters = list.count();
    emit chaptersAvailable(list);
}

void PlaybackManager::mpvw_tracksChanged(QVariantList tracks)
{
    videoList.clear();
    audioList.clear();
    subtitleList.clear();
    QPair<int64_t,QString> item;

    auto str = [](QVariantMap map, QString key) {
        return map[key].toString();
    };
    auto formatter = [&str](QVariantMap track) {
        QString output;
        output.append(QString("%1: ").arg(str(track,"id")));
        if (track.contains("codec"))
            output.append(QString("[%1] ").arg(str(track,"codec")));
        if (track.contains("lang"))
            output.append(QString("%1 ").arg(str(track,"lang")));
        if (track.contains("title"))
            output.append(QString("- %1 ").arg(str(track,"title")));
        return output;
    };

    for (QVariant track : tracks) {
        QVariantMap t = track.toMap();
        item.first = t["id"].toLongLong();
        item.second = formatter(t);
        if (str(t,"type") == "video") {
            videoList.append(item);
        } else if (str(t,"type") == "audio") {
            audioList.append(item);
        } else if (str(t,"type") == "sub") {
            subtitleList.append(item);
        }
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
