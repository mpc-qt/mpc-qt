// Portions of code in this module came from the examples directory in mpv's
// git repo.  Reworked by me.

#include <QLayout>
#include <QMainWindow>
#include <QGuiApplication>
#include <QThread>
#include <QTimer>
#include <QOpenGLContext>
#include <QMouseEvent>
#include <QMetaObject>
#include <QDir>
#include <QWindow>
#include <cmath>
#include <stdexcept>
#include "logger.h"
#include "mpvwidget.h"
#include "widgets/logowidget.h"
#include "storage.h"

#ifndef Q_PROCESSOR_ARM
    #ifndef GLAPIENTRY
    // On Windows, GLAPIENTRY may sometimes conveniently go missing
    #define GLAPIENTRY __stdcall
    #endif
#endif
#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

#define HANDLE_PROP(p, method, converter, dflt) \
{ \
    p, \
    [](MpvObject *self, bool ok, const QVariant &v) -> void { \
        if (ok && v.canConvert<decltype(dflt)>()) \
            emit self->method(v.converter());  \
        else \
            emit self->method(dflt); \
    } \
}

MpvObject::PropertyDispatchMap MpvObject::propertyDispatch = {
    HANDLE_PROP("time-pos", self_playTimeChanged, toDouble, 0.0),
    HANDLE_PROP("duration", self_playLengthChanged, toDouble, 0.0),
    HANDLE_PROP("seekable", seekableChanged, toBool, false),
    HANDLE_PROP("pause", pausedChanged, toBool, true),
    HANDLE_PROP("eof-reached", eofReachedChanged, toString, QString()),
    HANDLE_PROP("media-title", mediaTitleChanged, toString, QString()),
    HANDLE_PROP("chapter", self_chapterChanged, toDouble, 0.0),
    HANDLE_PROP("chapter-metadata/title", chapterTitleChanged, toString, QString()),
    HANDLE_PROP("chapter-list", chaptersChanged, toList, QVariantList()),
    HANDLE_PROP("track-list", tracksChanged, toList, QVariantList()),
    HANDLE_PROP("estimated-vf-fps", fpsChanged, toDouble, 0.0),
    HANDLE_PROP("avsync", avsyncChanged, toDouble, 0.0),
    HANDLE_PROP("frame-drop-count", displayFramedropsChanged, toLongLong, 0ll),
    HANDLE_PROP("decoder-frame-drop-count", decoderFramedropsChanged, toLongLong, 0ll),
    HANDLE_PROP("audio-bitrate", audioBitrateChanged, toDouble, 0.0),
    HANDLE_PROP("video-bitrate", videoBitrateChanged, toDouble, 0.0),
    HANDLE_PROP("video-params/aspect", self_aspectChanged, toDouble, 0.0),
    HANDLE_PROP("video-params/aspect-name", aspectNameChanged, toString, QString()),
    HANDLE_PROP("metadata", self_metadata, toMap, QVariantMap()),
    HANDLE_PROP("audio-device-list", self_audioDeviceList, toList, QVariantList()),
    HANDLE_PROP("filename", fileNameChanged, toString, QString()),
    HANDLE_PROP("file-format", fileFormatChanged, toString, QString()),
    HANDLE_PROP("file-size", fileSizeChanged, toLongLong, 0ll),
    HANDLE_PROP("path", filePathChanged, toString, QString()),
    HANDLE_PROP("sub-text", subTextChanged, toString, QString()),
    HANDLE_PROP("hwdec-current", hwdecCurrentChanged, toString, QString())
};

