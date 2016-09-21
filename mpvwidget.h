#ifndef MPVWIDGET_H
#define MPVWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QVariant>
#include <QSet>
#include <functional>
#include <mpv/client.h>
#include <mpv/opengl_cb.h>
#include <mpv/qthelper.hpp>

class QThread;
class QTimer;
class MpvController;
class LogoDrawer;

class MpvWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit MpvWidget(QWidget *parent = 0);
    ~MpvWidget();
    void showMessage(QString message);

    void fileOpen(QString filename);
    void discFilesOpen(QString path);
    void stopPlayback();
    void stepBackward();
    void stepForward();
    void seek(double amount, bool exact);
    void screenshot(const QString &fileName, bool subtitles);
    void setLogoUrl(const QString &filename);
    void setLoopImages(bool yes);

    int64_t chapter();
    bool setChapter(int64_t chapter);
    QString mediaTitle();
    void setMute(bool yes);
    void setPaused(bool yes);
    void setSpeed(double speed);
    void setTime(double position);
    void setLoopPoints(double first, double end);
    void setAudioTrack(int64_t id);
    void setSubtitleTrack(int64_t id);
    void setVideoTrack(int64_t id);
    void setDrawLogo(bool yes);
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
    void setScreenshotFormat(QString format);
    void setScreenshotJpegQuality(int64_t value);
    void setScreenshotJpegSmooth(int64_t value);
    void setScreenshotJpegSourceChroma(bool yes);
    void setScreenshotPngCompression(int64_t value);
    void setScreenshotPngFilter(int64_t value);
    void setScreenshotPngColorspace(bool yes);
    void setClientDebuggingMessages(bool yes);
    void setMpvLogLevel(QString level);

    MpvController *controller();
    double playLength();
    double playTime();
    QSize videoSize();

    void setCachedMpvOption(const QString &option, const QVariant &value);
    QVariant blockingMpvCommand(QVariant params);
    QVariant blockingSetMpvPropertyVariant(QString name, QVariant value);
    QVariant blockingSetMpvOptionVariant(QString name, QVariant value);
    QVariant getMpvPropertyVariant(QString name);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);

private:
    static void ctrl_update(void *ctx);
    void     setMpvPropertyVariant(QString name, QVariant value);
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
    void playbackIdling();
    void mediaTitleChanged(QString title);
    void metaDataChanged(QVariantMap metadata);
    void chapterDataChanged(QVariantMap metadata);
    void chaptersChanged(QVariantList chapters);
    void tracksChanged(QVariantList tracks);
    void videoSizeChanged(QSize size);
    void fpsChanged(double fps);
    void avsyncChanged(double sync);
    void displayFramedropsChanged(int64_t count);
    void decoderFramedropsChanged(int64_t cout);
    void audioBitrateChanged(double bitrate);
    void videoBitrateChanged(double bitrate);
    void playlistChanged(QVariantList playlist);

    void logoSizeChanged(QSize size);

private slots:
    void maybeUpdate();
    void ctrl_mpvPropertyChanged(QString name, QVariant v);
    void ctrl_logMessage(QString message);
    void ctrl_clientMessage(uint64_t id, const QStringList &args);
    void ctrl_unhandledMpvEvent(int eventLevel);
    void ctrl_videoSizeChanged(QSize size);
    void self_playTimeChanged(double playTime);
    void self_playLengthChanged(double playLength);
    void self_frameSwapped();
    void self_playbackStarted();
    void self_playbackFinished();
    void self_metadata(QVariantMap metadata);

private:
    QThread *worker;
    MpvController *ctrl;
    mpv_opengl_cb_context *glMpv;
    QVariantMap cachedState;

    QSize videoSize_;
    double playTime_;
    double playLength_;
    int glWidth, glHeight;

    bool drawLogo;
    LogoDrawer *logo;

    bool loopImages;

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



// This wraps a lambda so that it is invoked in the calling thread. i.e. use
// QMetaObject::invokeMethod on the controller's async functions to pass
// through this object, like this:
//    getPropertyVariantAsync("xyz", new MpvCallback([](const QVariant &v) {
//        ...
//    }));
class MpvCallback : public QObject {
    Q_OBJECT
public:
    typedef std::function<void(QVariant)> Callback;
    explicit MpvCallback(const Callback &callback, QObject *owner = 0);
public slots:
    void reply(QVariant value);
private:
    Callback callback;
};



// This controller attempts to shove as much libmpv related business off of
// the main thread.
class MpvController : public QObject
{
    Q_OBJECT
public:
    struct MpvProperty {
        QString name;
        uint64_t userData;
        mpv_format format;
        MpvProperty(const QString &name, uint64_t userData, mpv_format format)
            : name(name), userData(userData), format(format) {}
    };
    typedef QVector<MpvProperty> PropertyList;
    enum LogLevel { LogNone, LogFatal, LogError, LogWarn, LogInfo, LogV,
                    LogDebug, LogTrace, LogTerminalDefault };

    MpvController(QObject *parent = 0);
    ~MpvController();

signals:
    void durationChanged(int value);
    void positionChanged(int value);
    void mpvPropertyChanged(QString name, QVariant v, uint64_t userData);
    void logMessage(QString message);
    void clientMessage(uint64_t id, QStringList args);
    void videoSizeChanged(QSize size);
    void unhandledMpvEvent(int eventNumber);

public slots:
    void create(bool video = true, bool audio = true);
    void addHook(const QString &name, int id);
    int observeProperties(const MpvController::PropertyList &properties,
                          const QSet<QString> &throttled = QSet<QString>());
    int unobservePropertiesById(const QSet<uint64_t> &ids);
    void setThrottleTime(int msec);

    QString clientName();
    int64_t timeMicroseconds();
    int64_t apiVersion();

    void setLogLevel(MpvController::LogLevel level);
    mpv_opengl_cb_context *mpvDrawContext();

    int setOptionVariant(QString name, const QVariant &value);
    QVariant command(const QVariant &params);
    int setPropertyVariant(const QString &name, const QVariant &value);
    QVariant getPropertyVariant(const QString &name);
    int setPropertyString(const QString &name, const QString &value);
    QString getPropertyString(const QString &name);

    void commandAsync(const QVariant &params, MpvCallback *callback);
    void setPropertyVariantAsync(const QString &name, const QVariant &value, MpvCallback *callback);
    void getPropertyVariantAsync(const QString &name, MpvCallback *callback);

    void parseMpvEvents();

private:
    void setThrottledProperty(const QString &name, const QVariant &v, uint64_t userData);
    void flushProperties();
    void handleMpvEvent(mpv_event *event);
    static void mpvWakeup(void *ctx);

    mpv::qt::Handle mpv;
    mpv_opengl_cb_context *glMpv;
    QSize lastVideoSize;

    QTimer *throttler;
    QSet<QString> throttledProperties;
    QMap<QString,QPair<QVariant,uint64_t>> throttledValues;
};

#endif // MPVWIDGET_H
