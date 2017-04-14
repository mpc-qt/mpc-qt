// Portions of code in this module came from the examples directory in mpv's
// git repo.  Reworked by me.

#include <QtGlobal>
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
#include <QtX11Extras/QX11Info>
#endif
#include <QThread>
#include <QTimer>
#include <QOpenGLContext>
#include <QMouseEvent>
#include <QMetaObject>
#include <QDir>
#include <QDebug>
#include <cmath>
#include <stdexcept>
#include <mpv/qthelper.hpp>
#include "mpvwidget.h"
#include "helpers.h"

#ifndef Q_PROCESSOR_ARM
    #ifndef GLAPIENTRY
    // On Windows, GLAPIENTRY may sometimes conveniently go missing
    #define GLAPIENTRY __stdcall
    #endif
#endif
#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif



static const int HOOK_UNLOAD_CALLBACK_ID = 0xdeaddead;



static void* GLAPIENTRY glMPGetNativeDisplay(const char* name) {
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    if (!strcmp(name, "x11")) {
        return (void*)QX11Info::display();
    }
#elif defined(Q_OS_WIN)
    if (!strcmp(name, "IDirect3DDevice9")) {
        // Do something here ?
    }
#else
    Q_UNUSED(name);
#endif
    return NULL;
}

static void *get_proc_address(void *ctx, const char *name) {
    (void)ctx;
    auto glctx = QOpenGLContext::currentContext();
    if (!strcmp(name, "glMPGetNativeDisplay"))
        return (void*)glMPGetNativeDisplay;
    void *res = glctx ? (void*)glctx->getProcAddress(QByteArray(name)) : NULL;

#ifdef Q_OS_WIN32
    // QOpenGLContext::getProcAddress() in Qt 5.6 and below doesn't resolve all
    // core OpenGL functions, so fall back to Windows' GetProcAddress().
    if (!res) {
        HMODULE module = (HMODULE)QOpenGLContext::openGLModuleHandle();
        if (!module) {
            // QOpenGLContext::openGLModuleHandle() returns NULL when Qt isn't
            // using dynamic OpenGL. In this case, openGLModuleType() can be
            // used to determine which module to query.
            switch (QOpenGLContext::openGLModuleType()) {
            case QOpenGLContext::LibGL:
                module = GetModuleHandleW(L"opengl32.dll");
                break;
            case QOpenGLContext::LibGLES:
                module = GetModuleHandleW(L"libGLESv2.dll");
                break;
            }
        }
        if (module)
            res = (void*)GetProcAddress(module, name);
    }
#endif

    return res;
}

