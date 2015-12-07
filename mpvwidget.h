#ifndef MPVWIDGET_H
#define MPVWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QVariant>
#include <mpv/client.h>
#include <mpv/opengl_cb.h>
#include <mpv/qthelper.hpp>

class QThread;
class MpvController;

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
    void setClientDebuggingMessages(bool yes);
    void setMpvLogLevel(QString level);

    // These query the tracked video state
    double playLength();
    double playTime();
    QSize videoSize();
    bool nnedi3Available();

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);

private:
    static void ctrl_update(void *ctx);
    void     setMpvPropertyVariant(QString name, QVariant value);
    QVariant getMpvPropertyVariant(QString name);
    void     setMpvOptionVariant(QString name, QVariant value);

signals:
    void ctrlCommand(QVariant params);
    void ctrlSetOptionVariant(QString name, QVariant value);
    void ctrlSetPropertyVariant(QString name, QVariant value);

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
    void ctrl_nnedi3Unavailable();
    void ctrl_mpvPropertyChanged(QString name, QVariant v);
    void ctrl_logMessage(QString message);
    void ctrl_unhandledMpvEvent(int eventLevel);
    void ctrl_videoSizeChanged(QSize size);
    void self_playTimeChanged(double playTime);
    void self_playLengthChanged(double playLength);
    void self_frameSwapped();
    void self_playbackStarted();
    void self_playbackFinished();

private:
    QThread *worker;
    MpvController *ctrl;
    mpv_opengl_cb_context *glMpv;

    QSize videoSize_;
    double playTime_;
    double playLength_;
    bool nnedi3Available_;

    bool drawLogo;
    QOpenGLTexture *logo;
    QRectF logoLocation;
    QString logoUrl;

    bool debugMessages;
};

class MpvErrorCode {
public:
    MpvErrorCode() : value(0) {};
    MpvErrorCode(int value) : value(value) {};
    MpvErrorCode(const MpvErrorCode &mec) : value(mec.value) {}
    ~MpvErrorCode() {}
    int errorcode() { return value; }
private:
    int value;
};
Q_DECLARE_METATYPE(MpvErrorCode)

// This controller attempts to shove as much libmpv related business off of
// the main thread.
class MpvController : public QObject
{

    Q_OBJECT
public:
    typedef QPair<const char*, mpv_format> MpvProperty;
    typedef QVector<MpvProperty> PropertyList;
    enum LogLevel { LogNone, LogFatal, LogError, LogWarn, LogInfo, LogStatus,
                    LogV, LogDebug, LogTrace, LogTerminalDefault };

    MpvController(QObject *parent = 0);
    ~MpvController();

signals:
    void nnedi3Unavailable();
    void durationChanged(int value);
    void positionChanged(int value);
    void mpvPropertyChanged(QString name, QVariant v);
    void logMessage(QString message);
    void unhandledMpvEvent(int eventNumber);
    void videoSizeChanged(QSize size);

public slots:
    void create();
    void observeProperties(const MpvController::PropertyList &properties);
    void setLogLevel(MpvController::LogLevel level);
    mpv_opengl_cb_context *mpvDrawContext();
    int setOptionVariant(QString name, const QVariant &value);
    void command(const QVariant &params);
    void setPropertyVariant(const QString &name, const QVariant &value);
    QVariant getPropertyVariant(const QString &name);
    void parseMpvEvents();

private:
    void handleMpvEvent(mpv_event *event);
    static void mpvWakeup(void *ctx);

    mpv::qt::Handle mpv;
    mpv_opengl_cb_context *glMpv;
    QSize lastVideoSize;
};


#endif // MPVWIDGET_H
