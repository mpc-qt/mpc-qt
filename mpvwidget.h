#ifndef MPVWIDGET_H
#define MPVWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QTimer>
#include <QVariant>
#include <QSet>
#include <functional>
#include <mpv/client.h>
//#include <mpv/opengl_cb.h>
#include <mpv/qthelper.hpp>
#include <mpv/render.h>
#include <mpv/render_gl.h>
#include "helpers.h"

class QLayout;
class QMainWindow;
class QThread;
class QTimer;
class MpvWidgetInterface;
class MpvController;
class LogoDrawer;

class MpvObject : public QObject
{
    Q_OBJECT
public:
    explicit MpvObject(QObject *owner, const QString &clientName = "mpv");
    ~MpvObject();

    void setHostLayout(QLayout *hostLayout);
    void setHostWindow(QMainWindow *hostWindow);
    void setWidgetType(Helpers::MpvWidgetType widgetType);

    QString mpvVersion();
    MpvController *controller();
    QWidget *mpvWidget();

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
    void setVolume(int64_t volume);
    bool eofReached();
    void setClientDebuggingMessages(bool yes);
    void setMpvLogLevel(QString logLevel);

    double playLength();
    double playTime();
    QSize videoSize();
    bool clientDebuggingMessages();

    void setCachedMpvOption(const QString &option, const QVariant &value);
    QVariant blockingMpvCommand(QVariant params);
    QVariant blockingSetMpvPropertyVariant(QString name, QVariant value);
    QVariant blockingSetMpvOptionVariant(QString name, QVariant value);
    QVariant getMpvPropertyVariant(QString name);

signals:
    void ctrlContinueHook(int mpvId);
    void ctrlCommand(QVariant params);
    void ctrlSetOptionVariant(QString name, QVariant value);
    void ctrlSetPropertyVariant(QString name, QVariant value);
    void ctrlSetLogLevel(QString level);
    void ctrlShowStats(int page);

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

private:
    void setMpvPropertyVariant(QString name, QVariant value);
    void setMpvOptionVariant(QString name, QVariant value);
    void showCursor();
    void hideCursor();

private slots:
    void ctrl_mpvPropertyChanged(QString name, QVariant v);
    void ctrl_hookEvent(QString name, uint64_t selfId, uint64_t mpvId);
    void ctrl_unhandledMpvEvent(int eventLevel);
    void ctrl_videoSizeChanged(QSize size);
    void self_playTimeChanged(double playTime);
    void self_playLengthChanged(double playLength);
    void self_metadata(QVariantMap metadata);
    void self_audioDeviceList(const QVariantList &list);
    void hideTimer_timeout();

    void self_mouseMoved();

private:
    Helpers::MpvWidgetType widgetType = Helpers::NullWidget;
    QLayout *hostLayout = nullptr;
    QMainWindow *hostWindow = nullptr;
    MpvController *ctrl = nullptr;
    MpvWidgetInterface *widget = nullptr;

    QThread *worker = nullptr;
    QTimer *hideTimer = nullptr;

    QVariantMap cachedState;
    QSize videoSize_;
    double playTime_ = 0.0;
    double playLength_ = 0.0;

    int shownStatsPage = 0;
    bool loopImages = true;
    bool debugMessages = false;
};

class MpvWidgetInterface
{
public:
    explicit MpvWidgetInterface(MpvObject *object);
    virtual ~MpvWidgetInterface();
    void setController(MpvController *controller);

    virtual QWidget *self() = 0;
    virtual void initMpv() = 0;
    virtual void setLogoUrl(const QString &filename);
    virtual void setLogoBackground(const QColor &color);
    virtual void setDrawLogo(bool yes);

protected:
    MpvObject *mpvObject = nullptr;
    MpvController *ctrl = nullptr;
};
Q_DECLARE_INTERFACE(MpvWidgetInterface, "cmdrkotori.mpc-qt.MpvWidgetInterface/1.0");


class MpvGlWidget : public QOpenGLWidget, public MpvWidgetInterface
{
    Q_OBJECT
    Q_INTERFACES(MpvWidgetInterface)

public:
    explicit MpvGlWidget(MpvObject *object, QWidget *parent = nullptr);
    ~MpvGlWidget();

    QWidget *self();
    void initMpv();
    void setLogoUrl(const QString &filename);
    void setLogoBackground(const QColor &color);
    void setDrawLogo(bool yes);

signals:
    void mouseMoved(int x, int y);
    void mousePress(int x, int y);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);

private:
    static void render_update(void *ctx);

private slots:
    void maybeUpdate();
    void self_frameSwapped();
    void self_playbackStarted();
    void self_playbackFinished();

private:
    mpv_render_context *render = nullptr;
    LogoDrawer *logo = nullptr;
    bool drawLogo = true;
    int glWidth = 0, glHeight = 0;
};


// FIXME: implement MpvVulkanCbWidget
typedef MpvGlWidget MpvVulkanCbWidget;

// FIXME: implement MpvEmbedWidget
typedef MpvGlWidget MpvEmbedWidget;






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
    struct MpvOption {
        QString name;
        QVariant value;
    };
    typedef QVector<MpvOption> OptionList;

    MpvController(QObject *parent = 0);
    ~MpvController();

signals:
    void durationChanged(int value);
    void positionChanged(int value);
    void mpvPropertyChanged(QString name, QVariant v, uint64_t userData);
    void logMessageByParts(QString prefix, QString level, QString msg);
    //void logMessage(QString message);
    void clientMessage(uint64_t id, QStringList args);
    void videoSizeChanged(QSize size);
    void hookEvent(QString hookName, uint64_t selfId, uint64_t mpvId);
    void unhandledMpvEvent(int eventNumber);

public slots:
    void create(const MpvController::OptionList &earlyOptions);
    mpv_render_context *createRenderContext(mpv_render_param *params);
    void destroyRenderContext(mpv_render_context *render);

    void addHook(const QString &name, uint64_t selfId);
    void continueHook(uint64_t mpvId);

    int observeProperties(const MpvController::PropertyList &properties,
                          const QSet<QString> &throttled = QSet<QString>());
    int unobservePropertiesById(const QSet<uint64_t> &ids);
    void setThrottleTime(int msec);

    QString clientName();
    QStringList protocolList();
    int64_t timeMicroseconds();
    unsigned long apiVersion();

    void setLogLevel(QString logLevel);
    void showStatsPage(int page);

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
    QStringList protocolList_;
    QSize lastVideoSize = QSize(0,0);

    QTimer *throttler = nullptr;
    QSet<QString> throttledProperties;
    typedef QMap<QString,QPair<QVariant,uint64_t>> ThrottledValueMap;
    ThrottledValueMap throttledValues;

    int shownStatsPage = 0;
};

#endif // MPVWIDGET_H