MpvObject::MpvObject(QObject *owner, const QString &clientName) : QObject(owner)
{
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
    // QMetaObject::invokeMethod.
    connect(this, &MpvObject::ctrlContinueHook,
            ctrl, &MpvController::continueHook, Qt::QueuedConnection);
    connect(this, &MpvObject::ctrlCommand,
            ctrl, &MpvController::command, Qt::QueuedConnection);
    connect(this, &MpvObject::ctrlSetOptionVariant,
            ctrl, &MpvController::setOptionVariant, Qt::QueuedConnection);
    connect(this, &MpvObject::ctrlSetPropertyVariant,
            ctrl, &MpvController::setPropertyVariant, Qt::QueuedConnection);
    connect(this, &MpvObject::ctrlSetLogLevel,
            ctrl, &MpvController::setLogLevel, Qt::QueuedConnection);
    connect(this, &MpvObject::ctrlShowStats,
            ctrl, &MpvController::showStatsPage, Qt::QueuedConnection);

    // Wire up the event-handling callbacks
    connect(ctrl, &MpvController::mpvPropertyChanged,
            this, &MpvObject::ctrl_mpvPropertyChanged, Qt::QueuedConnection);
    connect(ctrl, &MpvController::hookEvent,
            this, &MpvObject::ctrl_hookEvent, Qt::QueuedConnection);
    connect(ctrl, &MpvController::unhandledMpvEvent,
            this, &MpvObject::ctrl_unhandledMpvEvent, Qt::QueuedConnection);
    connect(ctrl, &MpvController::videoSizeChanged,
            this, &MpvObject::ctrl_videoSizeChanged, Qt::QueuedConnection);

    // Wire up the mouse and timer-related callbacks
    connect(this, &MpvObject::mouseMoved,
            this, &MpvObject::self_mouseMoved);
    connect(this, &MpvObject::mousePress,
            this, &MpvObject::self_mousePress);
    connect(this, &MpvObject::keyPress,
            this, &MpvObject::self_keyPress);
    connect(this, &MpvObject::keyRelease,
            this, &MpvObject::self_keyRelease);
    connect(hideTimer, &QTimer::timeout,
            this, &MpvObject::hideTimer_timeout);

    // Wire up the logging interface
    connect(ctrl, &MpvController::logMessageByParts,
            Logger::singleton(), &Logger::makeLogDescriptively);

    // Fetch installed scripts
    QString scriptPath = Storage::fetchConfigPath() + "/scripts";
    auto scriptInfoList = QDir(scriptPath).entryInfoList({"*.lua"}, QDir::Files);
    QStringList scripts;
    for (auto &info : scriptInfoList)
        scripts.append(info.absoluteFilePath());

    // Initialize mpv playback instance
    MpvController::OptionList earlyOptions = {
        { "vo", "libmpv" },
        { "ytdl", "yes" },
        { "audio-client-name", clientName },
        { "load-scripts", true },
        { "scripts", scripts },
        { "clipboard-backends", "clr" }
    };
    QMetaObject::invokeMethod(ctrl, "create", Qt::BlockingQueuedConnection,
                              Q_ARG(MpvController::OptionList, earlyOptions));

    // clean up objects when the worker thread is deleted
    connect(worker, &QThread::finished, ctrl, &MpvController::deleteLater);

    // Observe some properties
    MpvController::PropertyList options = {
        { "time-pos", 0, MPV_FORMAT_DOUBLE },
        { "pause", 0, MPV_FORMAT_FLAG },
        { "eof-reached", 0, MPV_FORMAT_FLAG },
        { "video-params/aspect", 0, MPV_FORMAT_DOUBLE },
        { "video-params/aspect-name", 0, MPV_FORMAT_STRING },
        { "media-title", 0, MPV_FORMAT_STRING },
        { "chapter", 0, MPV_FORMAT_DOUBLE },
        { "chapter-metadata/title", 0, MPV_FORMAT_STRING },
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
        { "audio-device-list", 0, MPV_FORMAT_NODE },
        { "filename", 0, MPV_FORMAT_STRING },
        { "file-format", 0, MPV_FORMAT_STRING },
        { "file-size", 0, MPV_FORMAT_STRING },
        { "path", 0, MPV_FORMAT_STRING },
        { "seekable", 0, MPV_FORMAT_FLAG },
        { "sub-text", 0, MPV_FORMAT_STRING },
        { "hwdec-current", 0, MPV_FORMAT_STRING }
    };
    QSet<QString> throttled = {
        "time-pos", "avsync", "estimated-vf-fps", "frame-drop-count",
        "decoder-frame-drop-count", "audio-bitrate", "video-bitrate"
    };
    QMetaObject::invokeMethod(ctrl, "observeProperties",
                              Qt::QueuedConnection,
                              Q_ARG(MpvController::PropertyList, options),
                              Q_ARG(QSet<QString>, throttled));

    QMetaObject::invokeMethod(ctrl, "addHook",
                              Qt::QueuedConnection,
                              Q_ARG(QString, "on_unload"),
                              Q_ARG(uint64_t, reinterpret_cast<uint64_t>(this)));

    QMetaObject::invokeMethod(ctrl, "setLogLevel",
                              Qt::QueuedConnection,
                              Q_ARG(QString, "info"));
}

MpvObject::~MpvObject()
{
    Logger::log(logModule, "~MpvObject");
    if (widget)
        delete widget;
    if (hideTimer) {
        delete hideTimer;
        hideTimer = nullptr;
    }
    if (ctrl) {
        // no explicit delete. deleteLater will be called at thread finish.
        // Instead tell mpv to stop sending wake notifications so the
        // can be reliably idle when we end it.
        QMetaObject::invokeMethod(ctrl, "stop",
                                  Qt::BlockingQueuedConnection);
        ctrl = nullptr;
    }
    worker->quit();
    worker->wait();
    worker->deleteLater();
}

void MpvObject::setHostLayout(QLayout *hostLayout)
{
    if (!this->hostLayout)
        this->hostLayout = hostLayout;
}

void MpvObject::setHostWindow(QMainWindow *hostWindow)
{
    if (!this->hostWindow)
        this->hostWindow = hostWindow;
}

void MpvObject::setWidgetType(Helpers::MpvWidgetType widgetType, MpvWidgetInterface *customWidget)
{
    if (this->widgetType == widgetType)
        return;
    this->widgetType = widgetType;

    if (widget) {
        delete widget;
        widget = nullptr;
    }

    switch(widgetType) {
    case Helpers::NullWidget:
        widget = nullptr;
        break;
    case Helpers::EmbedWidget:
        widget = new MpvEmbedWidget(this);
        break;
    case Helpers::GlCbWidget:
        widget = new MpvGlWidget(this);
        break;
    case Helpers::VulkanCbWidget:
        widget = new MpvVulkanCbWidget(this);
        break;
    case Helpers::CustomWidget:
        widget = customWidget;
        if (widget == nullptr)
            this->widgetType = Helpers::NullWidget;
        break;
    }
    if (!widget)
        return;

    if (hostLayout)
        hostLayout->addWidget(widget->self());
    else if (hostWindow)
        hostWindow->setCentralWidget(widget->self());
    widget->setController(ctrl);
    widget->initMpv();
}

QString MpvObject::mpvVersion()
{
    return getMpvPropertyVariant("mpv-version").toString();
}

QString MpvObject::ffmpegVersion()
{
    return getMpvPropertyVariant("ffmpeg-version").toString();
}

MpvController *MpvObject::controller()
{
    return ctrl;
}

QWidget *MpvObject::mpvWidget()
{
    return widget ? widget->self() : nullptr;
}

