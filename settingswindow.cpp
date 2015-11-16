#include "settingswindow.h"
#include "ui_settingswindow.h"
#include <QDebug>
#include <QDialogButtonBox>

const char *settings::ditherTypeToText[] = {"fruit", "ordered", "no"};

const char *settings::scaleScalarToText[]  = {
    "bilinear", "bicubic_fast", "oversample", "spline16", "spline36",
    "spline64", "sinc", "lanczos", "gingseng", "jinc", "ewa_lanczos",
    "ewa_hanning", "ewa_gingseng", "ewa_lanczossharp", "ewa_lanczossoft",
    "hassnsoft",  "bicubic", "bcspline", "catmull_rom", "mitchell",
    "robidoux", "robidouxsharp", "ewa_robidoux", "ewa_robidouxsharp",
    "box", "nearest", "triangle", "gaussian"
};

const char *settings::timeScalarToText[] = {
    "oversample", "spline16", "spline64", "sinc", "lanczos",
    "gingseng", "catmull_rom", "mitchell", "robidoux", "robidouxsharp",
    "box", "nearest", "triangle", "gaussian"
};

Q_DECLARE_METATYPE(settings::OpenMode);
Q_DECLARE_METATYPE(settings::TitleBarMode);
Q_DECLARE_METATYPE(settings::LogoDisplay);
Q_DECLARE_METATYPE(settings::PlaybackProgression);
Q_DECLARE_METATYPE(settings::AutoZoomMethod);
Q_DECLARE_METATYPE(settings::VideoBackend);
Q_DECLARE_METATYPE(settings::FBDepth);
Q_DECLARE_METATYPE(settings::AlphaMode);
Q_DECLARE_METATYPE(settings::DitherType);
Q_DECLARE_METATYPE(settings::ScaleScalar);
Q_DECLARE_METATYPE(settings::ScaleWindow);
Q_DECLARE_METATYPE(settings::TimeScalar);
Q_DECLARE_METATYPE(settings::Prescalar);
Q_DECLARE_METATYPE(settings::Nnedi3Neurons);
Q_DECLARE_METATYPE(settings::Nnedi3Window);
Q_DECLARE_METATYPE(settings::Nnedi3UploadMethod);
Q_DECLARE_METATYPE(settings::TargetPrim);
Q_DECLARE_METATYPE(settings::TargetTrc);
Q_DECLARE_METATYPE(settings::AudioRenderer);
Q_DECLARE_METATYPE(settings::ShaderPresetList);
Q_DECLARE_METATYPE(settings::FullscreenShow);
Q_DECLARE_METATYPE(settings::XrandrModeList);
Q_DECLARE_METATYPE(settings::FramedropMode);
Q_DECLARE_METATYPE(settings::DecoderDropMode);
Q_DECLARE_METATYPE(settings::SyncMode);
Q_DECLARE_METATYPE(settings::SubtitlePlacementX);
Q_DECLARE_METATYPE(settings::SubtitlePlacementY);
Q_DECLARE_METATYPE(settings::AssOverride);
Q_DECLARE_METATYPE(settings::SubsAlignment);
Q_DECLARE_METATYPE(settings::TimeTooltipLocation);
#define STORE_PROP(X) m[__STRING(X)] = qVariantFromValue(X)
#define READ_PROP(X) X = m[__STRING(X)].value<__typeof__(X)>();

