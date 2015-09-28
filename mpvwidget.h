#ifndef MPVWIDGET_H
#define MPVWIDGET_H

#include <QWidget>
#include <QVariant>
#include <mpv/client.h>

class MpvWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MpvWidget(QWidget *parent = 0);
    ~MpvWidget();
    void showMessage(QString message);

    // These are straightforward calls to libmpv
    void fileOpen(QString filename);
    void stopPlayback();
    void stepBackward();
    void stepForward();

    // These are straightforward queries to libmpv
    int64_t chapter();
    bool setChapter(int64_t chapter);
    QString mediaTitle();
    void setMute(bool yes);
    void setPaused(bool yes);
    void setSpeed(double speed);
    void setTime(double position);
    void setAudioTrack(int64_t id);
    void setSubtitleTrack(int64_t id);
    void setVideoTrack(int64_t id);
    QString mpvVersion();
    void setVolume(int64_t volume);
    bool eofReached();

    // These query the tracked video state
    double playLength();
    double playTime();
    QSize videoSize();

signals:
    void fireMpvEvents();

    void playTimeChanged(double time);
    void playLengthChanged(double length);
    void playbackStarted();
    void pausedChanged(bool yes);
    void playbackFinished();
    void mediaTitleChanged(QString title);
    void chapterDataChanged(QVariantMap metadata);
    void chaptersChanged(QVariantList chapters);
    void tracksChanged(QVariantList tracks);
    void videoSizeChanged(QSize size);
    void fpsChanged(double fps);
    void avsyncChanged(double sync);
    void displayFramedropsChanged(int64_t count);
    void decoderFramedropsChanged(int64_t cout);

public slots:

private slots:
    void mpvEvents();

private:
    mpv_handle *mpv;
    QSize videoSize_;
    double playTime_;
    double playLength_;

    void handleMpvEvent(mpv_event *event);
    QString getPropertyString(const char *property);
};

#endif // MPVWIDGET_H