QList<AudioDevice> MpvObject::audioDevices()
{
    return AudioDevice::listFromVList(getMpvPropertyVariant("audio-device-list").toList());
}

QStringList MpvObject::supportedProtocols()
{
    return ctrl->protocolList();
}

void MpvObject::showMessage(QString message)
{
    if (shownStatsPage == 0)
        emit ctrlCommand(QVariantList({"show_text", message, "2000"}));
}

void MpvObject::showStatsPage(int page)
{
    emit ctrlShowStats(page);
    shownStatsPage = page;
}

int MpvObject::cycleStatsPage()
{
    QMap<int,int> pageFromTo = {
        { -1, 0 },
        { 0, 1 },
        { 1, 3 },
        { 2, 3 },
        { 3, -1 }
    };
    showStatsPage(pageFromTo.value(shownStatsPage, -1));
    return shownStatsPage;
}

int MpvObject::selectedStatsPage()
{
    return shownStatsPage;
}

void MpvObject::urlOpen(QUrl url)
{
    fileOpen(url.isLocalFile() ? url.toLocalFile()
                               : QUrl::fromPercentEncoding(url.toEncoded()));
}

void MpvObject::fileOpen(QString filename, bool replaceMpvPlaylist)
{
    clearSubFiles();
    //setStartTime(0.0);
    if (replaceMpvPlaylist)
        emit ctrlCommand(QStringList({"loadfile", filename}));
    else
        emit ctrlCommand(QStringList({"loadfile", filename, "append-play"}));
    setMouseHideTime(hideTimer->interval());
}

void MpvObject::discFilesOpen(QString path) {
    QStringList entryList = QDir(path).entryList();
    if (entryList.contains("VIDEO_TS") || entryList.contains("AUDIO_TS")) {
        fileOpen(path + "/VIDEO_TS/VIDEO_TS.IFO");
    } else if (entryList.contains("BDMV") || entryList.contains("AACS")) {
        fileOpen("bluray://" + path);
    }
}

void MpvObject::stopPlayback()
{
    emit ctrlCommand("stop");
}

void MpvObject::stepBackward()
{
    emit ctrlCommand("frame_back_step");
}

void MpvObject::stepForward()
{
    emit ctrlCommand("frame_step");
}

void MpvObject::seek(double amount, bool exact)
{
    QVariantList payload({"seek", amount});
    if (exact)
        payload.append("exact");
    emit ctrlCommand(payload);
}

void MpvObject::screenshot(const QString &fileName, Helpers::ScreenshotRender render)
{
    static QMap <Helpers::ScreenshotRender,const char*> methods {
        { Helpers::VideoRender, "video" },
        { Helpers::SubsRender, "subtitles" },
        { Helpers::WindowRender, "window" }
    };
    if (render == Helpers::WindowRender) {
        widget->self()->grab().save(fileName);
        return;
    }
    emit ctrlCommand(QStringList({"screenshot-to-file", fileName,
                                  methods.value(render, "video")}));
}

void MpvObject::setMouseHideTime(int msec)
{
    hideTimer->stop();
    hideTimer->setInterval(msec);
    showCursor();
    if (msec > 0)
        hideTimer->start();
}

void MpvObject::setLogoUrl(const QString &filename)
{
    if (widget)
        widget->setLogoUrl(filename);
}

void MpvObject::setLogoBackground(const QColor &color)
{
    if (widget)
        widget->setLogoBackground(color);
}

void MpvObject::setAudioFilters(const QList<QPair<QString, QString>> &filtersList)
{
    emit ctrlCommand("no-osd af clear");
    emit ctrlCommand("no-osd af set \"" + formatFiltersList(filtersList) + "\"");
}

void MpvObject::setVideoFilters(const QList<QPair<QString, QString>> &filtersList)
{
    emit ctrlCommand("no-osd vf clear");
    emit ctrlCommand("no-osd vf set \"" + formatFiltersList(filtersList) + "\"");
}

QString MpvObject::formatFiltersList(const QList<QPair<QString, QString>> &filtersList)
{
    QString filters;
    for (auto &[filterName, filterValue] : filtersList)
        filters.append(filterName).append('=').append(filterValue).append(',');

    filters.chop(1);
    return filters;
}

void MpvObject::setSubFile(QString filename)
{
    emit ctrlSetOptionVariant("sub-files", filename);
}

void MpvObject::addSubFile(QString filename)
{
    emit ctrlCommand(QStringList({"sub-add", filename}));
}

void MpvObject::clearSubFiles()
{
    emit ctrlSetOptionVariant("sub-files-clr", "");
}

void MpvObject::setSubtitlesDelay(int subDelayStep)
{
    double newSubDelay = getMpvPropertyVariant("sub-delay").toDouble() + (double) subDelayStep / 1000;
    setMpvPropertyVariant("sub-delay", QString::number(newSubDelay, 'f', 3));
    showMessage(tr("Subtitles delay: %1 ms").arg(std::round(newSubDelay * 1000)));
}

void MpvObject::moveSubtitlesVertically(int diff)
{
    emit ctrlCommand(QVariantList({"add", "sub-margin-y", diff}));
}

void MpvObject::setVideoAspect(double aspectDiff)
{
    setMpvPropertyVariant("video-aspect-override", aspect + aspectDiff);
}

void MpvObject::setVideoAspectPreset(double aspect)
{
    setMpvPropertyVariant("video-aspect-override", aspect);
}

void MpvObject::disableVideoAspect(bool yes)
{
    setMpvPropertyVariant("keepaspect", !yes ? "yes" : "no");
}

