#include <QApplication>
#include <QToolTip>
#include "videopreview.h"

VideoPreview::VideoPreview(QWidget *parent) : QWidget(parent)
{
    mpv = new MpvObject(this);
    videoWidget = new MpvGlWidget(mpv, this);
    mpv->setWidgetType(Helpers::CustomWidget, videoWidget);
    videoWidget->setFixedSize(1, 1);
    textLabel = new QLabel(this);
    textLabel->setAlignment(Qt::AlignCenter);
    layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(videoWidget);
    layout->addWidget(textLabel);
    setLayout(layout);
    if (QApplication::platformName() == "wayland")
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    else
        setWindowFlags(Qt::ToolTip);

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
    emit mpv->ctrlSetOptionVariant("audio-display", "no");
    emit mpv->ctrlSetOptionVariant("ytdl-format", "worst");

    connect(mpv, &MpvObject::aspectChanged, this, [this](double newAspect) {
        if (newAspect == 0) {
            aspectRatioSet = false;
            return;
        }
        int baseHeight = 180;
        int newWidth = int(baseHeight * newAspect);
        videoWidget->setFixedSize(newWidth, baseHeight);
        aspectRatioSet = true;
        if (shouldBeShown)
            show();
    });

    show();
    shouldBeShown = false;
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

void VideoPreview::setTimeText(const QString &text, double videoPosition)
{
    textLabel->setText(text);
    mpv->setTime(videoPosition);
    mpv->setPaused(true);
    videoWidget->update();
}

void VideoPreview::showEvent(QShowEvent *event)
{
    if (!aspectRatioSet) {
        shouldBeShown = true;
        return;
    }
    QWidget::showEvent(event);
}

void VideoPreview::hideEvent(QHideEvent *event)
{
    shouldBeShown = false;
    QWidget::hideEvent(event);
}