QVariantMap settings::toVMap()
{
    QVariantMap m;
    STORE_PROP(playerOpenMode);
    STORE_PROP(useTrayIcon);
    STORE_PROP(showOSD);
    STORE_PROP(disableDiscMenu);
    STORE_PROP(titleBar);
    STORE_PROP(replaceFileNameInTitleBar);
    STORE_PROP(rememberLastPlaylist);
    STORE_PROP(rememberWindowPosition);
    STORE_PROP(rememberWindowSize);
    STORE_PROP(rememberPanScanZoom);
    STORE_PROP(logoDisplayed);
    STORE_PROP(logoLocation);
    STORE_PROP(playbackProgress);
    STORE_PROP(playbackTimes);
    STORE_PROP(rewindPlaylist);
    STORE_PROP(volumeStep);
    STORE_PROP(speedStep);
    STORE_PROP(subtitleTracks);
    STORE_PROP(audioTracks);
    STORE_PROP(zoomMethod);
    STORE_PROP(autoFitFactor);
    STORE_PROP(autoZoom);
    STORE_PROP(autoloadAudio);
    STORE_PROP(autoloadSubtitles);
    STORE_PROP(audioBalance);
    STORE_PROP(audioVolume);
    STORE_PROP(videoIsDumb);
    STORE_PROP(videoBackend);
    STORE_PROP(framebufferDepth);
    STORE_PROP(framebufferAlpha);
    STORE_PROP(alphaMode);
    STORE_PROP(sharpen);
    STORE_PROP(dither);
    STORE_PROP(ditherDepth);
    STORE_PROP(ditherType);
    STORE_PROP(ditherFruitSize);
    STORE_PROP(temporalDither);
    STORE_PROP(temporalPeriod);
    STORE_PROP(downscaleCorrectly);
    STORE_PROP(scaleInLinearLight);
    STORE_PROP(temporalInterpolation);
    STORE_PROP(sigmoidizedUpscaling);
    STORE_PROP(sigmoidCenter);
    STORE_PROP(sigmoidSlope);
    STORE_PROP(scaleScalar);
    STORE_PROP(scaleParam1);
    STORE_PROP(scaleParam2);
    STORE_PROP(scaleRadius);
    STORE_PROP(scaleAntiRing);
    STORE_PROP(scaleBlur);
    STORE_PROP(scaleWindowParam);
    STORE_PROP(scaleWindow);
    STORE_PROP(scaleParam1Set);
    STORE_PROP(scaleParam2Set);
    STORE_PROP(scaleRadiusSet);
    STORE_PROP(scaleAntiRingSet);
    STORE_PROP(scaleBlurSet);
    STORE_PROP(scaleWindowParamSet);
    STORE_PROP(scaleWindowSet);
    STORE_PROP(scaleClamp);
    STORE_PROP(dscaleScalar);
    STORE_PROP(dscaleParam1);
    STORE_PROP(dscaleParam2);
    STORE_PROP(dscaleRadius);
    STORE_PROP(dscaleAntiRing);
    STORE_PROP(dscaleBlur);
    STORE_PROP(dscaleWindowParam);
    STORE_PROP(dscaleWindow);
    STORE_PROP(dscaleParam1Set);
    STORE_PROP(dscaleParam2Set);
    STORE_PROP(dscaleRadiusSet);
    STORE_PROP(dscaleAntiRingSet);
    STORE_PROP(dscaleBlurSet);
    STORE_PROP(dscaleWindowParamSet);
    STORE_PROP(dscaleWindowSet);
    STORE_PROP(dscaleClamp);
    STORE_PROP(cscaleScalar);
    STORE_PROP(cscaleParam1);
    STORE_PROP(cscaleParam2);
    STORE_PROP(cscaleRadius);
    STORE_PROP(cscaleAntiRing);
    STORE_PROP(cscaleBlur);
    STORE_PROP(cscaleWindowParam);
    STORE_PROP(cscaleWindow);
    STORE_PROP(cscaleParam1Set);
    STORE_PROP(cscaleParam2Set);
    STORE_PROP(cscaleRadiusSet);
    STORE_PROP(cscaleAntiRingSet);
    STORE_PROP(cscaleBlurSet);
    STORE_PROP(cscaleWindowParamSet);
    STORE_PROP(cscaleWindowSet);
    STORE_PROP(cscaleClamp);
    STORE_PROP(tscaleScalar);
    STORE_PROP(tscaleParam1);
    STORE_PROP(tscaleParam2);
    STORE_PROP(tscaleRadius);
    STORE_PROP(tscaleAntiRing);
    STORE_PROP(tscaleBlur);
    STORE_PROP(tscaleWindowParam);
    STORE_PROP(tscaleWindow);
    STORE_PROP(tscaleParam1Set);
    STORE_PROP(tscaleParam2Set);
    STORE_PROP(tscaleRadiusSet);
    STORE_PROP(tscaleAntiRingSet);
    STORE_PROP(tscaleBlurSet);
    STORE_PROP(tscaleWindowParamSet);
    STORE_PROP(tscaleWindowSet);
    STORE_PROP(tscaleClamp);
    STORE_PROP(prescalar);
    STORE_PROP(prescalarPasses);
    STORE_PROP(superxbrSharpness);
    STORE_PROP(superxbrEdgeStrength);
    STORE_PROP(nnedi3Neurons);
    STORE_PROP(nnedi3Window);
    STORE_PROP(debanding);
    STORE_PROP(debandIterations);
    STORE_PROP(debandThreshold);
    STORE_PROP(debandRange);
    STORE_PROP(debandGrain);
    STORE_PROP(gamma);
    STORE_PROP(targetPrim);
    STORE_PROP(targetTrc);
    STORE_PROP(iccAutodetect);
    STORE_PROP(iccLocation);
    STORE_PROP(audioRenderer);
    STORE_PROP(pulseHost);
    STORE_PROP(pulseSink);
    STORE_PROP(pulseBuffer);
    STORE_PROP(pulseLatencyHack);
    STORE_PROP(alsaMixer);
    STORE_PROP(alsaMixerDeice);
    STORE_PROP(alsaMixerName);
    STORE_PROP(alsaMixerIndex);
    STORE_PROP(alsaResample);
    STORE_PROP(alsaIgnoreChmap);
    STORE_PROP(ossDspDevice);
    STORE_PROP(ossMixerDevice);
    STORE_PROP(ossMixerChannel);
    STORE_PROP(nullIsUntimed);
    STORE_PROP(nullOutburst);
    STORE_PROP(nullBufferLength);
    STORE_PROP(shaderFiles);
    STORE_PROP(preShaders);
    STORE_PROP(postShaders);
    STORE_PROP(shaderPresets);
    STORE_PROP(fullscreenMonitor);
    STORE_PROP(fullscreenLaunch);
    STORE_PROP(exitFullscreenAtEnd);
    STORE_PROP(hidePanelsInFullscreen);
    STORE_PROP(hideControlsInFullscreen);
    STORE_PROP(controlShowDuration);
    STORE_PROP(xrandrModes);
    STORE_PROP(xrandrChangeDelay);
    STORE_PROP(switchXrandrBackToOldMode);
    STORE_PROP(restoreXrandrOnExit);
    STORE_PROP(framedroppingMode);
    STORE_PROP(decoderDroppingMode);
    STORE_PROP(syncMode);
    STORE_PROP(audioDropSize);
    STORE_PROP(maxAudioChange);
    STORE_PROP(maxVideoChange);
    STORE_PROP(subPlacementX);
    STORE_PROP(subPlacementY);
    STORE_PROP(overrideSubPlacement);
    STORE_PROP(subtitlePosition);
    STORE_PROP(subtitlesUseMargins);
    STORE_PROP(subtitlesInGrayscale);
    STORE_PROP(subtitlesWithFixedTiming);
    STORE_PROP(subtitlesClearOnSeek);
    STORE_PROP(assOverrideMode);
    STORE_PROP(subsFont);
    STORE_PROP(fontBold);
    STORE_PROP(borderSize);
    STORE_PROP(borderShadowOffset);
    STORE_PROP(subsAlignment);
    STORE_PROP(subsMarginX);
    STORE_PROP(subsMarginY);
    STORE_PROP(subsAreRelativeToVideoFrame);
    STORE_PROP(subsColor);
    STORE_PROP(borderColor);
    STORE_PROP(shadowColor);
    STORE_PROP(preferDefaultSubs);
    STORE_PROP(preferExternalSubs);
    STORE_PROP(ignoreEmbeddedSubs);
    STORE_PROP(subtitleDatabase);
    STORE_PROP(seekFast);
    STORE_PROP(showChapterMarks);
    STORE_PROP(openNextFile);
    STORE_PROP(showTimeTooltip);
    STORE_PROP(timeTooltipLocation);
    STORE_PROP(osdFont);
    STORE_PROP(osdSize);
    STORE_PROP(brightness);
    STORE_PROP(contrast);
    STORE_PROP(hue);
    STORE_PROP(saturation);
    return m;
}

