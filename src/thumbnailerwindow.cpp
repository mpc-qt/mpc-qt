#include <QFileDialog>
#include <QFont>
#include <QFontMetrics>
#include <QOpenGLContext>
#include <QPainter>
#include <QTimer>
#include "helpers.h"
#include "logger.h"
#include "thumbnailerwindow.h"
#include "ui_thumbnailerwindow.h"

// NOTE: do we configure the thumb format in the settings dialog?
static constexpr char logModule[] = "thumbnailer";
static constexpr char friendlyName[] = "Media Player Classic Qute Theater - Thumbnailer";
static constexpr char thumbFormat[] = "%f_thumbs_[%t{yyyy.MM.dd_hh.mm.ss}]";
static constexpr char saveDialogFilter[] = "Images (*.png *.jpg *.jpeg)";

static constexpr int thumbMargin = 8;
static constexpr int pageMargin = 10;
static constexpr int captionPadding = 10;
static constexpr int rhsPadding = pageMargin - thumbMargin;
static constexpr int emptySpace = pageMargin + rhsPadding;
static constexpr int pageMarginSum = pageMargin * 2;
static constexpr int thumbShadow = 4;
static constexpr int timerWaitMsec = 500;
static constexpr int osdFontSize = 12;
static constexpr int osdFontShadow = 2;

ThumbnailerWindow::ThumbnailerWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ThumbnailerWindow)
{
    Logger::log(logModule, "creating ui");
    ui->setupUi(this);
    Logger::log(logModule, "finished creating ui");
    connect(ui->actionGo, &QPushButton::clicked,
            this, &ThumbnailerWindow::begin);

    thumbnailer = new MpvThumbnailer(this);
    connect(thumbnailer, &MpvThumbnailer::progress,
            this, &ThumbnailerWindow::thumbnailer_setProgress);
    connect(thumbnailer, &MpvThumbnailer::finished,
            this, &ThumbnailerWindow::thumbnailer_finished);
}

ThumbnailerWindow::~ThumbnailerWindow()
{
    delete ui;
}

void ThumbnailerWindow::setScreenshotDirectory(QString folder)
{
    screenshotDirectory = folder;
}

void ThumbnailerWindow::setScreenshotFormat(QString format)
{
    screenshotFormat = format;
}

void ThumbnailerWindow::on_saveImageBrowse_clicked()
{
    static QFileDialog::Options options = QFileDialog::Options();
#ifdef Q_OS_MAC
    options = QFileDialog::DontUseNativeDialog;
#endif
    QString filename = QFileDialog::getSaveFileName(this, "Save Thumbnails",
                                                    ui->saveImage->text(),
                                                    saveDialogFilter,
                                                    nullptr, options);
    if (filename.isEmpty())
        return;
    ui->saveImage->setText(filename);
}

void ThumbnailerWindow::thumbnailer_setProgress(int percent)
{
    ui->actionProgress->setValue(percent);
}

void ThumbnailerWindow::thumbnailer_finished()
{
    setEnabled(true);
    hide();
}

void ThumbnailerWindow::open(QUrl sourceUrl)
{
    //QString displayText = sourceUrl.fileName();
    QString saveFile = Helpers::parseFormatEx(thumbFormat, sourceUrl, screenshotDirectory, screenshotFormat,
                                              Helpers::NothingDisabled, Helpers::SubtitlesDisabled,
                                              0.0, 0.0, 0.0);
    ui->mediaSource->setText(sourceUrl.toString());
    ui->saveImage->setText(saveFile);
    ui->actionProgress->setValue(0);
    show();
    raise();
}

void ThumbnailerWindow::begin()
{
    setEnabled(false);

    MpvThumbnailer::Params p;
    p.sourceUrl = ui->mediaSource->text();
    p.imageFile = ui->saveImage->text();
    p.jpegQuality = ui->imageQuality->value();
    p.imageWidth = ui->imageWidth->value();
    p.cols = ui->layoutColumns->value();
    p.rows = ui->layoutRow->value();
    thumbnailer->execute(p);

}

MpvThumbnailer::MpvThumbnailer(QObject *parent)
    : QObject(parent)
{

}

MpvThumbnailer::~MpvThumbnailer()
{
    deinitPlayer();
}

