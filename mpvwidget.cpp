// Portions of code in this module came from the examples directory in mpv's
// git repo.  Reworked by me.

#include <QOpenGLContext>
#include <QMetaObject>
#include <QJsonDocument>
#include <QDir>
#include <QDebug>
#include <cmath>
#include <mpv/qthelper.hpp>
#include "mpvwidget.h"

static void wakeup(void *ctx)
{
    // Notify the Qt GUI thread to wake up so that it can process events with
    // mpv_wait_event().
    MpvWidget *widget= (MpvWidget*)ctx;
    QMetaObject::invokeMethod(widget, "mpvEvents", Qt::QueuedConnection);
}

static void *get_proc_address(void *ctx, const char *name) {
    (void)ctx;
    auto glctx = QOpenGLContext::currentContext();
    return glctx ? (void*)glctx->getProcAddress(QByteArray(name)) : NULL;
}

MpvWidget::MpvWidget(QWidget *parent) :
    QOpenGLWidget(parent), drawLogo(true), logo(NULL),
    logoUrl(":/images/bitmaps/blank-screen.png")
{
#ifdef QT_DEBUG
    debugMessages = true;
#else
    debugMessages = false;
#endif
    mpv = mpv::qt::Handle::FromRawHandle(mpv_create());
    if (!mpv)
        throw std::runtime_error("[MpvWidget] Can't create mpv instance");

    // When (un)docking windows, some widgets may get transformed into native
    // widgets, causing painting glitches.  Tell Qt that we prefer non-native.
    setAttribute(Qt::WA_DontCreateNativeAncestors);

    // Let us receive property change events with MPV_EVENT_PROPERTY_CHANGE if
    // this property changes.
    QVector<QPair<const char*, mpv_format>> options = {
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
        { "paused-for-cache", MPV_FORMAT_FLAG }
    };
    foreach (auto item, options)
        mpv_observe_property(mpv, 0, item.first, item.second);

    // Request log messages with level "info" or higher.
    // They are received as MPV_EVENT_LOG_MESSAGE.
    mpv_request_log_messages(mpv, "info");

    // From this point on, the wakeup function will be called. The callback
    // can come from any thread, so we use the QueuedConnection mechanism from
    // the callback to relay the wakeup in a thread-safe way.
    mpv_set_wakeup_callback(mpv, wakeup, this);

    // Initialize non-managed settings
    setMpvOptionString("vo","opengl-cb");
    setMpvOptionString("ytdl", "yes");

    if (mpv_initialize(mpv) < 0)
        throw std::runtime_error("[MpvWidget] mpv failed to initialize");

    glMpv = reinterpret_cast<mpv_opengl_cb_context*>(
                mpv_get_sub_api(mpv, MPV_SUB_API_OPENGL_CB));
    if (!glMpv)
        throw std::runtime_error("[MpvWidget] OpenGL not compiled in");

    mpv_opengl_cb_set_update_callback(glMpv, MpvWidget::mpvw_update,
                                      reinterpret_cast<void*>(this));
    connect(this, &QOpenGLWidget::frameSwapped,
            this, &MpvWidget::self_frameSwapped);
    connect(this, &MpvWidget::playbackStarted,
            this, &MpvWidget::self_playbackStarted);
    connect(this, &MpvWidget::playbackFinished,
            this, &MpvWidget::self_playbackFinished);
}

MpvWidget::~MpvWidget()
{
    makeCurrent();
    if (glMpv) {
        mpv_opengl_cb_set_update_callback(glMpv, NULL, NULL);
        mpv_opengl_cb_uninit_gl(glMpv);
    }
    if (logo)
        delete logo;
}

void MpvWidget::showMessage(QString message)
{
    if (!mpv) return;
    mpv::qt::command_variant(mpv, QVariantList({"show_text", message, "1000"}));
}

void MpvWidget::fileOpen(QString filename)
{
    if (!mpv) return;
    const QByteArray c_filename = filename.toUtf8();
    const char *args[] = {"loadfile", c_filename.data(), NULL};
    mpv_command_async(mpv, 0, args);
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
    mpv_command_string(mpv, "frame_back_step");
}

void MpvWidget::stepForward()
{
    mpv_command_string(mpv, "frame_step");
}

void MpvWidget::setVOCommandLine(QString cmdline)
{
    qDebug() << "got advanced command line " << cmdline;
    mpv::qt::command_variant(mpv, QVariantList({"vo-cmdline", cmdline}));
}

void MpvWidget::stopPlayback()
{
    if (!mpv) return;
    mpv_command_string(mpv, "stop");
}

int64_t MpvWidget::chapter()
{
    if (!mpv) return 0;
    int64_t chapter = 0;
    getMpvProperty("chapter", MPV_FORMAT_INT64, &chapter);
    return chapter;
}

bool MpvWidget::setChapter(int64_t chapter)
{
    if (!mpv) return false;
    return setMpvProperty("chapter", MPV_FORMAT_INT64, &chapter) == 0;
}

QString MpvWidget::mediaTitle()
{
    return getPropertyString("media-title");
}