MpvWidget::MpvWidget(QWidget *parent, const QString &clientName) :
    QOpenGLWidget(parent), worker(NULL), ctrl(NULL), hideTimer(NULL),
    drawLogo(true), logo(NULL), loopImages(true)
{
    debugMessages = false;

    // Setup threads
    worker = new QThread();
    worker->start();

    // setup controller
    ctrl = new MpvController();
    ctrl->moveToThread(worker);

    // setup timer
    hideTimer = new QTimer(this);
    hideTimer->setSingleShot(true);
    hideTimer->setInterval(1000);

    // Wire the basic mpv functions to avoid littering the codebase with
    // QMetaObject::invokeMethod.  This way the compiler will catch our
    // typos rather than a runtime error.
    connect(this, &MpvWidget::ctrlCommand,
            ctrl, &MpvController::command, Qt::QueuedConnection);
    connect(this, &MpvWidget::ctrlSetOptionVariant,
            ctrl, &MpvController::setOptionVariant, Qt::QueuedConnection);
    connect(this, &MpvWidget::ctrlSetPropertyVariant,
            ctrl, &MpvController::setPropertyVariant, Qt::QueuedConnection);
    connect(this, &MpvWidget::ctrlSetLogLevel,
            ctrl, &MpvController::setLogLevel);

    // Wire up the event-handling callbacks
    connect(ctrl, &MpvController::mpvPropertyChanged,
            this, &MpvWidget::ctrl_mpvPropertyChanged, Qt::QueuedConnection);
    connect(ctrl, &MpvController::logMessage,
            this, &MpvWidget::ctrl_logMessage, Qt::QueuedConnection);
    connect(ctrl, &MpvController::clientMessage,
            this, &MpvWidget::ctrl_clientMessage, Qt::QueuedConnection);
    connect(ctrl, &MpvController::unhandledMpvEvent,
            this, &MpvWidget::ctrl_unhandledMpvEvent, Qt::QueuedConnection);
    connect(ctrl, &MpvController::videoSizeChanged,
            this, &MpvWidget::ctrl_videoSizeChanged, Qt::QueuedConnection);

    // Wire up the mouse and timer-related callbacks
    connect(this, &MpvWidget::mouseMoved,
            this, &MpvWidget::self_mouseMoved);
    connect(hideTimer, &QTimer::timeout,
            this, &MpvWidget::hideTimer_timeout);

    // Initialize mpv
    QMetaObject::invokeMethod(ctrl, "create", Qt::BlockingQueuedConnection);

    // grab a copy of the mpvGl draw context
    QMetaObject::invokeMethod(ctrl, "mpvDrawContext",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(mpv_opengl_cb_context *, glMpv));

    // ask mpv to make draw requests to us
    mpv_opengl_cb_set_update_callback(glMpv, MpvWidget::ctrl_update,
                                      (void *)this);

    // clean up objects when the worker thread is deleted
    connect(worker, &QThread::finished, ctrl, &MpvController::deleteLater);

    // Observe some properties
    MpvController::PropertyList options = {
        { "time-pos", 0, MPV_FORMAT_DOUBLE },
        { "pause", 0, MPV_FORMAT_FLAG },
        { "media-title", 0, MPV_FORMAT_STRING },
        { "chapter-metadata", 0, MPV_FORMAT_NODE },
        { "track-list", 0, MPV_FORMAT_NODE },
        { "chapter-list", 0, MPV_FORMAT_NODE },
        { "duration", 0, MPV_FORMAT_DOUBLE },
        { "estimated-vf-fps", 0, MPV_FORMAT_DOUBLE },
        { "avsync", 0, MPV_FORMAT_DOUBLE },
        { "frame-drop-count", 0, MPV_FORMAT_INT64 },
        { "decoder-frame-drop-count", 0, MPV_FORMAT_INT64 },
        { "audio-bitrate", 0, MPV_FORMAT_DOUBLE },
        { "video-bitrate", 0, MPV_FORMAT_DOUBLE },
        { "paused-for-cache", 0, MPV_FORMAT_FLAG },
        { "metadata", 0, MPV_FORMAT_NODE },
        { "audio-device-list", 0, MPV_FORMAT_NODE }
    };
    QSet<QString> throttled = {
        "time-pos", "avsync", "estimated-vf-fps", "frame-drop-count",
        "decoder-frame-drop-count", "audio-bitrate", "video-bitrate"
    };
    QMetaObject::invokeMethod(ctrl, "observeProperties",
                              Qt::BlockingQueuedConnection,
                              Q_ARG(const MpvController::PropertyList &, options),
                              Q_ARG(const QSet<QString> &, throttled));

    // Add hooks
    QMetaObject::invokeMethod(ctrl, "addHook",
                              Qt::BlockingQueuedConnection,
                              Q_ARG(QString, "on_unload"),
                              Q_ARG(int, HOOK_UNLOAD_CALLBACK_ID));

    emit ctrlSetOptionVariant("ytdl", "yes");
    emit ctrlSetOptionVariant("audio-client-name", clientName);

    connect(this, &QOpenGLWidget::frameSwapped,
            this, &MpvWidget::self_frameSwapped);
    connect(this, &MpvWidget::playbackStarted,
            this, &MpvWidget::self_playbackStarted);
    connect(this, &MpvWidget::playbackFinished,
            this, &MpvWidget::self_playbackFinished);
}

MpvWidget::~MpvWidget()
{
    if (glMpv) {
        makeCurrent();
        mpv_opengl_cb_set_update_callback(glMpv, NULL, NULL);
        mpv_opengl_cb_uninit_gl(glMpv);
    }
    if (logo) {
        delete logo;
        logo = NULL;
    }
    if (hideTimer) {
        delete hideTimer;
        hideTimer = NULL;
    }
    worker->deleteLater();
}

QList<AudioDevice> MpvWidget::audioDevices()
{
    return AudioDevice::listFromVList(getMpvPropertyVariant("audio-devices").toList());
}

