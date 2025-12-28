#ifndef MPVWIDGET_H
#define MPVWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLTexture>
#include <QTimer>
#include <QVariant>
#include <QSet>
#include <QMap>
#include <functional>
#include <mpv/client.h>
#include <mpv/render.h>
#include <mpv/render_gl.h>
#include "qthelper.hpp"
#include "helpers.h"

struct Chapter {
    double time;
    QString title;
};

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

    using PropertyDispatchFunction = std::function<void (MpvObject *, bool, const QVariant &)>;
    using PropertyDispatchMap = QMap<QString, PropertyDispatchFunction>;
public:
    explicit MpvObject(QObject *owner, const QString &clientName = "mpv");
    ~MpvObject();

    void setHostLayout(QLayout *hostLayout);
    void setHostWindow(QMainWindow *hostWindow);
    void setWidgetType(Helpers::MpvWidgetType widgetType, MpvWidgetInterface *customWidget = nullptr);

    QString mpvVersion();
    QString ffmpegVersion();
    MpvController *controller();
    QWidget *mpvWidget();

    QList<AudioDevice> audioDevices();
    QStringList supportedProtocols();

    void showMessage(QString message);
    void showStatsPage(int page);
    int cycleStatsPage();
    int selectedStatsPage();

    void urlOpen(QUrl url);
    void fileOpen(QString filename, bool replaceMpvPlaylist = true);
    void discFilesOpen(QString path);
    void stopPlayback();
    void stepBackward();
    void stepForward();
    void seek(double amount, bool exact);
    void screenshot(const QString &fileName, Helpers::ScreenshotRender render);
    void setMouseHideTime(int msec);
    void setLogoUrl(const QString &filename);
    void setLogoBackground(const QColor &color);
    void setAudioFilters(const QList<QPair<QString, QString>> &filtersList);
    void setVideoFilters(const QList<QPair<QString, QString>> &filtersList);
    QString formatFiltersList(const QList<QPair<QString, QString>> &filtersList);
    void setSubFile(QString filename);
    void addSubFile(QString filename);
    void setSubtitlesDelay(int subDelayStep);
    void moveSubtitlesVertically(int diff);
    void setVideoAspect(double aspectDiff);
    void setVideoAspectPreset(double aspect);
    void disableVideoAspect(bool yes);
    void setPanScan(double panScan);
    void setVideoZoom(double zoom);
    void setVideoWidthScale(double scale);
    void setVideoHeightScale(double scale);
    void setVideoPanX(double value);
    void setVideoPanY(double value);
    void setVideoRotate(int angle);
    void setVideoFlip(bool flip);

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
    void setClientDebuggingMessages(bool yes);
    void setMpvLogLevel(QString logLevel);
    void setSendKeyEvents(bool enabled);
    void setSendMouseEvents(bool enabled);

    double playLength();
    double playTime();
    QSize videoSize();
    bool clientDebuggingMessages();

    void setCachedMpvOption(const QString &option, const QVariant &value);
    void setUncachedMpvOption(const QString &option, const QVariant &value);
    QVariant blockingMpvCommand(const QVariant &params);
    QVariant blockingSetMpvPropertyVariant(QString name, const QVariant &value);
    QVariant blockingSetMpvOptionVariant(QString name, const QVariant &value);
    QVariant getMpvPropertyVariant(QString name);

signals:
    void ctrlContinueHook(uint64_t mpvId);
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
    void eofReachedChanged(QString eof);
    void playbackFinished();
    void playbackIdling();
    void mediaTitleChanged(QString title);
    void metaDataChanged(QVariantMap metadata);
    void chapterTitleChanged(QString title);
    void chaptersChanged(QVariantList chapters);
    void tracksChanged(QVariantList tracks);
    void videoSizeChanged(QSize size);
    void fpsChanged(double fps);
    void avsyncChanged(double sync);
    void displayFramedropsChanged(int64_t count);
    void decoderFramedropsChanged(int64_t cout);
    void audioBitrateChanged(double bitrate);
    void videoBitrateChanged(double bitrate);
    void aspectChanged(double newAspect);
    void aspectNameChanged(QString newAspectName);
    void fileNameChanged(QString filename);
    void fileFormatChanged(QString format);
    void fileSizeChanged(int64_t size);
    void filePathChanged(QString path);
    void subTextChanged(QString subText);
    void hwdecCurrentChanged(QString hwdecCurrent);
    void playlistChanged(QVariantList playlist);

    void audioTrackSet(int64_t id);
    void subtitleTrackSet(int64_t id);
    void videoTrackSet(int64_t id);

    void logoSizeChanged(QSize size);

    void mouseMoved(int x, int y);
    void mousePress(int x, int y, int btn);
    void mouseReleased();
    void keyPress(int key);
    void keyRelease(int key);

