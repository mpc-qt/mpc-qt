#include <QDebug>
#include "manager.h"
#include "mainwindow.h"
#include "mpvwidget.h"
#include "helpers.h"

using namespace Helpers;


PlaybackManager::PlaybackManager(QObject *parent) :
    QObject(parent), mpvSpeed(1.0), playbackPlayTimes(1)
{
    connect(this, &PlaybackManager::fireStartPlayingEvent,
            this, &PlaybackManager::mpvw_startPlaying,
            Qt::QueuedConnection);
}

void PlaybackManager::setMpvWidget(MpvWidget *mpvWidget, bool makeConnections)
{
    mpvWidget_ = mpvWidget;

    if (makeConnections) {
        connect(mpvWidget, &MpvWidget::mousePressed,
                this, &PlaybackManager::mpvw_mousePressed);

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
        connect(mpvWidget, &MpvWidget::playbackFinished,
                this, &PlaybackManager::mpvw_playbackFinished);
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

void PlaybackManager::fireNowPlayingState()
{
    emit nowPlayingChanged(nowPlayingList, nowPlayingItem);
}

void PlaybackManager::startPlayWithUuid(QUrl what, QUuid playlistUuid,
                                        QUuid itemUuid, bool isRepeating)
{
    if (what.isEmpty())
        return;
    // Mpv fires the playback finished state for the currently playing file
    // when we try to play a file open command while playback is still active,
    // thus triggering the playlist advancing code when we don't want to
    // advance.  This is because if we naively start a new file straight away,
    // the playlist advancing code will trigger *after* the command to play
    // the new file has been sent, and we assume we're at the end of the file
    // we literally started playing only moments ago.
    //
    // Therefore we have to try to force the playback to finish, then hope
    // that we receive the finished event before we start the next file.
    //
    // The other way to do this is to introduce a few data members which
    // indicate a sort-of "requested file change".  Which may introduce a
    // the problem of what do you do if it doesn't follow the usual code path.
    // (ouch!)
    if (playbackState != StoppedState) {
        // by setting the state to stopped here, when the finished playback
        // event is received, it will do nothing.
        playbackState = StoppedState;
        emit stateChanged(playbackState);
        mpvWidget_->stopPlayback();
    }
    emit fireStartPlayingEvent(what, playlistUuid, itemUuid, isRepeating);
}

void PlaybackManager::selectDesiredTracks()
{
    // search current tracks by mangled string of no id and no spaces
    auto mangle = [](QString s) {
        return QStringList(s.split(' ').mid(1)).join("");
    };
    auto findIdBySecond = [&](QList<QPair<int64_t,QString>> list,
                                   QString needle) {
        if (list.isEmpty() || (needle = mangle(needle)).isEmpty())
            return -1l;
        for (int i = 0; i < list.count(); i++) {
            if (mangle(list[i].second) == needle) {
                return list[i].first;
            }
        }
        return -1l;
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
    bool playAfterAdd = (playlistWindow_->isCurrentPlaylistEmpty() &&
            nowPlayingItem == QUuid()) || !playlistWindow_->isVisible();
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
    if (playbackState != StoppedState) {
        playbackState = StoppedState;
        emit stateChanged(playbackState);
        mpvWidget_->stopPlayback();
    }
    mpvWidget_->discFilesOpen(where.toLocalFile());
    nowPlayingItem = QUuid();
    nowPlayingList = QUuid();
    emit nowPlayingChanged(QUuid(), QUuid());
}

void PlaybackManager::playDisc(QUrl where)
{

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

}

void PlaybackManager::pausePlayer()
{
    mpvWidget_->setPaused(true);
}

void PlaybackManager::unpausePlayer()
{
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
        navigateToChapter(std::max(0l, chapter - 1));
    else
        playPrevFile();
}

void PlaybackManager::playNextFile()
{
    QUuid uuid = playlistWindow_->getItemAfter(nowPlayingList, nowPlayingItem);
    QUrl url = playlistWindow_->getUrlOf(nowPlayingList, uuid);
    if (url.isEmpty()) {
        nowPlaying_.clear();
        nowPlayingItem = QUuid();
        playbackState = StoppedState;
        emit stateChanged(playbackState);
    } else {
        nowPlayingItem = uuid;
        startPlayWithUuid(url, nowPlayingList, nowPlayingItem, false);
    }
}

void PlaybackManager::playPrevFile()
{
    QUuid uuid = playlistWindow_->getItemBefore(nowPlayingList, nowPlayingItem);
    if (uuid.isNull())
        return;
    QUrl url = playlistWindow_->getUrlOf(nowPlayingList, uuid);
    if (url.isEmpty())
        return;
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

void PlaybackManager::mpvw_mousePressed()
{
    if (playbackState == PlayingState)
        pausePlayer();
    else if (playbackState == PausedState)
        unpausePlayer();
}

void PlaybackManager::mpvw_startPlaying(QUrl what, QUuid playlistUuid,
                                        QUuid itemUuid, bool isRepeating)
{
    nowPlaying_ = what;
    mpvWidget_->fileOpen(what.isLocalFile() ? what.toLocalFile()
                                            : what.toString());
    this->nowPlayingList = playlistUuid;
    this->nowPlayingItem = itemUuid;
    if (!isRepeating) {
        // not repeating, so set playback count up
        playbackPlayTimesCount = 1;
    } else {
        // repeating, so it needs a addition
        playbackPlayTimesCount++;
    }
    fireNowPlayingState();
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
    playbackState = BufferingState;
    emit stateChanged(playbackState);
}

void PlaybackManager::mpvw_playbackStarted()
{
    playbackState = PlayingState;
    emit stateChanged(playbackState);
    emit playerSettingsRequested();
}

void PlaybackManager::mpvw_pausedChanged(bool yes)
{
    playbackState = yes ? PausedState : PlayingState;
    emit stateChanged(playbackState);
}

void PlaybackManager::mpvw_playbackFinished()
{
    if (playbackState == StoppedState)
        return; // the playback state change does not need to be processed

    bool isRepeating;
    isRepeating = playbackPlayTimes < 1 || playbackPlayTimesCount < playbackPlayTimes;
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