void MpvWidget::showMessage(QString message)
{
    emit ctrlCommand(QVariantList({"show_text", message, "1000"}));
}

void MpvWidget::fileOpen(QString filename)
{
    emit ctrlCommand(QStringList({"loadfile", filename}));
    setPaused(false);
    setMouseHideTime(hideTimer->interval());
}

void MpvWidget::discFilesOpen(QString path) {
    QStringList entryList = QDir(path).entryList();
    if (entryList.contains("VIDEO_TS") || entryList.contains("AUDIO_TS")) {
        fileOpen(path + "/VIDEO_TS/VIDEO_TS.IFO");
    } else if (entryList.contains("BDMV") || entryList.contains("AACS")) {
        fileOpen("bluray://" + path);
    }
}

void MpvWidget::stopPlayback()
{
    emit ctrlCommand("stop");
}

void MpvWidget::stepBackward()
{
    emit ctrlCommand("frame_back_step");
}

void MpvWidget::stepForward()
{
    emit ctrlCommand("frame_step");
}

void MpvWidget::seek(double amount, bool exact)
{
    QVariantList payload({"seek", amount});
    if (exact)
        payload.append("exact");
    emit ctrlCommand(payload);
}

void MpvWidget::screenshot(const QString &fileName, bool subtitles)
{
    emit ctrlCommand(QStringList({"screenshot-to-file", fileName, subtitles ? "subtitles" : "video"}));
}

void MpvWidget::setMouseHideTime(int msec)
{
    hideTimer->stop();
    hideTimer->setInterval(msec);
    showCursor();
    if (msec > 0)
        hideTimer->start();
}

void MpvWidget::setLogoUrl(const QString &filename)
{
    makeCurrent();
    if (!logo) {
        logo = new LogoDrawer(this);
        connect(logo, &LogoDrawer::logoSize,
                this, &MpvWidget::logoSizeChanged);
    }
    logo->setLogoUrl(filename);
    logo->resizeGL(width(), height());
    if (drawLogo)
        update();
    doneCurrent();
}

void MpvWidget::setSubFile(QString filename)
{
    emit ctrlSetOptionVariant("sub-file", filename);
}

int64_t MpvWidget::chapter()
{
    return getMpvPropertyVariant("chapter").toLongLong();
}

bool MpvWidget::setChapter(int64_t chapter)
{
    // As this requires knowledge of mpv's return value, it cannot be
    // queued as a simple message.  The usual return values are:
    // MPV_ERROR_PROPERTY_UNAVAILABLE: unchaptered file
    // MPV_ERROR_PROPERTY_FORMAT: past-the-end value requested
    // MPV_ERROR_SUCCESS: success
    int r;
    QMetaObject::invokeMethod(ctrl, "setPropertyVariant",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(int, r),
                              Q_ARG(QString, "chapter"),
                              Q_ARG(QVariant, QVariant((qlonglong)chapter)));
    return r == MPV_ERROR_SUCCESS;
}

QString MpvWidget::mediaTitle()
{
    return getMpvPropertyVariant("media-title").toString();
}

void MpvWidget::setMute(bool yes)
{
    setMpvPropertyVariant("mute", yes);
}

void MpvWidget::setPaused(bool yes)
{
    setMpvPropertyVariant("pause", yes);
}

void MpvWidget::setSpeed(double speed)
{
    setMpvPropertyVariant("speed", speed);
}

void MpvWidget::setTime(double position)
{
    setMpvPropertyVariant("time-pos", position);
}

void MpvWidget::setLoopPoints(double first, double end)
{
    setMpvPropertyVariant("ab-loop-a",
                          first < 0 ? QVariant("no") : QVariant(first));
    setMpvPropertyVariant("ab-loop-b",
                          end < 0 ? QVariant("no") : QVariant(end));
}

void MpvWidget::setAudioTrack(int64_t id)
{
    setMpvPropertyVariant("aid", (long long)id);
}

void MpvWidget::setSubtitleTrack(int64_t id)
{
    setMpvPropertyVariant("sid", (long long)id);
}

void MpvWidget::setVideoTrack(int64_t id)
{
    setMpvPropertyVariant("vid", (long long)id);
}