void MpvThumbnailer::execute(const MpvThumbnailer::Params &p)
{
    if (thumbState != AvailableState) {
        Logger::log(logModule, "tried to start with an already started thumbnailer");
        return;
    }
    thumbState = StartedState;

    initPlayer();
    LogStream(logModule) << "starting thumbnailing process for " << p.sourceUrl;
    this->p = p;
    pendingPts.clear();
    processedPts.clear();

    emit mpv->ctrlSetOptionVariant("profile", "gpu-hq");
    emit mpv->ctrlSetOptionVariant("blend-subtitles", "video");
    emit mpv->ctrlSetOptionVariant("sub-visibility", "no");
    emit mpv->ctrlSetOptionVariant("osd-align-x", "right");
    emit mpv->ctrlSetOptionVariant("osd-align-y", "bottom");
    emit mpv->ctrlSetOptionVariant("osd-duration", "3000");
    emit mpv->ctrlSetOptionVariant("osd-color", "#80FFFFFF");
    emit mpv->ctrlSetOptionVariant("osd-border-color", "#80000000");
    emit mpv->ctrlSetOptionVariant("osd-bold", "yes");
    emit mpv->ctrlSetOptionVariant("ao", "null");
    emit mpv->ctrlSetOptionVariant("ao-null-untimed", "yes");
    emit mpv->ctrlSetOptionVariant("audio", "no");
    emit mpv->ctrlSetOptionVariant("untimed", "yes");
    emit mpv->ctrlSetOptionVariant("fps", 200);
    mpv->urlOpen(p.sourceUrl);
    mpv->setPaused(true);
    emit progress(0);
}

void MpvThumbnailer::initPlayer()
{
    mpv = new MpvObject(this, friendlyName);
    thumbnailer = new MpvThumbnailDrawer(mpv);
    mpv->setWidgetType(Helpers::CustomWidget, thumbnailer);
    connect(mpv, &MpvObject::fileSizeChanged,
            this, &MpvThumbnailer::mpv_fileSizeChanged);
    connect(mpv, &MpvObject::playbackFinished,
            this, &MpvThumbnailer::mpv_playbackFinished);
    connect(mpv, &MpvObject::eofReachedChanged,
            this, &MpvThumbnailer::mpv_eofReachedChanged);
    connect(mpv, &MpvObject::playLengthChanged,
            this, &MpvThumbnailer::mpv_playLengthChanged);
    connect(mpv, &MpvObject::playTimeChanged,
            this, &MpvThumbnailer::mpv_playTimeChanged);
    connect(mpv, &MpvObject::videoSizeChanged,
            this, &MpvThumbnailer::mpv_videoSizeChanged);
    thumbnailer->setAttribute(Qt::WA_DontShowOnScreen);
    thumbnailer->show();
}

void MpvThumbnailer::deinitPlayer()
{
    if (!mpv)
        return;
    mpv->setWidgetType(Helpers::NullWidget);
    thumbnailer = nullptr;

    delete mpv;
    mpv = nullptr;
}

void MpvThumbnailer::initThumbPts()
{
    int total = p.rows * p.cols;
    int index = 1;
    int dx = (p.imageWidth - emptySpace)/p.cols;
    int dy = thumbSize.height() + thumbMargin;

    pendingPts.clear();
    processedPts.clear();
    for (int r = 0; r < p.rows; r++) {
        for (int c = 0; c < p.cols; c++) {
            pendingPts.enqueue({c*dx, r*dy,
                                (mpvDuration * index) / (total+1),
                                index * 100 / total, index,
                                QImage()});
            index++;
        }
    }
}

void MpvThumbnailer::processThumb()
{
    if (pendingPts.isEmpty()) {
        Logger::log(logModule, "tried to process a thumb but there's nothing here");
        return;
    }
    ThumbPts &front = pendingPts.front();
    LogStream(logModule) << "Processing slide " << front.index
                         << "(" << front.percent << "%)";
    emit progress(front.percent);
    front.thumb = thumbnailer->grabFramebuffer();
    processedPts.enqueue(pendingPts.dequeue());
}

bool MpvThumbnailer::seekNextFrame()
{
    if (pendingPts.isEmpty())
        return false;
    LogStream(logModule) << "seeking to " << pendingPts.front().pts;
    mpv->setTime(pendingPts.front().pts);
    mpv->showMessage(Helpers::toDateFormatFixed(pendingPts.front().pts, osdTimeFormat));
    thumbState = SeekingState;
    return true;
}