void settings::fromVMap(const QVariantMap &m)
{
    READ_PROP(playerOpenMode);
    READ_PROP(useTrayIcon);
    READ_PROP(showOSD);
    READ_PROP(disableDiscMenu);
    READ_PROP(titleBar);
    READ_PROP(replaceFileNameInTitleBar);
    READ_PROP(rememberLastPlaylist);
    READ_PROP(rememberWindowPosition);
    READ_PROP(rememberWindowSize);
    READ_PROP(rememberPanScanZoom);
    READ_PROP(logoDisplayed);
    READ_PROP(logoLocation);
    READ_PROP(playbackProgress);
    READ_PROP(playbackTimes);
    READ_PROP(rewindPlaylist);
    READ_PROP(volumeStep);
    READ_PROP(speedStep);
    READ_PROP(subtitleTracks);
    READ_PROP(audioTracks);
    READ_PROP(zoomMethod);
    READ_PROP(autoFitFactor);
    READ_PROP(autoZoom);
    READ_PROP(autoloadAudio);
    READ_PROP(autoloadSubtitles);
    READ_PROP(audioBalance);
    READ_PROP(audioVolume);
    READ_PROP(videoIsDumb);
    READ_PROP(videoBackend);
    READ_PROP(framebufferDepth);
    READ_PROP(framebufferAlpha);
    READ_PROP(alphaMode);
    READ_PROP(sharpen);
    READ_PROP(dither);
    READ_PROP(ditherDepth);
    READ_PROP(ditherType);
    READ_PROP(ditherFruitSize);
    READ_PROP(temporalDither);
    READ_PROP(temporalPeriod);
    READ_PROP(downscaleCorrectly);
    READ_PROP(scaleInLinearLight);
    READ_PROP(temporalInterpolation);
    READ_PROP(sigmoidizedUpscaling);
    READ_PROP(sigmoidCenter);
    READ_PROP(sigmoidSlope);
    READ_PROP(scaleScalar);
    READ_PROP(scaleParam1);
    READ_PROP(scaleParam2);
    READ_PROP(scaleRadius);
    READ_PROP(scaleAntiRing);
    READ_PROP(scaleBlur);
    READ_PROP(scaleWindowParam);
    READ_PROP(scaleWindow);
    READ_PROP(scaleParam1Set);
    READ_PROP(scaleParam2Set);
    READ_PROP(scaleRadiusSet);
    READ_PROP(scaleAntiRingSet);
    READ_PROP(scaleBlurSet);
    READ_PROP(scaleWindowParamSet);
    READ_PROP(scaleWindowSet);
    READ_PROP(scaleClamp);
    READ_PROP(dscaleScalar);
    READ_PROP(dscaleParam1);
    READ_PROP(dscaleParam2);
    READ_PROP(dscaleRadius);
    READ_PROP(dscaleAntiRing);
    READ_PROP(dscaleBlur);
    READ_PROP(dscaleWindowParam);
    READ_PROP(dscaleWindow);
    READ_PROP(dscaleParam1Set);
    READ_PROP(dscaleParam2Set);
    READ_PROP(dscaleRadiusSet);
    READ_PROP(dscaleAntiRingSet);
    READ_PROP(dscaleBlurSet);
    READ_PROP(dscaleWindowParamSet);
    READ_PROP(dscaleWindowSet);
    READ_PROP(dscaleClamp);
    READ_PROP(cscaleScalar);
    READ_PROP(cscaleParam1);
    READ_PROP(cscaleParam2);
    READ_PROP(cscaleRadius);
    READ_PROP(cscaleAntiRing);
    READ_PROP(cscaleBlur);
    READ_PROP(cscaleWindowParam);
    READ_PROP(cscaleWindow);
    READ_PROP(cscaleParam1Set);
    READ_PROP(cscaleParam2Set);
    READ_PROP(cscaleRadiusSet);
    READ_PROP(cscaleAntiRingSet);
    READ_PROP(cscaleBlurSet);
    READ_PROP(cscaleWindowParamSet);
    READ_PROP(cscaleWindowSet);
    READ_PROP(cscaleClamp);
    READ_PROP(tscaleScalar);
    READ_PROP(tscaleParam1);
    READ_PROP(tscaleParam2);
    READ_PROP(tscaleRadius);
    READ_PROP(tscaleAntiRing);
    READ_PROP(tscaleBlur);
    READ_PROP(tscaleWindowParam);
    READ_PROP(tscaleWindow);
    READ_PROP(tscaleParam1Set);
    READ_PROP(tscaleParam2Set);
    READ_PROP(tscaleRadiusSet);
    READ_PROP(tscaleAntiRingSet);
    READ_PROP(tscaleBlurSet);
    READ_PROP(tscaleWindowParamSet);
    READ_PROP(tscaleWindowSet);
    READ_PROP(tscaleClamp);
    READ_PROP(prescalar);
    READ_PROP(prescalarPasses);
    READ_PROP(superxbrSharpness);
    READ_PROP(superxbrEdgeStrength);
    READ_PROP(nnedi3Neurons);
    READ_PROP(nnedi3Window);
    READ_PROP(debanding);
    READ_PROP(debandIterations);
    READ_PROP(debandThreshold);
    READ_PROP(debandRange);
    READ_PROP(debandGrain);
    READ_PROP(gamma);
    READ_PROP(targetPrim);
    READ_PROP(targetTrc);
    READ_PROP(iccAutodetect);
    READ_PROP(iccLocation);
    READ_PROP(audioRenderer);
    READ_PROP(pulseHost);
    READ_PROP(pulseSink);
    READ_PROP(pulseBuffer);
    READ_PROP(pulseLatencyHack);
    READ_PROP(alsaMixer);
    READ_PROP(alsaMixerDeice);
    READ_PROP(alsaMixerName);
    READ_PROP(alsaMixerIndex);
    READ_PROP(alsaResample);
    READ_PROP(alsaIgnoreChmap);
    READ_PROP(ossDspDevice);
    READ_PROP(ossMixerDevice);
    READ_PROP(ossMixerChannel);
    READ_PROP(nullIsUntimed);
    READ_PROP(nullOutburst);
    READ_PROP(nullBufferLength);
    READ_PROP(shaderFiles);
    READ_PROP(preShaders);
    READ_PROP(postShaders);
    READ_PROP(shaderPresets);
    READ_PROP(fullscreenMonitor);
    READ_PROP(fullscreenLaunch);
    READ_PROP(exitFullscreenAtEnd);
    READ_PROP(hidePanelsInFullscreen);
    READ_PROP(hideControlsInFullscreen);
    READ_PROP(controlShowDuration);
    READ_PROP(xrandrModes);
    READ_PROP(xrandrChangeDelay);
    READ_PROP(switchXrandrBackToOldMode);
    READ_PROP(restoreXrandrOnExit);
    READ_PROP(framedroppingMode);
    READ_PROP(decoderDroppingMode);
    READ_PROP(syncMode);
    READ_PROP(audioDropSize);
    READ_PROP(maxAudioChange);
    READ_PROP(maxVideoChange);
    READ_PROP(subPlacementX);
    READ_PROP(subPlacementY);
    READ_PROP(overrideSubPlacement);
    READ_PROP(subtitlePosition);
    READ_PROP(subtitlesUseMargins);
    READ_PROP(subtitlesInGrayscale);
    READ_PROP(subtitlesWithFixedTiming);
    READ_PROP(subtitlesClearOnSeek);
    READ_PROP(assOverrideMode);
    READ_PROP(subsFont);
    READ_PROP(fontBold);
    READ_PROP(borderSize);
    READ_PROP(borderShadowOffset);
    READ_PROP(subsAlignment);
    READ_PROP(subsMarginX);
    READ_PROP(subsMarginY);
    READ_PROP(subsAreRelativeToVideoFrame);
    READ_PROP(subsColor);
    READ_PROP(borderColor);
    READ_PROP(shadowColor);
    READ_PROP(preferDefaultSubs);
    READ_PROP(preferExternalSubs);
    READ_PROP(ignoreEmbeddedSubs);
    READ_PROP(subtitleDatabase);
    READ_PROP(seekFast);
    READ_PROP(showChapterMarks);
    READ_PROP(openNextFile);
    READ_PROP(showTimeTooltip);
    READ_PROP(timeTooltipLocation);
    READ_PROP(osdFont);
    READ_PROP(osdSize);
    READ_PROP(brightness);
    READ_PROP(contrast);
    READ_PROP(hue);
    READ_PROP(saturation);
}