void MpvWidget::setDrawLogo(bool yes)
{
    drawLogo = yes;
    update();
}

QString MpvWidget::mpvVersion()
{
    return getMpvPropertyVariant("mpv-version").toString();
}

void MpvWidget::setVolume(int64_t volume)
{
    setMpvPropertyVariant("volume", (long long)volume);
}

bool MpvWidget::eofReached()
{
    return getMpvPropertyVariant("eof-reached").toBool();
}

void MpvWidget::setClientDebuggingMessages(bool yes)
{
    debugMessages = yes;
}

void MpvWidget::setMpvLogLevel(QString logLevel)
{
    emit ctrlSetLogLevel(logLevel);
}

MpvController *MpvWidget::controller()
{
    return ctrl;
}

double MpvWidget::playLength()
{
    return playLength_;
}

double MpvWidget::playTime()
{
    return playTime_;
}

QSize MpvWidget::videoSize()
{
    return videoSize_;
}

void MpvWidget::setCachedMpvOption(const QString &option, const QVariant &value)
{
    if (cachedState.contains(option) && cachedState.value(option) == value)
        return;
    cachedState.insert(option, value);
    setMpvOptionVariant(option, value);
}

QVariant MpvWidget::blockingMpvCommand(QVariant params)
{
    QVariant v;
    QMetaObject::invokeMethod(ctrl, "command",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVariant, v),
                              Q_ARG(QVariant, params));
    return v;
}

QVariant MpvWidget::blockingSetMpvPropertyVariant(QString name, QVariant value)
{
    int v;
    QMetaObject::invokeMethod(ctrl, "setPropertyVariant",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(int, v),
                              Q_ARG(QString, name),
                              Q_ARG(QVariant, value));
    return v == MPV_ERROR_SUCCESS ? QVariant()
                                  : QVariant::fromValue(MpvErrorCode(v));
}

QVariant MpvWidget::blockingSetMpvOptionVariant(QString name, QVariant value)
{
    int v;
    QMetaObject::invokeMethod(ctrl, "setOptionVariant",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(int, v),
                              Q_ARG(QString, name),
                              Q_ARG(QVariant, value));
    return v == MPV_ERROR_SUCCESS ? QVariant()
                                  : QVariant::fromValue(MpvErrorCode(v));
}

QVariant MpvWidget::getMpvPropertyVariant(QString name)
{
    QVariant v;
    QMetaObject::invokeMethod(ctrl, "getPropertyVariant",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVariant, v),
                              Q_ARG(QString, name));
    return v;
}

void MpvWidget::initializeGL()
{
    if (mpv_opengl_cb_init_gl(glMpv, NULL, get_proc_address, NULL) < 0)
        throw std::runtime_error("[MpvWidget] cb init gl failed.");

    if (!logo)
        logo = new LogoDrawer(this);
}

void MpvWidget::paintGL()
{
    if (debugMessages)
        qDebug() << "paintGL";
    if (!drawLogo) {
        mpv_opengl_cb_draw(glMpv, defaultFramebufferObject(),
                           glWidth, -glHeight);
    } else {
        logo->paintGL(this);
    }
}

void MpvWidget::resizeGL(int w, int h)
{
    qreal r = devicePixelRatio();
    glWidth = w * r;
    glHeight = h * r;
    logo->resizeGL(width(),height());
}

void MpvWidget::mouseMoveEvent(QMouseEvent *event)
{
    emit mouseMoved(event->x(), event->y());
    QOpenGLWidget::mouseMoveEvent(event);
}

void MpvWidget::mousePressEvent(QMouseEvent *event)
{
    emit mousePress(event->x(), event->y());
    QOpenGLWidget::mousePressEvent(event);
}

void MpvWidget::setMpvPropertyVariant(QString name, QVariant value)
{
    if (debugMessages)
        qDebug() << "property set " << name << value;
    emit ctrlSetPropertyVariant(name, value);
}

void MpvWidget::setMpvOptionVariant(QString name, QVariant value)
{
    if (debugMessages)
        qDebug() << "option set " << name << value;
    emit ctrlSetOptionVariant(name, value);
}

void MpvWidget::showCursor()
{
    setCursor(Qt::ArrowCursor);
}

