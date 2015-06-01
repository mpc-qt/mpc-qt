#include "manager.h"

PlaybackManager::PlaybackManager(QObject *parent) :
    QObject(parent), mainWindow_(NULL)
{
}

void PlaybackManager::setMainWindow(MainWindow *mainWindow)
{
    mainWindow_ = mainWindow;
}

void PlaybackManager::setMpvWidget(MpvWidget *mpvWidget, bool makeConnections)
{
    mpvWidget_ = mpvWidget;
}

void PlaybackManager::openFiles(QList<QUrl> what)
{

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

void PlaybackManager::pause()
{

}

void PlaybackManager::unpause()
{

}

void PlaybackManager::stop()
{

}

void PlaybackManager::stepBackward()
{

}

void PlaybackManager::stepForward()
{

}

void PlaybackManager::navigateToNextChapter()
{

}

void PlaybackManager::navigateToPrevChapter()
{

}

void PlaybackManager::navigateToChapter(int64_t chapter)
{

}

void PlaybackManager::navigateToTime(double time)
{

}

void PlaybackManager::speedUp()
{

}

void PlaybackManager::speedDown()
{

}

void PlaybackManager::setPlaybackSpeed(double speed)
{

}

void PlaybackManager::setAudioTrack(int64_t id)
{

}

void PlaybackManager::setSubtitleTrack(int64_t id)
{

}

void PlaybackManager::setVideoTrack(int64_t id)
{

}

void PlaybackManager::setVolume(int64_t volume)
{

}

void PlaybackManager::setMute(bool muted)
{

}

void PlaybackManager::mpvw_playTimeChanged(double time)
{

}

void PlaybackManager::mpvw_playLengthChanged(double length)
{

}

void PlaybackManager::mpvw_playbackStarted()
{

}

void PlaybackManager::mpvw_pausedChanged(bool yes)
{

}

void PlaybackManager::mpvw_playbackFinished()
{

}

void PlaybackManager::mpvw_mediaTitleChanged(QString title)
{

}

void PlaybackManager::mpvw_chaptersChanged(QVariantList chapters)
{

}

void PlaybackManager::mpvw_tracksChanged(QVariantList tracks)
{

}

void PlaybackManager::mpvw_videoSizeChanged(QSize size)
{

}