SettingsWindow::SettingsWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SettingsWindow)
{
    ui->setupUi(this);
    ui->pageStack->setCurrentIndex(0);
    ui->videoTabs->setCurrentIndex(0);
    ui->scalingTabs->setCurrentIndex(0);
    ui->prescalarStack->setCurrentIndex(0);
    ui->audioRendererStack->setCurrentIndex(0);
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}

settings SettingsWindow::buildSettings() {
    settings s;
    s.videoIsDumb = ui->videoDumbMode->isChecked();
    s.temporalInterpolation = ui->scalingTemporalInterpolation->isChecked();
    s.dither = ui->ditherDithering->isChecked();
    s.ditherDepth = ui->ditherDepth->value();
    s.ditherType = (settings::DitherType)ui->ditherType->currentIndex();
    s.ditherFruitSize = ui->ditherFruitSize->value();
    s.debanding = ui->debandEnabled->isChecked();
    s.scaleScalar = (settings::ScaleScalar)ui->scaleScalar->currentIndex();
    s.dscaleScalar = (settings::ScaleScalar)(ui->dscaleScalar->currentIndex() - 1);
    s.cscaleScalar = (settings::ScaleScalar)ui->cscaleScalar->currentIndex();
    s.tscaleScalar = (settings::TimeScalar)ui->tscaleScalar->currentIndex();
    return s;
}