void MpvObject::setPanScan(double panScan)
{
    setMpvPropertyVariant("panscan", panScan);
}

void MpvObject::setVideoZoom(double zoom)
{
    setMpvPropertyVariant("video-zoom", zoom);
}

void MpvObject::setVideoWidthScale(double scale)
{
    setMpvPropertyVariant("video-scale-x", scale);
}

void MpvObject::setVideoHeightScale(double scale)
{
    setMpvPropertyVariant("video-scale-y", scale);
}

void MpvObject::setVideoPanX(double value)
{
    setMpvPropertyVariant("video-pan-x", value);
}

void MpvObject::setVideoPanY(double value)
{
    setMpvPropertyVariant("video-pan-y", value);
}
void MpvObject::setVideoRotate(int angle)
{
    setMpvPropertyVariant("video-rotate", angle);
}

void MpvObject::setVideoFlip(bool flip)
{
    if (flip) {
        setMpvOptionVariant("vf", "hflip");
    } else {
        setMpvOptionVariant("vf", "");
    }
}

int64_t MpvObject::chapter()
{
    return chapter_;
}

bool MpvObject::setChapter(int64_t chapter)
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
                              Q_ARG(QVariant, QVariant(qlonglong(chapter))));
    return r == MPV_ERROR_SUCCESS;
}

QString MpvObject::mediaTitle()
{
    return getMpvPropertyVariant("media-title").toString();
}

void MpvObject::setMute(bool yes)
{
    setMpvPropertyVariant("mute", yes);
}

void MpvObject::setPaused(bool yes)
{
    setMpvPropertyVariant("pause", yes);
}

void MpvObject::setSpeed(double speed)
{
    setMpvPropertyVariant("speed", speed);
}

void MpvObject::setTime(double position)
{
    setMpvPropertyVariant("time-pos", position);
}

void MpvObject::setTimeSync(double position)
{
    ctrl->command(QVariantList() << "seek" << position << "absolute");
}

void MpvObject::setLoopPoints(double first, double end)
{
    setMpvPropertyVariant("ab-loop-a",
                          first < 0 ? QVariant("no") : QVariant(first));
    setMpvPropertyVariant("ab-loop-b",
                          end < 0 ? QVariant("no") : QVariant(end));
}

void MpvObject::setAudioTrack(int64_t id)
{
    if (id == -1)
        setMpvPropertyVariant("aid", 1);
    else
        setMpvPropertyVariant("aid", qlonglong(id));
    emit audioTrackSet(id);
}

void MpvObject::setSubtitleTrack(int64_t id)
{
    setMpvPropertyVariant("sid", qlonglong(id));
    emit subtitleTrackSet(id);
}

void MpvObject::setVideoTrack(int64_t id)
{
    if (id == -1)
        setMpvPropertyVariant("vid", 1);
    else
        setMpvPropertyVariant("vid", qlonglong(id));
    emit videoTrackSet(id);
}

void MpvObject::setDrawLogo(bool yes)
{
    widget->setDrawLogo(yes);
}

void MpvObject::setVolume(int64_t volume)
{
    static int64_t lastVolume = -1;
    if (lastVolume == volume)
        return;
    lastVolume = volume;
    setMpvPropertyVariant("volume", qlonglong(volume));
}

void MpvObject::setClientDebuggingMessages(bool yes)
{
    debugMessages = yes;
}

void MpvObject::setMpvLogLevel(QString logLevel)
{
    emit ctrlSetLogLevel(logLevel);
}

void MpvObject::setSendKeyEvents(bool enabled)
{
    sendKeyEvents = enabled;
}

void MpvObject::setSendMouseEvents(bool enabled)
{
    sendMouseEvents = enabled;
}

double MpvObject::playLength()
{
    return playLength_;
}

double MpvObject::playTime()
{
    return playTime_;
}

QSize MpvObject::videoSize()
{
    return videoSize_;
}

bool MpvObject::clientDebuggingMessages()
{
    return debugMessages;
}

void MpvObject::setCachedMpvOption(const QString &option, const QVariant &value)
{
    if (cachedState.contains(option) && cachedState.value(option) == value)
        return;
    cachedState.insert(option, value);
    setMpvOptionVariant(option, value);
}

void MpvObject::setUncachedMpvOption(const QString &option, const QVariant &value)
{
    setMpvOptionVariant(option, value);
}

QVariant MpvObject::blockingMpvCommand(const QVariant &params)
{
    QVariant v;
    QMetaObject::invokeMethod(ctrl, "command",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVariant, v),
                              Q_ARG(QVariant, params));
    return v;
}

QVariant MpvObject::blockingSetMpvPropertyVariant(QString name, const QVariant &value)
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

QVariant MpvObject::blockingSetMpvOptionVariant(QString name, const QVariant &value)
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

QVariant MpvObject::getMpvPropertyVariant(QString name)
{
    QVariant v;
    QMetaObject::invokeMethod(ctrl, "getPropertyVariant",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVariant, v),
                              Q_ARG(QString, name));
    return v;
}


void MpvObject::setMpvPropertyVariant(QString name, const QVariant &value)
{
    if (debugMessages)
        LogStream(logModule) << name << " property set to " << value;
    emit ctrlSetPropertyVariant(name, value);
}

void MpvObject::setMpvOptionVariant(QString name, const QVariant &value)
{
    if (debugMessages)
        LogStream(logModule) << name << " option set to " << value;
    emit ctrlSetOptionVariant(name, value);
}

