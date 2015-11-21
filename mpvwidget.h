#ifndef MPVWIDGET_H
#define MPVWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QVariant>
#include <mpv/client.h>
#include <mpv/opengl_cb.h>
#include <mpv/qthelper.hpp>

class MpvWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit MpvWidget(QWidget *parent = 0);
    ~MpvWidget();
    void showMessage(QString message);

    // These are straightforward calls to libmpv
    void fileOpen(QString filename);
    void discFilesOpen(QString path);
    void stopPlayback();
    void stepBackward();
    void stepForward();

    // These are straightforward queries to libmpv
    void setVOCommandLine(QString cmdline);

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
    void setSubsAreGray(bool yes);
    void setFramedropMode(QString mode);
    void setDecoderDropMode(QString mode);
    void setDisplaySyncMode(QString mode);
    void setAudioDropSize(double size);
    void setMaximumAudioChange(double change);
    void setMaximumVideoChange(double change);

    // These query the tracked video state
    double playLength();
    double playTime();
    QSize videoSize();

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);

private:
    static void mpvw_update(void *ctx);
    QString getPropertyString(const char *property);
    void handleMpvEvent(mpv_event *event);

signals:
    void playTimeChanged(double time);
    void playLengthChanged(double length);
    void playbackLoading();
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

private slots:
    void mpvEvents();
    void self_frameSwapped();
    void self_playbackStarted();
    void self_playbackFinished();

private:
    mpv::qt::Handle mpv;
    mpv_opengl_cb_context *glMpv;
    QSize videoSize_;
    double playTime_;
    double playLength_;

    bool drawLogo;
    QOpenGLTexture *logo;
    QRectF logoLocation;
    QString logoUrl;
};

#endif // MPVWIDGET_H
