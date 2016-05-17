// Portions of code in this module came from the examples directory in mpv's
// git repo.  Reworked by me.

#include <QtGlobal>
#ifdef Q_OS_LINUX
#include <QtX11Extras/QX11Info>
#endif
#include <QThread>
#include <QTimer>
#include <QOpenGLContext>
#include <QMetaObject>
#include <QDir>
#include <QDebug>
#include <cmath>
#include <stdexcept>
#include <mpv/qthelper.hpp>
#include "mpvwidget.h"
#include "helpers.h"

#ifndef GLAPIENTRY
// On Windows, GLAPIENTRY may sometimes conveniently go missing
#define GLAPIENTRY __stdcall
#endif

static void* GLAPIENTRY glMPGetNativeDisplay(const char* name) {
#ifdef Q_OS_LINUX
    if (!strcmp(name, "x11")) {
        return (void*)QX11Info::display();
    }
#endif
#ifdef Q_OS_WIN
    if (!strcmp(name, "IDirect3DDevice9")) {
        // Do something here ?
    }
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

MpvWidget::MpvWidget(QWidget *parent) :
    QOpenGLWidget(parent), nnedi3Available_(true), drawLogo(true),
    logo(NULL)
{
    debugMessages = false;

    // When (un)docking windows, some widgets may get transformed into native
    // widgets, causing painting glitches.  Tell Qt that we prefer non-native.
    setAttribute(Qt::WA_DontCreateNativeAncestors);

    // Setup threads
    worker = new QThread();
    worker->start();

    // setup controller
    ctrl = new MpvController();
    ctrl->moveToThread(worker);

    // Wire the basic mpv functions to avoid littering the codebase with
    // QMetaObject::invokeMethod.  This way the compiler will catch our
    // typos rather than a runtime error.
    connect(this, &MpvWidget::ctrlCommand,
            ctrl, &MpvController::command, Qt::QueuedConnection);
    connect(this, &MpvWidget::ctrlSetOptionVariant,
            ctrl, &MpvController::setOptionVariant, Qt::QueuedConnection);
    connect(this, &MpvWidget::ctrlSetPropertyVariant,
            ctrl, &MpvController::setPropertyVariant, Qt::QueuedConnection);

    // Wire up the event-handling callbacks
    connect(ctrl, &MpvController::nnedi3Unavailable,
            this, &MpvWidget::ctrl_nnedi3Unavailable, Qt::QueuedConnection);
    connect(ctrl, &MpvController::mpvPropertyChanged,
            this, &MpvWidget::ctrl_mpvPropertyChanged, Qt::QueuedConnection);
    connect(ctrl, &MpvController::logMessage,
            this, &MpvWidget::ctrl_logMessage, Qt::QueuedConnection);
    connect(ctrl, &MpvController::unhandledMpvEvent,
            this, &MpvWidget::ctrl_unhandledMpvEvent, Qt::QueuedConnection);
    connect(ctrl, &MpvController::videoSizeChanged,
            this, &MpvWidget::ctrl_videoSizeChanged, Qt::QueuedConnection);

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
        { "time-pos", MPV_FORMAT_DOUBLE },
        { "pause", MPV_FORMAT_FLAG },
        { "media-title", MPV_FORMAT_STRING },
        { "chapter-metadata", MPV_FORMAT_NODE },
        { "track-list", MPV_FORMAT_NODE },
        { "chapter-list", MPV_FORMAT_NODE },
        { "duration", MPV_FORMAT_DOUBLE },
        { "estimated-vf-fps", MPV_FORMAT_DOUBLE },
        { "avsync", MPV_FORMAT_DOUBLE },
        { "vo-drop-frame-count", MPV_FORMAT_INT64 },
        { "drop-frame-count", MPV_FORMAT_INT64 },
        { "audio-bitrate", MPV_FORMAT_DOUBLE },
        { "video-bitrate", MPV_FORMAT_DOUBLE },
        { "paused-for-cache", MPV_FORMAT_FLAG },
        { "metadata", MPV_FORMAT_NODE }
    };
    QSet<QString> throttled = {
        "time-pos", "avsync", "estimated-vf-fps", "vo-drop-frame-count",
        "drop-frame-count", "audio-bitrate", "video-bitrate"
    };
    QMetaObject::invokeMethod(ctrl, "observeProperties",
                              Qt::BlockingQueuedConnection,
                              Q_ARG(const MpvController::PropertyList &, options),
                              Q_ARG(const QSet<QString> &, throttled));

    // Output debug messages from mpv
    if (debugMessages)
        QMetaObject::invokeMethod(ctrl, "setLogLevel",
                                  Qt::BlockingQueuedConnection,
                                  Q_ARG(MpvController::LogLevel,
                                        MpvController::LogInfo));

    emit ctrlSetOptionVariant("ytdl", "yes");

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
    worker->deleteLater();
}