private:
    void setMpvPropertyVariant(QString name, const QVariant &value);
    void setMpvOptionVariant(QString name, const QVariant &value);
    void showCursor();
    void hideCursor();

private slots:
    void ctrl_mpvPropertyChanged(QString name, const QVariant &v);
    void ctrl_hookEvent(QString name, uint64_t selfId, uint64_t mpvId);
    void ctrl_unhandledMpvEvent(int eventLevel);
    void ctrl_videoSizeChanged(QSize size);
    void self_playTimeChanged(double playTime);
    void self_playLengthChanged(double playLength);
    void self_chapterChanged(int64_t chapter);
    void self_metadata(QVariantMap metadata);
    void self_audioDeviceList(const QVariantList &list);
    void hideTimer_timeout();
    void self_aspectChanged(double newAspect);

    void self_mouseMoved(int x, int y);
    void self_mousePress(int x, int y, int btn);
    void self_keyPress(int key);
    void self_keyRelease(int key);

private:
    static constexpr char logModule[] = "mpvobject";
    static PropertyDispatchMap propertyDispatch;

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
    int64_t chapter_ = 0;
    double aspect = 0.0;

    int shownStatsPage = 0;
    bool loopImages = true;
    bool debugMessages = false;

    bool sendMouseEvents = false;
    bool sendKeyEvents = false;
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

    QWidget *self() override;
    void initMpv() override;
    void setLogoUrl(const QString &filename) override;
    void setLogoBackground(const QColor &color) override;
    void setDrawLogo(bool yes) override;
    static void *get_proc_address(void *ctx, const char *name);

signals:
    void mouseMoved(int x, int y);
    void mousePress(int x, int y);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    static void render_update(void *ctx);

private slots:
    void maybeUpdate();
    void self_frameSwapped();
    void self_playbackStarted();
    void self_playbackFinished();

private:
    static constexpr char logModule[] = "glwidget";
    mpv_render_context *render = nullptr;
    LogoDrawer *logo = nullptr;
    bool drawLogo = true;
    int glWidth = 0;
    int glHeight = 0;
    bool windowDragging = false;
    int resizeEdge = 0;
    QPointF mousePressPosition;
};


// FIXME: implement MpvVulkanCbWidget
using MpvVulkanCbWidget = MpvGlWidget;

// FIXME: implement MpvEmbedWidget
using MpvEmbedWidget = MpvGlWidget;






class MpvErrorCode {
public:
    MpvErrorCode() = default;
    explicit MpvErrorCode(int value) : value(value) {}
    MpvErrorCode(const MpvErrorCode &mec) = default;
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
    using Callback = std::function<void (QVariant)>;
    explicit MpvCallback(const Callback &callback, QObject *owner = nullptr);
public slots:
    void reply(const QVariant &value);
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
    using PropertyList = QVector<MpvProperty>;
    struct MpvOption {
        QString name;
        QVariant value;
    };
    using OptionList = QVector<MpvOption>;

    explicit MpvController(QObject *parent = nullptr);
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
    void stop();
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
    static constexpr char logModule[] =  "mpvctrl";
    void setThrottledProperty(const QString &name, const QVariant &v, uint64_t userData);
    void flushProperties();
    void handleMpvEvent(mpv_event *event);
    static void mpvWakeup(void *ctx);

    mpv::qt::Handle mpv;
    QStringList protocolList_;
    QSize lastVideoSize = QSize(0,0);

    QTimer *throttler = nullptr;
    QSet<QString> throttledProperties;
    using ThrottledValueMap = QMap<QString, QPair<QVariant, uint64_t>>;
    ThrottledValueMap throttledValues;

    int shownStatsPage = 0;
};

#endif // MPVWIDGET_H