void MpvThumbnailer::renderImage()
{
    Logger::log(logModule, "rendering thumbnail image");

    QFont blurbFont("Helvetica", 12);
    QFontMetrics blurbMetrics(blurbFont);
    QString blurb = "File Name: %1\n" "File Size: %2\n" "Resolution: %3x%4\n" "Duration: %5";
    blurb = blurb.arg(p.sourceUrl.fileName(),
                      Helpers::fileSizeToString(mpvFileSize),
                      QString::number(mpvVideoSize.width()),
                      QString::number(mpvVideoSize.height()),
                      Helpers::toDateFormat(mpvDuration));

    // Calculate blurb size and therefore caption area
    QRect oversizeRect = QRect(0, 0, p.imageWidth, p.imageWidth);
    QRect blurbRect = blurbMetrics.boundingRect(oversizeRect, 0, blurb);
    QRect captionArea = QRect(pageMargin, 0,
                              p.imageWidth - pageMarginSum,
                              blurbRect.height() + pageMargin + captionPadding);

    // Calculate size of self label to fit the caption area
    QFont selfFont("Helvetica", 1000, QFont::Black);
    QFontMetrics selfMetrics1000(selfFont);
    selfFont.setPixelSize((captionArea.height() - (pageMargin + captionPadding))
                          * selfMetrics1000.height() / selfMetrics1000.ascent());
    QFontMetrics selfMetrics(selfFont);
    QString self = "MPC-QT";
    QRect selfRect = captionArea.adjusted(0, -selfMetrics.descent(),
                                          -pageMargin, selfMetrics.descent());

    // Create new image with size.
    // h = captionbottom + margin-thumbmargin + (thumb+thumbmargin)*rows
    QSize imageSize(p.imageWidth, captionArea.bottom() + rhsPadding +
                    ((thumbSize.height()+ thumbMargin) * p.rows));
    render = QImage(imageSize, QImage::Format_RGB32);
    render.fill(QColor(0xef, 0xee, 0xec));
    QPainter p(&render);

    // Draw texts
    p.setPen(QColor(0xfc, 0xfb, 0xfa));
    p.setFont(selfFont);
    p.drawText(selfRect, Qt::AlignRight | Qt::AlignVCenter, self);

    p.setPen(QColor(0x10, 0x0f, 0x0d));
    p.setFont(blurbFont);
    p.drawText(blurbRect.translated(pageMargin, pageMargin), 0, blurb);

    // Draw thumbnails
    for (auto &t : processedPts) {
        QRectF rc({int(t.x) + 0.5, int(t.y) + 0.5}, t.thumb.size());
        rc.translate(pageMargin, captionArea.bottom());
        p.fillRect(rc.translated(thumbShadow, thumbShadow), {0xbc, 0xbb, 0xba});
        p.fillRect(rc.adjusted(-1,-1,1,1), {0x8c, 0x8b, 0x8a});
        p.drawImage(rc.topLeft().toPoint(), t.thumb);
    }
}

void MpvThumbnailer::saveImage()
{
    LogStream(logModule) << "saving thumbnails to " << p.imageFile;
    if (!render.save(p.imageFile, nullptr, p.jpegQuality))
        Logger::log(logModule, "file was not saved. Is the filename correct?");
    render.detach();
}

void MpvThumbnailer::mpv_fileSizeChanged(int64_t bytes)
{
    mpvFileSize = bytes;
}

void MpvThumbnailer::mpv_videoSizeChanged(QSize video)
{
    mpvVideoSize = video;
    if (mpvVideoSize == QSize(-1,-1))
        return;

    // Resize thumbnail
    auto safeDiv = [](double u, double v) { return std::max(1.0,u) / std::max(1.0,v); };
    double aRatio = safeDiv(video.width(), video.height());
    int availPx = (p.imageWidth - emptySpace)/p.cols - thumbMargin;
    int h = int(availPx / aRatio + 0.5);
    int w = int(h * aRatio + 0.5);
    thumbSize = QSize(w, h);
    thumbnailer->resize(thumbSize);

    // Set a consistent size for the osd message
    double factor = safeDiv(mpvVideoSize.height(), h);
    emit mpv->ctrlSetOptionVariant("osd-font-size", int(osdFontSize * factor));
    emit mpv->ctrlSetOptionVariant("osd-border-size", int(osdFontShadow * factor));

    if (thumbState == StaleState) {
        // video size was not valid at first navigation,
        // so initialize our stuff now.
        initThumbPts();
        seekNextFrame();
    }
}

void MpvThumbnailer::mpv_playLengthChanged(double length)
{
    mpvDuration = length;
    osdTimeFormat = length < 3600.0 ? Helpers::ShortHourFormat
                                    : Helpers::ShortFormat;
}