void MpvWidget::hideCursor()
{
    setCursor(Qt::BlankCursor);
}

void MpvWidget::maybeUpdate()
{
    if (window()->isMinimized()) {
        makeCurrent();
        paintGL();
        context()->swapBuffers(context()->surface());
        self_frameSwapped();
        doneCurrent();
    } else {
        update();
    }
}

void MpvWidget::ctrl_update(void *ctx)
{
    QMetaObject::invokeMethod(reinterpret_cast<MpvWidget*>(ctx), "maybeUpdate");
}

#define HANDLE_PROP_1(p, method, converter, dflt) \
    if (name == p) { \
        if (ok && v.canConvert<decltype(dflt)>()) \
            method(v.converter()); \
        else \
            method(dflt); \
        return; \
    }
#define HANDLE_PROP_2(p, method, converter, dflt) \
    if (name == p) { \
        if (ok) \
            method(v.converter()); \
        else \
            method(dflt); \
        return; \
    }

void MpvWidget::ctrl_mpvPropertyChanged(QString name, QVariant v)
{
    if (debugMessages)
        qDebug() << "property changed " << name << v;

    bool ok = v.type() < QVariant::UserType;
    HANDLE_PROP_1("time-pos", self_playTimeChanged, toDouble, -1.0);
    HANDLE_PROP_1("duration", self_playLengthChanged, toDouble, -1.0);
    HANDLE_PROP_1("pause", pausedChanged, toBool, true);
    HANDLE_PROP_1("media-title", mediaTitleChanged, toString, QString());
    HANDLE_PROP_1("chapter-metadata", chapterDataChanged, toMap, QVariantMap());
    HANDLE_PROP_1("chapter-list", chaptersChanged, toList, QVariantList());
    HANDLE_PROP_1("track-list", tracksChanged, toList, QVariantList());
    HANDLE_PROP_1("estimated-vf-fps", fpsChanged, toDouble, 0.0);
    HANDLE_PROP_1("avsync", avsyncChanged, toDouble, 0.0);
    HANDLE_PROP_1("frame-drop-count", displayFramedropsChanged, toLongLong, 0ll);
    HANDLE_PROP_1("decoder-frame-drop-count", decoderFramedropsChanged, toLongLong, 0ll);
    HANDLE_PROP_1("audio-bitrate", audioBitrateChanged, toDouble, 0.0);
    HANDLE_PROP_1("video-bitrate", videoBitrateChanged, toDouble, 0.0);
    HANDLE_PROP_1("metadata", self_metadata, toMap, QVariantMap());
    HANDLE_PROP_1("audio-device-list", self_audioDeviceList, toList, QVariantList());
}

void MpvWidget::ctrl_logMessage(QString message)
{
    qDebug() << message;
}

void MpvWidget::ctrl_clientMessage(uint64_t id, const QStringList &args)
{
    Q_UNUSED(id);
    if (args[1] == QString::number(HOOK_UNLOAD_CALLBACK_ID)) {
        QVariantList playlist = getMpvPropertyVariant("playlist").toList();
        if (playlist.count() > 1)
            playlistChanged(playlist);
        emit ctrlCommand(QStringList({"hook-ack", args[2]}));
    }
}

void MpvWidget::ctrl_unhandledMpvEvent(int eventLevel)
{
    switch(eventLevel) {
    case MPV_EVENT_START_FILE: {
        if (debugMessages)
            qDebug() << "start file";
        playbackLoading();
        break;
    }
    case MPV_EVENT_FILE_LOADED: {
        if (debugMessages)
            qDebug() << "file loaded";
        playbackStarted();
        break;
    }
    case MPV_EVENT_END_FILE: {
        if (debugMessages)
            qDebug() << "end file";
        playbackFinished();
        break;
    }
    case MPV_EVENT_IDLE: {
        if (debugMessages)
            qDebug() << "idling";
        playbackIdling();
        break;
    }
    case MPV_EVENT_SHUTDOWN: {
        if (debugMessages)
            qDebug() << "event shutdown";
        playbackFinished();
        break;
    }
    }
}

void MpvWidget::ctrl_videoSizeChanged(QSize size)
{
    videoSize_ = size;
    emit videoSizeChanged(videoSize_);
}