void MpvWidget::setMute(bool yes)
{
    int64_t flag = yes ? 1 : 0;
    setMpvProperty("mute", MPV_FORMAT_FLAG, &flag);
}

void MpvWidget::setPaused(bool yes)
{
    if (!mpv) return;
    mpv_set_property_string(mpv, "pause", yes ? "yes" : "no");
}

void MpvWidget::setSpeed(double speed)
{
    if (!mpv) return;
    setMpvProperty("speed", MPV_FORMAT_DOUBLE, &speed);
}

void MpvWidget::setTime(double position)
{
    if (!mpv) return;
    setMpvProperty("time-pos", MPV_FORMAT_DOUBLE, &position);
}

void MpvWidget::setAudioTrack(int64_t id)
{
    setMpvProperty("aid", MPV_FORMAT_INT64, &id);
}

void MpvWidget::setSubtitleTrack(int64_t id)
{
    setMpvProperty("sid", MPV_FORMAT_INT64, &id);
}

void MpvWidget::setVideoTrack(int64_t id)
{
    setMpvProperty("vid", MPV_FORMAT_INT64, &id);
}

QString MpvWidget::mpvVersion()
{
    return getPropertyString("mpv-version");
}

void MpvWidget::setVolume(int64_t volume)
{
    if (!mpv) return;
    setMpvProperty("volume", MPV_FORMAT_INT64, &volume);
}

bool MpvWidget::eofReached()
{
    bool result = false;
    getMpvProperty("eof-reached", MPV_FORMAT_FLAG, &result);
    return result;
}

void MpvWidget::setSubsAreGray(bool yes)
{
    if (!mpv) return;
    setMpvOption("sub-gray", MPV_FORMAT_FLAG, &yes);
}

void MpvWidget::setFramedropMode(QString mode)
{
    if (!mpv) return;
    setMpvOptionString("framedrop", mode.toUtf8().constData());
}

void MpvWidget::setDecoderDropMode(QString mode)
{
    if (!mpv) return;
    setMpvOptionString("vd-lavc-framedrop", mode.toUtf8().constData());
}

void MpvWidget::setDisplaySyncMode(QString mode)
{
    if (!mpv) return;
    setMpvOptionString("video-sync", mode.toUtf8().constData());
}

void MpvWidget::setAudioDropSize(double size)
{
    if (!mpv) return;
    setMpvOption("video-sync-adrop-size", MPV_FORMAT_DOUBLE, &size);
}

void MpvWidget::setMaximumAudioChange(double change)
{
    if (!mpv) return;
    setMpvOption("video-sync-max-audio-change", MPV_FORMAT_DOUBLE, &change);
}

void MpvWidget::setMaximumVideoChange(double change)
{
    if (!mpv) return;
    setMpvOption("video-sync-max-video-change", MPV_FORMAT_DOUBLE, &change);
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

void MpvWidget::initializeGL()
{
    if (mpv_opengl_cb_init_gl(glMpv, NULL, get_proc_address, NULL) < 0)
        throw std::runtime_error("[MpvWidget] cb init gl failed.");

    if (!logo)
        logo = new QOpenGLTexture(QImage(logoUrl),
                                  QOpenGLTexture::DontGenerateMipMaps);
}

void MpvWidget::paintGL()
{
    if (!drawLogo) {
        mpv_opengl_cb_draw(glMpv, defaultFramebufferObject(),
                           width(), -height());
    } else {
        glClear(GL_COLOR_BUFFER_BIT);
        glLoadIdentity();
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, logo->textureId());
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);  // eww, quads!
            glTexCoord2f(0,0);
                glVertex2f(logoLocation.left(), logoLocation.bottom());
            glTexCoord2f(1,0);
                glVertex2f(logoLocation.right(), logoLocation.bottom());
            glTexCoord2f(1,1);
                glVertex2f(logoLocation.right(), logoLocation.top());
            glTexCoord2f(0,1);
                glVertex2f(logoLocation.left(), logoLocation.top());
        glEnd();
        glDisable(GL_TEXTURE_2D);
    }
}

void MpvWidget::resizeGL(int w, int h)
{
    float fw = 2.0f/w;
    float fh = 2.0f/h;
    float iw = fw * logo->width();
    float ih = fh * logo->height();
    logoLocation = {-iw/2, -ih/2, iw, ih};
}

void MpvWidget::mpvw_update(void *ctx)
{
    QMetaObject::invokeMethod(reinterpret_cast<MpvWidget*>(ctx), "update");
}

QString MpvWidget::getPropertyString(const char *property)
{
    if (debugMessages)
        qDebug() << "get prop string: " << property;
    QString text;
    char *value;
    if (mpv_get_property(mpv, property, MPV_FORMAT_STRING,
                         &value) == 0) {
        text = QString::fromUtf8(QByteArray(value));
        mpv_free(value);
    }
    return text;
}

int MpvWidget::getMpvProperty(const char *name, mpv_format fmt, void *data)
{
    if (debugMessages)
        qDebug() << "get mpv property " << name;
    return mpv_get_property(mpv, name, fmt, data);
}

