#include <QToolTip>
#include "videopreview.h"

constexpr int previewMarginX = 20;
constexpr int labelHeight = 20;
constexpr int videoHeight = 180;

VideoPreview::VideoPreview(QWidget *parent) : QWidget(parent)
{
    mpv = new MpvObject(this);
    videoWidget = new MpvGlWidget(mpv, parent);
    mpv->setWidgetType(Helpers::CustomWidget, videoWidget);
    textLabel = new QLabel(parent);
    textLabel->setAlignment(Qt::AlignCenter);

    textLabel->setAutoFillBackground(true);
    QPalette tooltipPalette = QToolTip::palette();
    QPalette customPalette = this->palette();
    customPalette.setColor(QPalette::Window, tooltipPalette.color(QPalette::ToolTipBase));
    customPalette.setColor(QPalette::WindowText, tooltipPalette.color(QPalette::ToolTipText));
    textLabel->setPalette(customPalette);

    emit mpv->ctrlSetOptionVariant("vo", "libmpv");
    emit mpv->ctrlSetOptionVariant("keep-open", true);
    emit mpv->ctrlSetOptionVariant("sub-visibility", "no");
    emit mpv->ctrlSetOptionVariant("hr-seek", "no");
    emit mpv->ctrlSetOptionVariant("audio", "no");
    emit mpv->ctrlSetOptionVariant("audio-display", "no");
    emit mpv->ctrlSetOptionVariant("ytdl-raw-options", "format-sort=[+size,+br,+res,+fps]");
    emit mpv->ctrlSetOptionVariant("clipboard-backends", "clr");

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
    mpv->urlOpen(fileUrl);
    mpv->setPaused(true);
    aspectRatioSet = false;
}

void VideoPreview::show(const QString &text, double videoPosition, const QPoint &where, int mainWindowWidth)
{
    textLabel->setText(text);
    mpv->setTime(videoPosition);
    mpv->setPaused(true);
    videoWidget->update();
    setPreviewPosition(where, mainWindowWidth);
    show();
}

void VideoPreview::setPreviewPosition(const QPoint &where, int mainWindowWidth)
{
    int tooltipWidth = videoWidget->width();
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
    int newWidth = int(videoHeight * newAspect);
    videoWidget->setFixedSize(newWidth, videoHeight);
    textLabel->setFixedSize(newWidth, labelHeight);
    aspectRatioSet = true;
    if (shouldBeShown)
        show();
    else
        hide();
}

void VideoPreview::show()
{
    if (!aspectRatioSet) {
        shouldBeShown = true;
        return;
    }
    videoWidget->move(previewBottomLeft.x(),
                      previewBottomLeft.y() - videoWidget->height() - textLabel->height());
    textLabel->move(previewBottomLeft.x(),
                    previewBottomLeft.y() - textLabel->height());
}

void VideoPreview::hide() {
    shouldBeShown = false;
    videoWidget->move(-50000, -50000);
    textLabel->move(-50000, -50000);
}