void MpvObject::showCursor()
{
    if (!widget)
        return;
    auto w = widget->self()->window();
    if (w->cursor() == Qt::BlankCursor || w->isFullScreen()
                                       || w->isMaximized()) {
        widget->self()->setCursor(Qt::ArrowCursor);
        w->setCursor(Qt::ArrowCursor);
    }
}

void MpvObject::hideCursor()
{
    if (!widget)
        return;
    auto w = widget->self()->window();
    if (widget->self()->cursor() == Qt::ArrowCursor || w->isFullScreen()
                                       || w->isMaximized()) {
        if (QGuiApplication::platformName().contains("wayland") && !w->isActiveWindow())
            return;
        widget->self()->setCursor(Qt::BlankCursor);
        //REMOVEME: work around KDE Plasma 6.2.4 bug where cursor stays visible in rightmost position
        if (w->isFullScreen() && QCursor::pos().x() == w->geometry().right()) {
            w->setCursor(Qt::BlankCursor);
            Logger::log(logModule, "workaround: hiding cursor on rightmost pixels");
        }
    }
}

void MpvObject::self_aspectChanged(double newAspect)
{
    aspect = newAspect;
    emit aspectChanged(newAspect);
}

void MpvObject::ctrl_mpvPropertyChanged(QString name, const QVariant &v)
{
    QVariant vForLog = v;
    // Don't show more than 3 decimals or none if those are zero
    if (vForLog.typeId() == QMetaType::Double) {
        vForLog = QString("%1").arg(QString::number(vForLog.toDouble(), 'f', 3));
        if (vForLog.toString().section('.', 1) == "000")
            vForLog = vForLog.toString().section('.', 0, 0);
    }
    if (debugMessages)
        LogStream(logModule) << name << " property changed to " << vForLog;

    bool ok = v.metaType().id() < QMetaType::User
              && v.metaType().id() != QMetaType::UnknownType;
    if (propertyDispatch.contains(name))
        propertyDispatch[name](this, ok, v);
    else
        LogStream(logModule) << name << " property changed, but was not in dispatch list.";
}

void MpvObject::ctrl_hookEvent(QString name, uint64_t selfId, uint64_t mpvId)
{
    if (reinterpret_cast<MpvObject*>(selfId) != this)
        return;

    if (name == "on_unload") {
        QVariantList playlist = getMpvPropertyVariant("playlist").toList();
        emit playlistChanged(playlist);
    }
    emit ctrlContinueHook(mpvId);
}

void MpvObject::ctrl_unhandledMpvEvent(int eventLevel)
{
    switch(eventLevel) {
    case MPV_EVENT_START_FILE: {
        if (debugMessages)
            Logger::log(logModule, "start file");
        emit playbackLoading();
        break;
    }
    case MPV_EVENT_FILE_LOADED: {
        if (debugMessages)
            Logger::log(logModule, "file loaded");
        emit playbackStarted();
        break;
    }
    case MPV_EVENT_END_FILE: {
        if (debugMessages)
            Logger::log(logModule, "end file");
        emit playbackFinished();
        break;
    }
    // Received when the player has no more files to play and is in an idle state
    case MPV_EVENT_IDLE: {
        if (debugMessages)
            Logger::log(logModule, "idling");
        emit playbackIdling();
        break;
    }
    case MPV_EVENT_SHUTDOWN: {
        if (debugMessages)
            Logger::log(logModule, "event shutdown");
        emit playbackFinished();
        break;
    }
    }
}

void MpvObject::ctrl_videoSizeChanged(QSize size)
{
    videoSize_ = size;
    emit videoSizeChanged(videoSize_);
}

void MpvObject::self_playTimeChanged(double playTime)
{
    playTime_ = playTime;
    emit playTimeChanged(playTime);
}

void MpvObject::self_playLengthChanged(double playLength)
{
    playLength_ = playLength;
    emit playLengthChanged(playLength);
}

void MpvObject::self_chapterChanged(int64_t chapter)
{
    chapter_ = chapter;
}

void MpvObject::self_metadata(QVariantMap metadata)
{
    QVariantMap map;
    for (auto it = metadata.begin(); it != metadata.end(); it++)
        map.insert(it.key().toLower(), it.value());
    emit metaDataChanged(map);
}

void MpvObject::self_audioDeviceList(const QVariantList &list)
{
    emit audioDeviceList(AudioDevice::listFromVList(list));
}

void MpvObject::self_mouseMoved(int x, int y)
{
    if (hideTimer->interval() > 0)
        hideTimer->start();
    showCursor();

    if (sendMouseEvents)
        emit ctrlCommand(QStringList({"mouse", QString::number(x),
                                      QString::number(y)}));
}


void MpvObject::self_mousePress(int x, int y, int btn)
{
    if (sendMouseEvents)
        emit ctrlCommand(QStringList({"mouse", QString::number(x),
                                      QString::number(y),
                                      QString::number(btn), "single"}));
}

void MpvObject::self_keyPress(int key)
{
    if (sendKeyEvents) {
        if (key >= 0x20 && key <= 0x7f) {
            QChar str[2] = { QChar::fromLatin1(char(key)), QChar::Null };
            emit ctrlCommand(QStringList({"keydown", QString::fromRawData(str, 1)}));
        }
    }
}

void MpvObject::self_keyRelease(int key)
{
    if (sendKeyEvents) {
        if (key >= 0x20 && key <= 0x7f) {
            QChar str[2] = { QChar::fromLatin1(char(key)), QChar::Null };
            emit ctrlCommand(QStringList({"keyup", QString::fromRawData(str, 1)}));
        }
    }
}