void MpvThumbnailer::mpv_playTimeChanged(double time)
{
    Logger::log("MpvThumbnailer::mpv_playTimeChanged");
    LogStream(logModule) << "thumbState: " << thumbState;
    LogStream(logModule) << "time: " << time;
    // This function is called:
    // * Once at file open with timestamp 0
    // * Once per seek navigation with the requested time (sometimes)
    // * Once with a msec accurate value (soon after seek navigation)
    //
    // Therefore when we get the seek navigation, the render pipeline may not
    // have caught up and may be stale.  But we can't just ignore the first
    // time change notification since that sometimes is skipped.  And there
    // will be some file out there where our divided pts will line up with a
    // msec exactly, so we can't just check if the pts is the same either.
    // Therefore, we need to either use a longish timer to give time for
    // frames to render (ha ha), or use some other potentionally more correct
    // option.  Perhaps core-idle?  Patches welcome.
    mpvTime = time;
    if (time < 0) {
        return;
    }

    if (thumbState == StartedState) {
        if (mpvVideoSize == QSize(-1,-1)) {
            thumbState = StaleState;
            return;
        }
        initThumbPts();
        seekNextFrame();
        return;
    }

    if (thumbState == SeekingState) {
        //Logger::log(logModule, "ignored the seek navigation");
        thumbState = PlayingState;
        //return;
    }

    if (thumbState == PlayingState) {
        thumbState = WaitingForTimer;
        QTimer::singleShot(timerWaitMsec, this, &MpvThumbnailer::timer_navigateTick);
    }
}

void MpvThumbnailer::mpv_playbackFinished()
{
    thumbState = FinishedState;
}

void MpvThumbnailer::mpv_eofReachedChanged()
{
    if (thumbState != FinishedState)
        return;
    deinitPlayer();
    thumbState = AvailableState;
    emit finished();
}

void MpvThumbnailer::timer_navigateTick()
{
    processThumb();
    if (seekNextFrame())
        return;

    renderImage();
    saveImage();
    mpv->stopPlayback();
}



MpvThumbnailDrawer::MpvThumbnailDrawer(MpvObject *object)
    : QOpenGLWidget(nullptr), MpvWidgetInterface(object)
{
    setWindowTitle(friendlyName);
}

MpvThumbnailDrawer::~MpvThumbnailDrawer()
{
    makeCurrent();
    if (render) {
        ctrl->destroyRenderContext(render);
        render = nullptr;
    }
    doneCurrent();
}

void MpvThumbnailDrawer::setLogoUrl(const QString &filename)
{
    Q_UNUSED(filename)
}

void MpvThumbnailDrawer::setLogoBackground(const QColor &color)
{
    Q_UNUSED(color)
}

void MpvThumbnailDrawer::setDrawLogo(bool yes)
{
    Q_UNUSED(yes)
}

void MpvThumbnailDrawer::initializeGL()
{
#if MPV_CLIENT_API_VERSION < MPV_MAKE_VERSION(2,0)
    mpv_opengl_init_params glInit { &MpvGlWidget::get_proc_address, this, nullptr };
#else
    mpv_opengl_init_params glInit { &MpvGlWidget::get_proc_address, this };
#endif
    mpv_render_param params[] {
        { MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL) },
        { MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &glInit },
        { MPV_RENDER_PARAM_INVALID, nullptr },
    };
    render = ctrl->createRenderContext(params);
    mpv_render_context_set_update_callback(render, MpvThumbnailDrawer::render_update, this);
}

void MpvThumbnailDrawer::paintGL()
{
    if (!render) {
        Logger::log(logModule, "tried to draw a frame, but no renderer set");
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
    emit renderedFrame();
}

void MpvThumbnailDrawer::resizeGL(int w, int h)
{
    qreal r = devicePixelRatioF();
    glWidth = int(w * r);
    glHeight = int(h * r);
}

void MpvThumbnailDrawer::alwaysUpdate()
{
    makeCurrent();
    paintGL();
    context()->swapBuffers(context()->surface());
    mpv_render_context_report_swap(render);
    doneCurrent();
}

void MpvThumbnailDrawer::render_update(void *ctx)
{
    QMetaObject::invokeMethod(static_cast<MpvThumbnailDrawer*>(ctx), "alwaysUpdate");
}

void MpvThumbnailDrawer::initMpv()
{

}

QWidget *MpvThumbnailDrawer::self()
{
    return this;
}

