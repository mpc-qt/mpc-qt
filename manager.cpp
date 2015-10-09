#include "manager.h"
#include "mainwindow.h"
#include "mpvwidget.h"
#include "helpers.h"

using namespace Helpers;


PlaybackManager::PlaybackManager(QObject *parent) :
    QObject(parent), mpvSpeed(1.0)
{
    connect(this, &PlaybackManager::fireStartPlayingEvent,
            this, &PlaybackManager::mpvw_startPlaying,
            Qt::QueuedConnection);
}

void PlaybackManager::setMpvWidget(MpvWidget *mpvWidget, bool makeConnections)
{
    mpvWidget_ = mpvWidget;

    if (makeConnections) {
        connect(mpvWidget, &MpvWidget::playTimeChanged,
                this, &PlaybackManager::mpvw_playTimeChanged);
        connect(mpvWidget, &MpvWidget::playLengthChanged,
                this, &PlaybackManager::mpvw_playLengthChanged);
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
            this, &PlaybackManager::playlistw_itemDesired);
    connect(this, &PlaybackManager::nowPlayingChanged,
            playlistWindow, &PlaylistWindow::changePlaylistSelection);
}

void PlaybackManager::fireNowPlayingState()
{
    emit nowPlayingChanged(nowPlayingList, nowPlayingItem);
}

void PlaybackManager::startPlayWithUuid(QUrl what, QUuid playlistUuid,
                                        QUuid itemUuid)
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
    emit fireStartPlayingEvent(what, playlistUuid, itemUuid);
}

void PlaybackManager::openSeveralFiles(QList<QUrl> what)
{
    // For the moment, until we get a working playback ui, play the first file
    // TODO: Add the entire list to the quick playlist.
    bool playAfterAdd = playlistWindow_->isCurrentPlaylistEmpty() &&
            nowPlayingItem == QUuid();
    auto info = playlistWindow_->addToCurrentPlaylist(what);
    if (playAfterAdd) {
        startPlayWithUuid(what.front(), info.first, info.second);
    }
}

void PlaybackManager::openFile(QUrl what)
{
    auto info = playlistWindow_->urlToQuickPlaylist(what);
    startPlayWithUuid(what, info.first, info.second);
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

}

void PlaybackManager::playItem(QUuid playlist, QUuid item)
{

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
    navigateToChapter(mpvWidget_->chapter() + 1);
}

void PlaybackManager::navigateToPrevChapter()
{
    navigateToChapter(std::max(0l, mpvWidget_->chapter() - 1));
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

void PlaybackManager::setAudioTrack(int64_t id)
{
    mpvWidget_->setAudioTrack(id);
}

void PlaybackManager::setSubtitleTrack(int64_t id)
{
    mpvWidget_->setSubtitleTrack(id);
}

void PlaybackManager::setVideoTrack(int64_t id)
{
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

void PlaybackManager::mpvw_startPlaying(QUrl what, QUuid playlistUuid, QUuid itemUuid)
{
    mpvWidget_->fileOpen(what.toLocalFile());
    this->nowPlayingList = playlistUuid;
    this->nowPlayingItem = itemUuid;
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

void PlaybackManager::mpvw_playbackStarted()
{
    playbackState = PlayingState;
    emit stateChanged(playbackState);
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
    auto uuid = playlistWindow_->getItemAfter(nowPlayingList, nowPlayingItem);
    if (uuid.isNull()) {
        nowPlayingItem = QUuid();
        playbackState = StoppedState;
        emit stateChanged(playbackState);
    } else {
        auto url = playlistWindow_->getUrlOf(nowPlayingList, uuid);
        if (url.isEmpty()) {
            nowPlayingItem = QUuid();
            playbackState = StoppedState;
            emit stateChanged(playbackState);
        } else {
            nowPlayingItem = uuid;
            startPlayWithUuid(url, nowPlayingList, nowPlayingItem);
        }
    }
    fireNowPlayingState();
}

void PlaybackManager::mpvw_mediaTitleChanged(QString title)
{
    emit titleChanged(title);
}

void PlaybackManager::mpvw_chapterDataChanged(QVariantMap metadata)
{
    emit chapterTitleChanged(metadata.value("TITLE").toString());
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
    emit chaptersAvailable(list);
}

void PlaybackManager::mpvw_tracksChanged(QVariantList tracks)
{
    QList<QPair<int64_t,QString>> videoList;
    QList<QPair<int64_t,QString>> audioList;
    QList<QPair<int64_t,QString>> subtitleList;
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

void PlaybackManager::playlistw_itemDesired(QUuid playlistUuid, QUuid itemUuid)
{
    auto url = playlistWindow_->getUrlOf(playlistUuid, itemUuid);
    startPlayWithUuid(url, playlistUuid, itemUuid);
}