void MpvObject::hideTimer_timeout()
{
    hideCursor();
}

//----------------------------------------------------------------------------

MpvWidgetInterface::MpvWidgetInterface(MpvObject *object)
    : mpvObject(object)
{
    ctrl = object->controller();
}

MpvWidgetInterface::~MpvWidgetInterface()
{
    ctrl = nullptr;
}

void MpvWidgetInterface::setController(MpvController *controller)
{
    ctrl = controller;
}

void MpvWidgetInterface::setLogoUrl(const QString &filename)
{
    Q_UNUSED(filename)
}

void MpvWidgetInterface::setLogoBackground(const QColor &color)
{
    Q_UNUSED(color)
}

void MpvWidgetInterface::setDrawLogo(bool yes)
{
    Q_UNUSED(yes)
}

//----------------------------------------------------------------------------

static void* GLAPIENTRY glMPGetNativeDisplay(const char* name)
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    if (!strcmp(name, "x11")) {
        auto x11App = qApp->nativeInterface<QNativeInterface::QX11Application>();
        return x11App ? x11App->display() : nullptr;
    }
#elif defined(Q_OS_WIN)
    if (!strcmp(name, "IDirect3DDevice9")) {
        // Do something here ?
    }
#else
    Q_UNUSED(name);
#endif
    return nullptr;
}

 void *MpvGlWidget::get_proc_address(void *ctx, const char *name)
 {
    (void)ctx;
    auto glctx = QOpenGLContext::currentContext();
    if (!strcmp(name, "glMPGetNativeDisplay"))
        return reinterpret_cast<void*>(&glMPGetNativeDisplay);
    void * res = glctx ? reinterpret_cast<void*>(glctx->getProcAddress(QByteArray(name))) : nullptr;
    if (!res)
        LogStream(logModule) << "Looked up OpenGL function " << name << ", but it was not available.";
    return res;
}

MpvGlWidget::MpvGlWidget(MpvObject *object, QWidget *parent) :
    QOpenGLWidget(parent), MpvWidgetInterface(object)
{
    connect(this, &QOpenGLWidget::frameSwapped,
            this, &MpvGlWidget::self_frameSwapped);
    connect(mpvObject, &MpvObject::playbackStarted,
            this, &MpvGlWidget::self_playbackStarted);
    connect(mpvObject, &MpvObject::playbackFinished,
            this, &MpvGlWidget::self_playbackFinished);
    setContextMenuPolicy(Qt::CustomContextMenu);
}

MpvGlWidget::~MpvGlWidget()
{
    makeCurrent();
    if (render) {
        ctrl->destroyRenderContext(render);
        render = nullptr;
    }
    if (logo) {
        delete logo;
        logo = nullptr;
    }
    doneCurrent();
}

QWidget *MpvGlWidget::self()
{
    return this;
}

void MpvGlWidget::initMpv()
{
    // this takes place in initializeGL
}


void MpvGlWidget::setLogoUrl(const QString &filename)
{
    makeCurrent();
    if (!logo) {
        logo = new LogoDrawer(this);
        connect(logo, &LogoDrawer::logoSize,
                mpvObject, &MpvObject::logoSizeChanged);
    }
    logo->setLogoUrl(filename);
    logo->resizeGL(width(), height(), devicePixelRatioF());
    if (drawLogo)
        update();
    doneCurrent();
}

void MpvGlWidget::setLogoBackground(const QColor &color)
{
    logo->setLogoBackground(color);
}

void MpvGlWidget::setDrawLogo(bool yes)
{
    drawLogo = yes;
    update();
}

void MpvGlWidget::initializeGL()
{
    if (!logo)
        logo = new LogoDrawer(this);

#if MPV_CLIENT_API_VERSION < MPV_MAKE_VERSION(2,0)
    mpv_opengl_init_params glInit { &get_proc_address, this, nullptr };
#else
    mpv_opengl_init_params glInit { &get_proc_address, this };
#endif
    mpv_render_param params[] {
        { MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL) },
        { MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &glInit },
        { MPV_RENDER_PARAM_INVALID, nullptr },
        { MPV_RENDER_PARAM_INVALID, nullptr }
    };
    QWidget *nativeParent = nativeParentWidget();
    if (nativeParent == nullptr) {
        Logger::log(logModule, "no native parent handle");
    }
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    else if (auto x11App = qApp->nativeInterface<QNativeInterface::QX11Application>()) {
        Logger::log(logModule, "assigning X11 display");
        params[2].type = MPV_RENDER_PARAM_X11_DISPLAY;
        params[2].data = x11App->display();
    }
#if QT_VERSION >= QT_VERSION_CHECK(6,5,0)
    else if (auto wlApp = qApp->nativeInterface<QNativeInterface::QWaylandApplication>()) {
        Logger::log(logModule, "assigning wayland display");
        params[2].type = MPV_RENDER_PARAM_WL_DISPLAY;
        params[2].data = wlApp->display();
    }
#endif // is qt >= 6.5
#endif // is Linux
    else {
        Logger::log(logModule, "unknown display mode (eglfs et al)");
    }

    render = ctrl->createRenderContext(params);
    mpv_render_context_set_update_callback(render, MpvGlWidget::render_update, this);
}

void MpvGlWidget::paintGL()
{
    if (mpvObject->clientDebuggingMessages())
        Logger::log(logModule, "paintGL");
    if (drawLogo || !render) {
        if (logo)
            logo->paintGL(this);
        return;
    }

    int yes = 1; //true
    mpv_opengl_fbo fbo { static_cast<int>(defaultFramebufferObject()), glWidth, glHeight, 0 };
    mpv_render_param params[] {
        { MPV_RENDER_PARAM_OPENGL_FBO, &fbo },
        { MPV_RENDER_PARAM_FLIP_Y, &yes },
        { MPV_RENDER_PARAM_INVALID, nullptr }
    };
    mpv_render_context_render(render, params);
}

