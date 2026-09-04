#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include "videopreview.h"

static constexpr char logModule[] =  "videopreview";
static constexpr int previewMarginX = 20;
static constexpr int labelHeight = 20;
static constexpr int shadowMargin = 8;

VideoPreview::VideoPreview(QWidget *parent) : QWidget(parent)
{
    mpv = new MpvObject(this);
    videoContainer = new QWidget(parent);
    auto *shadow = new QGraphicsDropShadowEffect(videoContainer);
    shadow->setBlurRadius(40);
    shadow->setOffset(0);
    shadow->setColor(QColor(0, 0, 0));
    videoContainer->setGraphicsEffect(shadow);

    videoWidget = new MpvGlWidget(mpv, videoContainer);
    mpv->setWidgetType(Helpers::CustomWidget, videoWidget);
    textLabel = new QLabel(videoContainer);
    textLabel->setAlignment(Qt::AlignCenter);

    textLabel->setAutoFillBackground(true);
    updatePalette();

    emit mpv->ctrlSetOptionVariant("vo", "libmpv");
    emit mpv->ctrlSetOptionVariant("keep-open", true);
    emit mpv->ctrlSetOptionVariant("sub-visibility", "no");
    emit mpv->ctrlSetOptionVariant("hr-seek", "no");
    emit mpv->ctrlSetOptionVariant("audio", "no");
    emit mpv->ctrlSetOptionVariant("audio-display", "no");
    emit mpv->ctrlSetOptionVariant("ytdl-format",
        "bestvideo/best");
    setYtdlRawOptions();
    emit mpv->ctrlSetOptionVariant("clipboard-backends", "clr");
    mpv->setPaused(true);

    connect(mpv, &MpvObject::aspectChanged,
            this, &VideoPreview::updateWidth);

    shouldBeShown = false;
    hide();
    this->setVisible(false);
}

VideoPreview::~VideoPreview()
{
    if (!mpv)
        return;
    mpv->setWidgetType(Helpers::NullWidget);
    videoWidget = nullptr;
    delete mpv;
    mpv = nullptr;
}

void VideoPreview::openFile(const QUrl &fileUrl)
{
    if (fileUrl.isEmpty())
        return;
    mpv->urlOpen(fileUrl);
    aspectRatioSet = false;
    aspectRatio = 0;
}

void VideoPreview::updatePalette()
{
    QPalette palette = QApplication::palette();
    palette.setColor(QPalette::Window, palette.color(QPalette::ToolTipBase));
    palette.setColor(QPalette::WindowText, palette.color(QPalette::ToolTipText));
    textLabel->setPalette(palette);
}

void VideoPreview::show(const QString &text, double videoPosition, const QPoint &where,
                        int mainWindowWidth, int previewHeight)
{
    textLabel->setText(text);
    if (previewHeight != videoWidget->height()) {
        videoWidget->setFixedHeight(previewHeight);
        updateWidth(aspectRatio);
        setYtdlRawOptions();
    }
    mpv->seek(videoPosition, false, true, true);
    videoWidget->update();
    setPreviewPosition(where, mainWindowWidth);
    show();
}

void VideoPreview::setPreviewPosition(const QPoint &where, int mainWindowWidth)
{
    int tooltipWidth = videoContainer->width();
    int xPos = where.x() - std::round(tooltipWidth / 2);
    if (xPos + tooltipWidth + previewMarginX > mainWindowWidth)
        xPos = mainWindowWidth - tooltipWidth - previewMarginX;
    else if (xPos < previewMarginX)
        xPos = previewMarginX;
    previewBottomLeft = QPoint(xPos, where.y());
}

void VideoPreview::updateWidth(double newAspect)
{
    if (newAspect == 0) {
        aspectRatioSet = false;
        return;
    }
    aspectRatio = newAspect;
    double dpr = devicePixelRatioF();
    int newWidth = floor(round(videoWidget->height() * dpr) * newAspect) / dpr;
    videoWidget->setFixedWidth(newWidth);
    textLabel->setFixedSize(newWidth, labelHeight);
    videoWidget->move(0, shadowMargin);
    textLabel->move(0, shadowMargin + videoWidget->height());
    videoContainer->setFixedSize(newWidth + shadowMargin * 2,
                                 videoWidget->height() + labelHeight + shadowMargin * 2);
    aspectRatioSet = true;
    if (shouldBeShown)
        show();
    else
        hide();
}

void VideoPreview::setYtdlRawOptions()
{
    emit mpv->ctrlSetOptionVariant("ytdl-raw-options", QString("js-runtimes=quickjs,"\
                                                    "remote-components=ejs:github,"\
                                                    "format-sort=[res:%1,+size,+br,+fps]").arg(videoWidget->height()));
}

void VideoPreview::show()
{
    if (!aspectRatioSet) {
        shouldBeShown = true;
        return;
    }
    videoContainer->move(previewBottomLeft.x(),
                         previewBottomLeft.y() - videoContainer->height());
}

void VideoPreview::hide() {
    shouldBeShown = false;
    videoContainer->move(-50000, -50000);
}