void MpvWidget::self_playTimeChanged(double playTime)
{
    playTime_ = playTime;
    emit playTimeChanged(playTime);
}

void MpvWidget::self_playLengthChanged(double playLength)
{
    playLength_ = playLength;
    emit playLengthChanged(playLength);
}

void MpvWidget::self_frameSwapped()
{
    if (!drawLogo)
        mpv_opengl_cb_report_flip(glMpv, 0);
}

void MpvWidget::self_playbackStarted()
{
    drawLogo = false;
}

void MpvWidget::self_playbackFinished()
{
    drawLogo = true;
    update();
}

void MpvWidget::self_metadata(QVariantMap metadata)
{
    QVariantMap map;
    for(QString key : metadata.keys()) {
        map.insert(key.toLower(), metadata[key]);
    }
    emit metaDataChanged(map);
}

void MpvWidget::self_audioDeviceList(const QVariantList &list)
{
    emit audioDeviceList(AudioDevice::listFromVList(list));
}

void MpvWidget::self_mouseMoved()
{
    if (hideTimer->interval() > 0)
        hideTimer->start();
    showCursor();
}

void MpvWidget::hideTimer_timeout()
{
    hideCursor();
}



MpvCallback::MpvCallback(const Callback &callback,
                         QObject *owner)
    : QObject(owner)
{
    this->callback = callback;
}

void MpvCallback::reply(QVariant value)
{
    callback(value);
    deleteLater();
}



MpvController::MpvController(QObject *parent) : QObject(parent),
    glMpv(NULL), lastVideoSize(0,0)
{
    throttler = new QTimer(this);
    connect(throttler, &QTimer::timeout,
            this, &MpvController::flushProperties);
    throttler->setInterval(1000/12);
    throttler->start();
}

MpvController::~MpvController()
{
    mpv_set_wakeup_callback(mpv, NULL, NULL);
    throttler->deleteLater();
}

void MpvController::create(bool video, bool audio)
{
    mpv = mpv::qt::Handle::FromRawHandle(mpv_create());
    if (!mpv)
        throw std::runtime_error("could not create mpv context");

    if (mpv_initialize(mpv) < 0)
        throw std::runtime_error("could not initialize mpv context");

    if (!audio) {
        setOptionVariant("ao", "null");
        setOptionVariant("no-audio", true);
    }
    if (!video) {
        // NOTE: this completely skips setting up the gl interface.
        setOptionVariant("vo", "null");
        setOptionVariant("no-video", true);
    } else {
        setOptionVariant("vo", "opengl-cb");
        glMpv = (mpv_opengl_cb_context *)mpv_get_sub_api(mpv, MPV_SUB_API_OPENGL_CB);
        if (!glMpv)
            throw std::runtime_error("OpenGL not compiled in");
    }
    mpv_set_wakeup_callback(mpv, MpvController::mpvWakeup, this);
}

void MpvController::addHook(const QString &name, int id)
{
    command(QStringList({"hook-add", name, QString::number(id), "0"}));
}

int MpvController::observeProperties(const MpvController::PropertyList &properties,
                                      const QSet<QString> &throttled)
{
    int rval = 0;
    foreach (const MpvProperty &item, properties)
        rval  = std::min(rval, mpv_observe_property(mpv, item.userData, item.name.toUtf8().data(), item.format));
    throttledProperties.unite(throttled);
    return rval;
}

int MpvController::unobservePropertiesById(const QSet<uint64_t> &ids)
{
    int rval = 0;
    foreach (uint64_t id, ids)
        rval = std::min(rval, mpv_unobserve_property(mpv, id));
    return rval;
}

void MpvController::setThrottleTime(int msec)
{
    throttler->setInterval(msec);
}

QString MpvController::clientName()
{
    return QString::fromUtf8(mpv_client_name(mpv));
}

int64_t MpvController::timeMicroseconds()
{
    return mpv_get_time_us(mpv);
}

int64_t MpvController::apiVersion()
{
    return mpv_client_api_version();
}

void MpvController::setLogLevel(QString logLevel)
{
    mpv_request_log_messages(mpv, logLevel.toUtf8().data());
}

mpv_opengl_cb_context* MpvController::mpvDrawContext()
{
    return glMpv;
}