void MpvGlWidget::resizeGL(int w, int h)
{
    qreal r = devicePixelRatioF();
    glWidth = int(w * r);
    glHeight = int(h * r);
    logo->resizeGL(width(),height(), devicePixelRatioF());
}

void MpvGlWidget::mouseMoveEvent(QMouseEvent *event)
{
    QPointF pos = event->position();
    emit mpvObject->mouseMoved(pos.x()*devicePixelRatioF(),
                               pos.y()*devicePixelRatioF());
    event->accept();
    int e = 0;
    if (!window()->isFullScreen() && !window()->isMaximized()) {
        if (pos.x() < 10) { e |= Qt::LeftEdge; }
        if (pos.x() > width() - 10) { e |= Qt::RightEdge; }
        if (pos.y() < 10) { e |= Qt::TopEdge; }
        if (pos.y() > height() - 10) { e |= Qt::BottomEdge; }

        if (e == 0) {
            setCursor(Qt::ArrowCursor);
            if (!windowDragging && event->buttons().testAnyFlag(Qt::LeftButton)
                && event->position() != mousePressPosition) {
                QWindow *parentWindow = this->window()->windowHandle();
                parentWindow->startSystemMove();
                windowDragging = true;
            }
        }
        else {
            if ((e & Qt::LeftEdge && e & Qt::BottomEdge) || (e & Qt::RightEdge && e & Qt::TopEdge))
                setCursor(Qt::SizeBDiagCursor);
            else if ((e & Qt::LeftEdge && e & Qt::TopEdge) || (e & Qt::RightEdge && e & Qt::BottomEdge))
                setCursor(Qt::SizeFDiagCursor);
            else if (e & Qt::LeftEdge || e & Qt::RightEdge)
                setCursor(Qt::SizeHorCursor);
            else if (e & Qt::TopEdge || e & Qt::BottomEdge)
                setCursor(Qt::SizeVerCursor);
        }
    }
    this->resizeEdge = e;
}

void MpvGlWidget::mousePressEvent(QMouseEvent *event)
{
    mousePressPosition = event->position();
    windowDragging = false;
    QPointF pos = event->position();
    int btn = int(log2(double(event->button()) + 0.5));
    emit mpvObject->mousePress(pos.x()*devicePixelRatioF(),
                               pos.y()*devicePixelRatioF(),
                               btn);
    QOpenGLWidget::mousePressEvent(event);
    if (this->resizeEdge != 0)
        window()->windowHandle()->startSystemResize((Qt::Edge) this->resizeEdge);
}

void MpvGlWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (windowDragging) {
        windowDragging = false;
        event->accept();
        return;
    }
    QOpenGLWidget::mouseReleaseEvent(event);
    emit mpvObject->mouseReleased();
}

void MpvGlWidget::keyPressEvent(QKeyEvent *event)
{
    emit mpvObject->keyPress(event->key());
}

void MpvGlWidget::keyReleaseEvent(QKeyEvent *event)
{
    emit mpvObject->keyRelease(event->key());
}

void MpvGlWidget::render_update(void *ctx)
{
    QMetaObject::invokeMethod(static_cast<MpvGlWidget*>(ctx), "maybeUpdate");
}

void MpvGlWidget::maybeUpdate()
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

void MpvGlWidget::self_frameSwapped()
{
    if (render && !drawLogo)
        mpv_render_context_report_swap(render);
}

void MpvGlWidget::self_playbackStarted()
{
    drawLogo = false;
}

void MpvGlWidget::self_playbackFinished()
{
    drawLogo = true;
    update();
}



MpvCallback::MpvCallback(const Callback &callback,
                         QObject *owner)
    : QObject(owner)
{
    this->callback = callback;
}

void MpvCallback::reply(const QVariant &value)
{
    callback(value);
    deleteLater();
}

//----------------------------------------------------------------------------

MpvController::MpvController(QObject *parent) : QObject(parent),
    lastVideoSize(0,0)
{
    throttler = new QTimer(this);
    connect(throttler, &QTimer::timeout,
            this, &MpvController::flushProperties);
    throttler->setInterval(1000/12);
    throttler->start();
}

MpvController::~MpvController()
{
    stop();
    throttler->deleteLater();
}

void MpvController::create(const OptionList &earlyOptions)
{
    mpv = mpv::qt::Handle::FromRawHandle(mpv_create());
    if (!mpv)
        throw std::runtime_error("could not create mpv context");

    // Certain things like encoding options and input server need to be
    // set _before_ mpv initialize.
    for (const MpvOption &option : earlyOptions)
        setOptionVariant(option.name, option.value);

    if (mpv_initialize(mpv) < 0)
        throw std::runtime_error("could not initialize mpv context");

    mpv_set_wakeup_callback(mpv, MpvController::mpvWakeup, this);
    protocolList_ = getPropertyVariant("protocol-list").toStringList();
}

void MpvController::stop()
{
    mpv_set_wakeup_callback(mpv, nullptr, nullptr);
}

mpv_render_context *MpvController::createRenderContext(mpv_render_param *params)
{
    mpv_render_context *render = nullptr;
    if (mpv_render_context_create(&render, mpv, params) < 0)
        throw std::runtime_error("Could not create render context");
    return render;
}

