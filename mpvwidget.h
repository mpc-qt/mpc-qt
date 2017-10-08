#ifndef MPVWIDGET_H
#define MPVWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QTimer>
#include <QVariant>
#include <QSet>
#include <functional>
#include <mpv/client.h>
#include <mpv/opengl_cb.h>
#include <mpv/qthelper.hpp>
#include "helpers.h"

class QThread;
class QTimer;
class MpvController;
class LogoDrawer;

class MpvWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit MpvWidget(QWidget *parent = 0, const QString &clientName = "mpv");
    ~MpvWidget();
    QList<AudioDevice> audioDevices();
    QStringList supportedProtocols();

    void showMessage(QString message);
    void showStatsPage(int page);
    int cycleStatsPage();

    void fileOpen(QString filename);
    void discFilesOpen(QString path);
    void stopPlayback();
    void stepBackward();
    void stepForward();
    void seek(double amount, bool exact);
    void screenshot(const QString &fileName, Helpers::ScreenshotRender render);
    void setMouseHideTime(int msec);
    void setLogoUrl(const QString &filename);
    void setLogoBackground(const QColor &color);
    void setSubFile(QString filename);
    void addSubFile(QString filename);

    int64_t chapter();
    bool setChapter(int64_t chapter);
    QString mediaTitle();
    void setMute(bool yes);
    void setPaused(bool yes);
    void setSpeed(double speed);
    void setTime(double position);
    void setTimeSync(double position);
    void setLoopPoints(double first, double end);
    void setAudioTrack(int64_t id);
    void setSubtitleTrack(int64_t id);
    void setVideoTrack(int64_t id);
    void setDrawLogo(bool yes);
    QString mpvVersion();
    void setVolume(int64_t volume);
    bool eofReached();
    void setClientDebuggingMessages(bool yes);
    void setMpvLogLevel(QString logLevel);

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
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);

private:
    static void ctrl_update(void *ctx);
    void setMpvPropertyVariant(QString name, QVariant value);
    void setMpvOptionVariant(QString name, QVariant value);
    void showCursor();
    void hideCursor();

signals:
    void ctrlCommand(QVariant params);
    void ctrlSetOptionVariant(QString name, QVariant value);
    void ctrlSetPropertyVariant(QString name, QVariant value);
    void ctrlSetLogLevel(QString level);

    void audioDeviceList(const QList<AudioDevice> audioDevices);

    void playTimeChanged(double time);
    void playLengthChanged(double length);
    void seekableChanged(bool yes);
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
    void fileNameChanged(QString filename);
    void fileFormatChanged(QString format);
    void fileSizeChanged(int64_t size);
    void fileCreationTimeChanged(int64_t secsSinceEpoch);
    void filePathChanged(QString path);
    void playlistChanged(QVariantList playlist);

    void logoSizeChanged(QSize size);

    void mouseMoved(int x, int y);
    void mousePress(int x, int y);

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
    void self_audioDeviceList(const QVariantList &list);
    void self_mouseMoved();
    void hideTimer_timeout();

private:
    QThread *worker = nullptr;
    MpvController *ctrl = nullptr;
    mpv_opengl_cb_context *glMpv = nullptr;
    QTimer *hideTimer = nullptr;

    QVariantMap cachedState;
    QSize videoSize_;
    double playTime_ = 0.0;
    double playLength_ = 0.0;
    int glWidth = 0, glHeight = 0;

    int shownStatsPage = 0;
    bool drawLogo = true;
    LogoDrawer *logo = nullptr;

    bool loopImages = true;

    bool debugMessages = false;
};



class MpvErrorCode {
public:
    MpvErrorCode() {};
    MpvErrorCode(int value) : value(value) {};
    MpvErrorCode(const MpvErrorCode &mec) : value(mec.value) {}
    ~MpvErrorCode() {}
    int errorcode() { return value; }
private:
    int value = 0;
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
    QStringList protocolList();
    int64_t timeMicroseconds();
    unsigned long apiVersion();

    void setLogLevel(QString logLevel);
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
    mpv_opengl_cb_context *glMpv = nullptr;
    QStringList protocolList_;
    QSize lastVideoSize = QSize(0,0);

    QTimer *throttler = nullptr;
    QSet<QString> throttledProperties;
    typedef QMap<QString,QPair<QVariant,uint64_t>> ThrottledValueMap;
    ThrottledValueMap throttledValues;
};

#endif // MPVWIDGET_H