int MpvWidget::setMpvProperty(const char *name, mpv_format fmt, void *data)
{
    if (debugMessages)
        qDebug() << "set mpv property " << name;
    return mpv_set_property(mpv, name, fmt, data);
}

int MpvWidget::setMpvOption(const char *name, mpv_format fmt, void *data)
{
    if (debugMessages)
        qDebug() << "set mpv option " << name;
    return mpv_set_option(mpv, name, fmt, data);
}

int MpvWidget::setMpvOptionString(const char *name, const char *string)
{
    if (debugMessages)
        qDebug() << "set mpv option str " << name;
    return mpv_set_option_string(mpv, name, string);
}

int MpvWidget::setMpvPropertyString(const char *name, const char *string)
{
    if (debugMessages)
        qDebug() << "set mpv property str " << name;
    return mpv_set_property_string(mpv, name, string);
}

void MpvWidget::handleMpvEvent(mpv_event *event)
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
        if (debugMessages)
            qDebug() << "prop change " << prop->name;
        // TODO: Refactor this by looking up a map of functions to supported
        // property changes.
        if (strcmp(prop->name, "time-pos") == 0) {
            playTime_ = asDouble();
            playTimeChanged(playTime_);
        } else if (strcmp(prop->name, "duration") == 0) {
            playLength_ = asDouble();
            playLengthChanged(playLength_);
        } else if (strcmp(prop->name, "pause") == 0) {
            pausedChanged(asBool(true));
        } else if (strcmp(prop->name, "media-title") == 0) {
            mediaTitleChanged(asString());
        } else if (strcmp(prop->name, "chapter-metadata") == 0) {
            chapterDataChanged(asNode().toMap());
        } else if (strcmp(prop->name, "chapter-list") == 0) {
            chaptersChanged(asNode().toList());
        } else if (strcmp(prop->name, "track-list") == 0) {
            tracksChanged(asNode().toList());
        } else if (strcmp(prop->name, "estimated-vf-fps") == 0) {
            fpsChanged(asDouble());
        } else if (strcmp(prop->name, "avsync") == 0) {
            avsyncChanged(asDouble());
        } else if (strcmp(prop->name, "vo-drop-frame-count") == 0) {
            displayFramedropsChanged(asInt64());
        } else if (strcmp(prop->name, "drop-frame-count") == 0) {
            decoderFramedropsChanged(asInt64());
        } else {
            // Dump other properties as various formats for development
            // purposes.  Eventually this will never be called as more control
            // features are implemented.
            if (!debugMessages)
                break;
            QString msg = QString("Change property %1: %2").
                    arg(QString(prop->name));
            if (prop->data == NULL) {
                qDebug() << msg.arg("NULL");
            } else if (prop->format == MPV_FORMAT_NODE) {
                QJsonDocument d = QJsonDocument::fromVariant(asNode());
                qDebug() << msg.arg(d.toJson().data());
            } else if (prop->format == MPV_FORMAT_INT64) {
                qDebug() << msg.arg(asInt64());
            } else if (prop->format == MPV_FORMAT_DOUBLE) {
                qDebug() << msg.arg(asDouble());
            } else if (prop->format == MPV_FORMAT_STRING ||
                       prop->format == MPV_FORMAT_OSD_STRING) {
                qDebug() << msg.arg(asString());
            } else if (prop->format == MPV_FORMAT_FLAG) {
                qDebug() << msg.arg(asBool());
            } else {
                qDebug() << msg.arg(QString::number(prop->format));
            }
        }
        break;
    }
    case MPV_EVENT_VIDEO_RECONFIG: {
        if (debugMessages)
            qDebug() << "video reconfig";
        // Retrieve the new video size.
        int64_t w, h;
        if (getMpvProperty("width", MPV_FORMAT_INT64, &w) >= 0 &&
            getMpvProperty("height", MPV_FORMAT_INT64, &h) >= 0 &&
            w > 0 && h > 0)
        {
            videoSize_.setWidth(w);
            videoSize_.setHeight(h);
            if (lastVideoSize != videoSize_)
                videoSizeChanged(videoSize_);
            lastVideoSize = videoSize_;
        }
        break;
    }
    case MPV_EVENT_LOG_MESSAGE: {
        mpv_event_log_message *msg =
                reinterpret_cast<mpv_event_log_message*>(event->data);
        if (debugMessages)
            qDebug() << QString("[%1] %2: %3").arg(msg->prefix, msg->level,
                                                   msg->text);
        break;
    }
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
        playTimeChanged(0);
        playLengthChanged(0);
        playbackFinished();
        break;
    }
    case MPV_EVENT_SHUTDOWN: {
        if (debugMessages)
            qDebug() << "event shutdown";
        playbackFinished();
        break;
    }
    default: ;
        // Ignore uninteresting or unknown events.
    }
}

void MpvWidget::mpvEvents()
{
    // Process all events, until the event queue is empty.
    while (mpv) {
        mpv_event *event = mpv_wait_event(mpv, 0);
        if (event->event_id == MPV_EVENT_NONE)
            break;
        handleMpvEvent(event);
    }
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
}