void MpvWidget::showMessage(QString message)
{
    emit ctrlCommand(QVariantList({"show_text", message, "1000"}));
}

void MpvWidget::fileOpen(QString filename)
{
    emit ctrlCommand(QStringList({"loadfile", filename}));
    setPaused(false);
}

void MpvWidget::discFilesOpen(QString path) {
    QStringList entryList = QDir(path).entryList();
    if (entryList.contains("VIDEO_TS") || entryList.contains("AUDIO_TS")) {
        fileOpen(path + "/VIDEO_TS/VIDEO_TS.IFO");
    } else if (entryList.contains("BDMV") || entryList.contains("AACS")) {
        fileOpen("bluray://" + path);
    }
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

void MpvWidget::setVOCommandLine(QString cmdline)
{
    if (debugMessages)
        qDebug() << "got advanced command line " << cmdline;
    emit ctrlCommand(QVariantList({"vo-cmdline", cmdline}));
}

void MpvWidget::stopPlayback()
{
    emit ctrlCommand("stop");
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
    setMpvPropertyVariant("ab-loop-a", first < 0 ? (-0x1p+63) : first);
    setMpvPropertyVariant("ab-loop-b", end < 0 ? (-0x1p+63) : end);
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

void MpvWidget::setSubsAreGray(bool yes)
{
    setMpvOptionVariant("sub-gray", yes);
}

void MpvWidget::setFramedropMode(QString mode)
{
    setMpvOptionVariant("framedrop", mode);
}

void MpvWidget::setDecoderDropMode(QString mode)
{
    setMpvOptionVariant("vd-lavc-framedrop", mode);
}

void MpvWidget::setDisplaySyncMode(QString mode)
{
    setMpvOptionVariant("video-sync", mode);
}

void MpvWidget::setAudioDropSize(double size)
{
    setMpvOptionVariant("video-sync-adrop-size", size);
}

void MpvWidget::setMaximumAudioChange(double change)
{
    setMpvOptionVariant("video-sync-max-audio-change", change);
}

void MpvWidget::setMaximumVideoChange(double change)
{
    setMpvOptionVariant("video-sync-max-video-change", change);
}

void MpvWidget::setScreenshotFormat(QString format)
{
    setMpvOptionVariant("screenshot-format", format);
}

void MpvWidget::setScreenshotJpegQuality(int64_t value)
{
    setMpvOptionVariant("screenshot-jpeg-quality", (long long)value);
}

void MpvWidget::setScreenshotJpegSmooth(int64_t value)
{
    setMpvOptionVariant("screenshot-jpeg-smooth", (long long)value);
}

void MpvWidget::setScreenshotJpegSourceChroma(bool yes)
{
    setMpvOptionVariant("screenshot-jpeg-source-chroma", yes);
}

void MpvWidget::setScreenshotPngCompression(int64_t value)
{
    setMpvOptionVariant("screenshot-png-compression", (long long)value);
}

void MpvWidget::setScreenshotPngFilter(int64_t value)
{
    setMpvOptionVariant("screenshot-png-filter", (long long)value);
}

void MpvWidget::setScreenshotPngColorspace(bool yes)
{
    setMpvOptionVariant("screenshot-tag-colorspace", yes);
}

void MpvWidget::setClientDebuggingMessages(bool yes)
{
    debugMessages = yes;
}

void MpvWidget::setMpvLogLevel(QString level)
{
    setMpvOptionVariant("log-level", QString("all=%1").arg(level));
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

bool MpvWidget::nnedi3Available()
{
    return nnedi3Available_;
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

void MpvWidget::ctrl_update(void *ctx)
{
    QMetaObject::invokeMethod(reinterpret_cast<MpvWidget*>(ctx), "update");
}

void MpvWidget::setMpvPropertyVariant(QString name, QVariant value)
{
    if (debugMessages)
        qDebug() << "property set " << name << value;
    emit ctrlSetPropertyVariant(name, value);
}

QVariant MpvWidget::getMpvPropertyVariant(QString name)
{
    if (debugMessages)
        qDebug() << "property get " << name;
    QVariant v;
    QMetaObject::invokeMethod(ctrl, "getPropertyVariant",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVariant, v),
                              Q_ARG(QString, name));
    return v;
}

void MpvWidget::setMpvOptionVariant(QString name, QVariant value)
{
    if (debugMessages)
        qDebug() << "option set " << name << value;
    emit ctrlSetOptionVariant(name, value);
}

void MpvWidget::ctrl_nnedi3Unavailable()
{
    if (debugMessages)
        qDebug() << "nnedi3 is unavailable!";
    nnedi3Available_ = false;
}

#define HANDLE_PROP_1(p, method, converter, dflt) \
    if (name == p) { \
        if (ok && v.canConvert<__typeof__ dflt>()) \
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
    HANDLE_PROP_1("vo-drop-frame-count", displayFramedropsChanged, toLongLong, 0ll);
    HANDLE_PROP_1("drop-frame-count", decoderFramedropsChanged, toLongLong, 0ll);
    HANDLE_PROP_1("metadata", self_metadata, toMap, QVariantMap());
}

void MpvWidget::ctrl_logMessage(QString message)
{
    qDebug() << message;
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

    setLogLevel(LogTerminalDefault);

    if (!audio) {
        setOptionVariant("ao", "null");
        setOptionVariant("no-audio", true);
    }
    if (!video) {
        // NOTE: this completely skips setting up the gl interface.
        setOptionVariant("vo", "null");
        setOptionVariant("no-video", true);
    } else {
        // check for nnedi3
        if (setOptionVariant("vo", "opengl-cb:prescale-luma=nnedi3") < 0)
            emit nnedi3Unavailable();
        setOptionVariant("vo", "opengl-cb");

        glMpv = (mpv_opengl_cb_context *)mpv_get_sub_api(mpv, MPV_SUB_API_OPENGL_CB);
        if (!glMpv)
            throw std::runtime_error("OpenGL not compiled in");
    }
    mpv_set_wakeup_callback(mpv, MpvController::mpvWakeup, this);
}

void MpvController::observeProperties(const MpvController::PropertyList &properties,
                                      const QSet<QString> &throttled)
{
    foreach (MpvProperty item, properties)
        mpv_observe_property(mpv, 0, item.first, item.second);
    throttledProperties = throttled;
}

void MpvController::setThrottleTime(int msec)
{
    throttler->setInterval(msec);
}

void MpvController::setLogLevel(LogLevel level)
{
    QVector<const char*> logLevels = { "no", "fatal", "error", "warn ",
                                       "info", "status", "v", "debug",
                                       "trace", "terminal-default" };
    mpv_request_log_messages(mpv, logLevels.value(level));
}

mpv_opengl_cb_context* MpvController::mpvDrawContext()
{
    return glMpv;
}

int MpvController::setOptionVariant(QString name, const QVariant &value)
{
    return mpv::qt::set_option_variant(mpv, name, value);
}

void MpvController::command(const QVariant &params)
{
    if (params.type() == QVariant::String)
        mpv_command_string(mpv, params.toString().toUtf8().data());
    else
        mpv::qt::command_variant(mpv, params);
}

int MpvController::setPropertyVariant(const QString &name, const QVariant &value)
{
    return mpv::qt::set_property_variant(mpv, name, value);
}

QVariant MpvController::getPropertyVariant(const QString &name)
{
    mpv_node node;
    int r = mpv_get_property(mpv, name.toUtf8().data(), MPV_FORMAT_NODE, &node);
    mpv::qt::node_autofree f(&node);
    if (r < 0)
        return QVariant::fromValue<MpvErrorCode>(MpvErrorCode(r));
    return mpv::qt::node_to_variant(&node);
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

void MpvController::setThrottledProperty(const QString &name, const QVariant &v)
{
    throttledValues[name] = v;
}

void MpvController::flushProperties()
{
    foreach (QString key, throttledValues.keys())
        emit mpvPropertyChanged(key, throttledValues[key]);
    throttledValues.clear();
}

void MpvController::handleMpvEvent(mpv_event *event)
{
    switch (event->event_id) {
    case MPV_EVENT_PROPERTY_CHANGE: {
        mpv_event_property *prop =
                reinterpret_cast<mpv_event_property*>(event->data);
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
        QVariant v;
        if (prop->data == NULL) {
            v = QVariant::fromValue<MpvErrorCode>(MpvErrorCode(event->error));
        } else if (prop->format == MPV_FORMAT_NODE) {
            v = asNode();
        } else if (prop->format == MPV_FORMAT_INT64) {
            v = (long long)asInt64();
        } else if (prop->format == MPV_FORMAT_DOUBLE) {
            v = asDouble();
        } else if (prop->format == MPV_FORMAT_STRING ||
                   prop->format == MPV_FORMAT_OSD_STRING) {
            v = asString();
        } else if (prop->format == MPV_FORMAT_FLAG) {
            v = asBool();
        }
        QString propname = QString::fromUtf8(prop->name);
        if (throttledProperties.contains(propname))
            setThrottledProperty(propname, v);
        else
            emit mpvPropertyChanged(propname, v);
        break;
    }
    case MPV_EVENT_LOG_MESSAGE: {
        mpv_event_log_message *msg =
                reinterpret_cast<mpv_event_log_message*>(event->data);
        emit logMessage(QString("[%1] %2: %3").arg(msg->prefix, msg->level,
                                                   msg->text));
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
