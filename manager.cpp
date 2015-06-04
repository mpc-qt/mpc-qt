#include "manager.h"
#include "mainwindow.h"
#include "mpvwidget.h"
#include "helpers.h"

using namespace Helpers;


PlaybackManager::PlaybackManager(QObject *parent) :
    QObject(parent)
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
        connect(mpvWidget, &MpvWidget::playbackStarted,
                this, &PlaybackManager::mpvw_playbackStarted);
        connect(mpvWidget, &MpvWidget::pausedChanged,
                this, &PlaybackManager::mpvw_pausedChanged);
        connect(mpvWidget, &MpvWidget::playbackFinished,
                this, &PlaybackManager::mpvw_playbackFinished);
        connect(mpvWidget, &MpvWidget::mediaTitleChanged,
                this, &PlaybackManager::mpvw_mediaTitleChanged);
        connect(mpvWidget, &MpvWidget::chaptersChanged,
                this, &PlaybackManager::mpvw_chaptersChanged);
        connect(mpvWidget, &MpvWidget::tracksChanged,
                this, &PlaybackManager::mpvw_tracksChanged);
        connect(mpvWidget, &MpvWidget::videoSizeChanged,
                this, &PlaybackManager::mpvw_videoSizeChanged);
    }
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

void PlaybackManager::pausePlayer()
{

}

void PlaybackManager::unpausePlayer()
{

}

void PlaybackManager::stopPlayer()
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
    emit stateChanged(PlayingState);
}

void PlaybackManager::mpvw_pausedChanged(bool yes)
{
    emit stateChanged(yes ? PausedState : PlayingState);
}

void PlaybackManager::mpvw_playbackFinished()
{
    // TODO: call playlist management here instead
    emit stateChanged(StoppedState);
}

void PlaybackManager::mpvw_mediaTitleChanged(QString title)
{
    emit titleChanged(title);
}

void PlaybackManager::mpvw_chaptersChanged(QVariantList chapters)
{
    QList<QPair<int64_t,QString>> list;
    int64_t index = 0;
    for (QVariant v : chapters) {
        QMap<QString, QVariant> node = v.toMap();
        QString text = QString("[%1] - %2").arg(
                toDateFormat(node["time"].toDouble()),
                node["title"].toString());
        QPair<int64_t,QString> item(index, text);
        list.append(item);
        ++index;
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