int MpvController::setOptionVariant(QString name, const QVariant &value)
{
    return mpv::qt::set_option_variant(mpv, name, value);
}

QVariant MpvController::command(const QVariant &params)
{
    if (params.canConvert<QString>()) {
        int value = mpv_command_string(mpv, params.value<QString>().toUtf8().data());
        if (value < 0)
            return QVariant::fromValue(MpvErrorCode(value));
        return QVariant();
    }

    mpv::qt::node_builder node(params);
    mpv_node res;
    int value = mpv_command_node(mpv, node.node(), &res);
    if (value < 0)
        return QVariant::fromValue(MpvErrorCode(value));
    mpv::qt::node_autofree f(&res);
    QVariant v = mpv::qt::node_to_variant(&res);
    return v;
}

int MpvController::setPropertyVariant(const QString &name, const QVariant &value)
{
    return mpv::qt::set_property_variant(mpv, name, value);
}

QVariant MpvController::getPropertyVariant(const QString &name)
{
    mpv_node node;
    int r = mpv_get_property(mpv, name.toUtf8().data(), MPV_FORMAT_NODE, &node);
    if (r < 0)
        return QVariant::fromValue<MpvErrorCode>(MpvErrorCode(r));
    QVariant v = mpv::qt::node_to_variant(&node);
    mpv_free_node_contents(&node);
    return v;
}

int MpvController::setPropertyString(const QString &name, const QString &value)
{
    return mpv_set_property_string(mpv, name.toUtf8().data(), value.toUtf8().data());
}

QString MpvController::getPropertyString(const QString &name)
{
    char *c = mpv_get_property_string(mpv, name.toUtf8().data());
    if (!c)
        return QString();
    QByteArray b(c);
    mpv_free(c);
    return QString::fromUtf8(b);
}

void MpvController::commandAsync(const QVariant &params, MpvCallback *callback)
{
    mpv::qt::node_builder node(params);
    mpv_command_node_async(mpv, reinterpret_cast<uint64_t>(callback),
                           node.node());
}

void MpvController::setPropertyVariantAsync(const QString &name,
                                            const QVariant &value,
                                            MpvCallback *callback)
{
    mpv::qt::node_builder node(value);
    mpv_set_property_async(mpv, reinterpret_cast<uint64_t>(callback),
                           name.toUtf8().data(), MPV_FORMAT_NODE, node.node());
}

void MpvController::getPropertyVariantAsync(const QString &name,
                                            MpvCallback *callback)
{
    mpv_get_property_async(mpv, reinterpret_cast<uint64_t>(callback),
                           name.toUtf8().data(), MPV_FORMAT_NODE);
}

void MpvController::parseMpvEvents()
{
    // Process all events, until the event queue is empty.
    while (mpv) {
        mpv_event *event = mpv_wait_event(mpv, 0);
        if (event->event_id == MPV_EVENT_NONE) {
            break;
        }
        handleMpvEvent(event);
    }
}

void MpvController::setThrottledProperty(const QString &name, const QVariant &v, uint64_t userData)
{
    throttledValues.insert(name, QPair<QVariant,uint64_t>(v,userData));
}

void MpvController::flushProperties()
{
    foreach (QString key, throttledValues.keys())
        emit mpvPropertyChanged(key, throttledValues[key].first,
                                throttledValues[key].second);
    throttledValues.clear();
}