void SettingsWindow::takeSettings(const settings &s)
{
    ui->videoDumbMode->setChecked(s.videoIsDumb);
    ui->scalingTemporalInterpolation->setChecked(s.temporalInterpolation);
    ui->ditherDepth->setValue(s.ditherDepth);
    ui->ditherType->setCurrentIndex(s.ditherType);
    ui->ditherFruitSize->setValue(s.ditherFruitSize);
    ui->debandEnabled->setChecked(s.debanding);
    ui->scaleScalar->setCurrentIndex(s.scaleScalar);
    ui->dscaleScalar->setCurrentIndex(s.dscaleScalar);
    ui->cscaleScalar->setCurrentIndex(s.cscaleScalar);
    ui->tscaleScalar->setCurrentIndex(s.tscaleScalar);
}

void SettingsWindow::on_pageTree_itemSelectionChanged()
{
    QModelIndex modelIndex = ui->pageTree->currentIndex();
    if (!modelIndex.isValid())
        return;

    static int parentIndex[] = { 0, 4, 9, 12, 13 };
    int index = 0;
    if (!modelIndex.parent().isValid())
        index = parentIndex[modelIndex.row()];
    else
        index = parentIndex[modelIndex.parent().row()] + modelIndex.row() + 1;
    ui->pageStack->setCurrentIndex(index);
    ui->pageLabel->setText(QString("<big><b>%1</b></big>").
                           arg(modelIndex.data().toString()));
}

void SettingsWindow::on_buttonBox_clicked(QAbstractButton *button)
{
    QDialogButtonBox::ButtonRole buttonRole;
    buttonRole = ui->buttonBox->buttonRole(button);
    if (buttonRole == QDialogButtonBox::ApplyRole ||
            buttonRole == QDialogButtonBox::AcceptRole) {
        emit settingsData(buildSettings());
    }
    if (buttonRole == QDialogButtonBox::AcceptRole ||
            buttonRole == QDialogButtonBox::RejectRole)
        close();
}