void MpvController::destroyRenderContext(mpv_render_context *render)
{
    mpv_render_context_set_update_callback(render, nullptr, nullptr);
    mpv_render_context_free(render);
}

void MpvController::addHook(const QString &name, uint64_t selfId)
{
    // The caller of this function MUST listen to the hookEvent signal,
    // and when it hears its selfId, it MUST invoke continueHook.
    mpv_hook_add(mpv, selfId, name.toUtf8().data(), 0);
}

void MpvController::continueHook(uint64_t mpvId)
{
    mpv_hook_continue(mpv, mpvId);
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

QStringList MpvController::protocolList()
{
    return protocolList_;
}

int64_t MpvController::timeMicroseconds()
{
    return mpv_get_time_us(mpv);
}

unsigned long MpvController::apiVersion()
{
    return mpv_client_api_version();
}

void MpvController::setLogLevel(QString logLevel)
{
    mpv_request_log_messages(mpv, logLevel.toUtf8().data());
}

void MpvController::showStatsPage(int page)
{
    bool statsVisible = shownStatsPage > 0;
    bool wantVisible = page > 0;
    if (wantVisible != statsVisible) {
        Logger::log(logModule, "toggling stats page");
        command(QStringList({"script-binding",
                             "stats/display-stats-toggle"}));
    }
    if (wantVisible) {
        LogStream(logModule) << "setting page to " << page;
        QString pageCommand("stats/display-page-%1");
        command(QStringList({"script-binding",
                             pageCommand.arg(QString::number(page))}));
    }
    shownStatsPage = page;
}

int MpvController::setOptionVariant(QString name, const QVariant &value)
{
    return mpv::qt::set_option_variant(mpv, name, value);
}

QVariant MpvController::command(const QVariant &params)
{
    if (params.canConvert<QString>() && params.metaType().id() != QMetaType::QStringList) {
        int value = mpv_command_string(mpv, params.toString().toUtf8().data());
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
    for (auto it = throttledValues.begin(); it != throttledValues.end(); it++)
        emit mpvPropertyChanged(it.key(), it.value().first, it.value().second);
    throttledValues.clear();
}

void MpvController::handleMpvEvent(mpv_event *event)
{
    auto propertyToVariant = [event](mpv_event_property *prop) -> QVariant {
        auto asBool = [&](bool dflt = false) {
            return (prop->format != MPV_FORMAT_FLAG || prop->data == nullptr) ?
                        dflt : *static_cast<bool*>(prop->data);
        };
        auto asDouble = [&](double dflt = nan("")) {
            return (prop->format != MPV_FORMAT_DOUBLE || prop->data == nullptr) ?
                        dflt : *static_cast<double*>(prop->data);
        };
        auto asInt64 = [&](int64_t dflt = -1) {
            return (prop->format != MPV_FORMAT_INT64 || prop->data == nullptr) ?
                        dflt : *static_cast<int64_t*>(prop->data);
        };
        auto asString = [&](QString dflt = QString()) {
            return (!(prop->format == MPV_FORMAT_STRING ||
                      prop->format == MPV_FORMAT_OSD_STRING) ||
                    prop->data == nullptr) ?
                        dflt : QString(*static_cast<char**>(prop->data));
        };
        auto asNode = [&](QVariant dflt = QVariant()) {
            return (prop->format != MPV_FORMAT_NODE || prop->data == nullptr) ?
                        dflt : mpv::qt::node_to_variant(
                            static_cast<mpv_node*>(prop->data));
        };
        if (prop->data == nullptr) {
            return QVariant::fromValue<MpvErrorCode>(MpvErrorCode(event->error));
        } else if (prop->format == MPV_FORMAT_NODE) {
            return asNode();
        } else if (prop->format == MPV_FORMAT_INT64) {
            return qlonglong(asInt64());
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
        QVariant v = propertyToVariant(static_cast<mpv_event_property*>(event->data));
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
        QVariant v = propertyToVariant(static_cast<mpv_event_property*>(event->data));
        QString propname = QString::fromUtf8(static_cast<mpv_event_property*>(event->data)->name);
        if (throttledProperties.contains(propname))
            setThrottledProperty(propname, v, event->reply_userdata);
        else
            emit mpvPropertyChanged(propname, v, event->reply_userdata);
        break;
    }
    case MPV_EVENT_LOG_MESSAGE: {
        mpv_event_log_message *msg =
                static_cast<mpv_event_log_message*>(event->data);
        emit logMessageByParts(msg->prefix, msg->level, msg->text);
        break;
    }
    case MPV_EVENT_CLIENT_MESSAGE: {
        mpv_event_client_message *msg =
                static_cast<mpv_event_client_message*>(event->data);
        QStringList list;
        for (int i = 0; i < msg->num_args; i++)
            list.append(msg->args[i]);
        emit clientMessage(event->reply_userdata, list);
        break;
    }
    case MPV_EVENT_VIDEO_RECONFIG: {
        // Retrieve the new video size.
        QVariant vw, vh;
        vw = getPropertyVariant("dwidth");
        vh = getPropertyVariant("dheight");
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
    case MPV_EVENT_HOOK: {
        mpv_event_hook *msg = static_cast<mpv_event_hook*>(event->data);
        emit hookEvent(msg->name, event->reply_userdata, msg->id);
        break;
    }
    default:
        emit unhandledMpvEvent(event->event_id);
    }
}

void MpvController::mpvWakeup(void *ctx)
{
    QMetaObject::invokeMethod(static_cast<MpvController*>(ctx), "parseMpvEvents",
                              Qt::QueuedConnection);
}