void MpvController::handleMpvEvent(mpv_event *event)
{
    auto propertyToVariant = [event](mpv_event_property *prop) -> QVariant {
        auto asBool = [&](bool dflt = false) {
            return (prop->format != MPV_FORMAT_FLAG || prop->data == NULL) ?
                        dflt : *reinterpret_cast<bool*>(prop->data);
        };
        auto asDouble = [&](double dflt = nan("")) {
            return (prop->format != MPV_FORMAT_DOUBLE || prop->data == NULL) ?
                        dflt : *reinterpret_cast<double*>(prop->data);
        };
        auto asInt64 = [&](int64_t dflt = -1) {
            return (prop->format != MPV_FORMAT_INT64 || prop->data == NULL) ?
                        dflt : *reinterpret_cast<int64_t*>(prop->data);
        };
        auto asString = [&](QString dflt = QString()) {
            return (!(prop->format == MPV_FORMAT_STRING ||
                      prop->format == MPV_FORMAT_OSD_STRING) ||
                    prop->data == NULL) ?
                        dflt : QString(*reinterpret_cast<char**>(prop->data));
        };
        auto asNode = [&](QVariant dflt = QVariant()) {
            return (prop->format != MPV_FORMAT_NODE || prop->data == NULL) ?
                        dflt : mpv::qt::node_to_variant(
                            reinterpret_cast<mpv_node*>(prop->data));
        };
        if (prop->data == NULL) {
            return QVariant::fromValue<MpvErrorCode>(MpvErrorCode(event->error));
        } else if (prop->format == MPV_FORMAT_NODE) {
            return asNode();
        } else if (prop->format == MPV_FORMAT_INT64) {
            return (long long)asInt64();
        } else if (prop->format == MPV_FORMAT_DOUBLE) {
            return asDouble();
        } else if (prop->format == MPV_FORMAT_STRING ||
                   prop->format == MPV_FORMAT_OSD_STRING) {
            return asString();
        } else if (prop->format == MPV_FORMAT_FLAG) {
            return asBool();
        }
        return QVariant();
    };

    switch (event->event_id) {
    case MPV_EVENT_GET_PROPERTY_REPLY: {
        QVariant v = propertyToVariant(reinterpret_cast<mpv_event_property*>(event->data));
        if (!event->reply_userdata)
            return;
        QMetaObject::invokeMethod(reinterpret_cast<MpvCallback*>(event->reply_userdata),
                                  "reply", Qt::QueuedConnection,
                                  Q_ARG(QVariant, v));
        break;
    }
    case MPV_EVENT_COMMAND_REPLY:
    case MPV_EVENT_SET_PROPERTY_REPLY: {
        QVariant v = QVariant::fromValue<MpvErrorCode>(MpvErrorCode(event->error));
        if (!event->reply_userdata)
            return;
        QMetaObject::invokeMethod(reinterpret_cast<MpvCallback*>(event->reply_userdata),
                                  "reply", Qt::QueuedConnection,
                                  Q_ARG(QVariant, v));
        break;
    }
    case MPV_EVENT_PROPERTY_CHANGE: {
        QVariant v = propertyToVariant(reinterpret_cast<mpv_event_property*>(event->data));
        QString propname = QString::fromUtf8(reinterpret_cast<mpv_event_property*>(event->data)->name);
        if (throttledProperties.contains(propname))
            setThrottledProperty(propname, v, event->reply_userdata);
        else
            emit mpvPropertyChanged(propname, v, event->reply_userdata);
        break;
    }
    case MPV_EVENT_LOG_MESSAGE: {
        mpv_event_log_message *msg =
                reinterpret_cast<mpv_event_log_message*>(event->data);
        emit logMessage(QString("[%1] %2: %3").arg(msg->prefix, msg->level,
                                                   msg->text));
        break;
    }
    case MPV_EVENT_CLIENT_MESSAGE: {
        mpv_event_client_message *msg =
                reinterpret_cast<mpv_event_client_message*>(event->data);
        QStringList list;
        for (int i = 0; i < msg->num_args; i++)
            list.append(msg->args[i]);
        emit clientMessage(event->reply_userdata, list);
        break;
    }
    case MPV_EVENT_VIDEO_RECONFIG: {
        // Retrieve the new video size.
        QVariant vw, vh;
        vw = getPropertyVariant("width");
        vh = getPropertyVariant("height");
        int w, h;
        if (!vw.canConvert<MpvErrorCode>() && !vh.canConvert<MpvErrorCode>()
                && (w = vw.toInt()) > 0 && (h = vh.toInt()) > 0) {
            QSize videoSize(w, h);
            if (lastVideoSize != videoSize) {
                emit videoSizeChanged(videoSize);
                lastVideoSize = videoSize;
            }
        } else if (!lastVideoSize.isEmpty()) {
            lastVideoSize = QSize();
            emit videoSizeChanged(QSize());
        }
        break;
    }
    default:
        emit unhandledMpvEvent(event->event_id);
    }
}

void MpvController::mpvWakeup(void *ctx)
{
    QMetaObject::invokeMethod((MpvController*)ctx, "parseMpvEvents",
                              Qt::QueuedConnection);
}
