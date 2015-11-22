#include "settingswindow.h"
#include "ui_settingswindow.h"
#include <QDebug>
#include <QDialogButtonBox>

const char *Settings::videoBackendToText[]  = {
    "auto", "x11", "wayland","x11egl"
};
const char *Settings::fbDepthToText[][2] = {
    { "rgb8", "rgba" }, { "rgb10", "rgb10_a2" }, { "rgba12", "rgba12" },
    { "rgb16", "rgba16" }, { "rgb16f", "rgba16f" }, { "rgb32f", "rgba32f" }
};
const char *Settings::alphaModeToText[] = {
    "blend", "yes", "no"
};
const char *Settings::ditherTypeToText[] = {
    "fruit", "ordered", "no"
};
const char *Settings::scaleScalarToText[]  = {
    "bilinear", "bicubic_fast", "oversample", "spline16", "spline36",
    "spline64", "sinc", "lanczos", "gingseng", "jinc", "ewa_lanczos",
    "ewa_hanning", "ewa_gingseng", "ewa_lanczossharp", "ewa_lanczossoft",
    "hassnsoft",  "bicubic", "bcspline", "catmull_rom", "mitchell",
    "robidoux", "robidouxsharp", "ewa_robidoux", "ewa_robidouxsharp",
    "box", "nearest", "triangle", "gaussian"
};
const char *Settings::scaleWindowToText[] = {
    "box", "triable", "bartlett", "hanning", "hamming", "quadric", "welch",
    "kaiser", "blackman", "gaussian", "sinc", "jinc", "sphinx"
};
const char *Settings::timeScalarToText[] = {
    "oversample", "spline16", "spline36", "spline64", "sinc", "lanczos",
    "gingseng", "catmull_rom", "mitchell", "robidoux", "robidouxsharp",
    "box", "nearest", "triangle", "gaussian"
};
const char *Settings::prescalarToText[] = {
    "none", "superxbr", "needi3"
};
const char *Settings::nnedi3NeuronsToText[] = {
    "16", "32", "64", "128"
};
const char *Settings::nnedi3WindowToText[] = {
    "8x4", "8x6"
};
const char *Settings::nnedi3UploadMethodToText[] = {
    "ubo", "shader"
};
const char *Settings::targetPrimToText[] = {
    "auto", "bt.601-525", "bt.601-625", "bt.709", "bt.2020", "bt.470m",
    "apple", "adobe", "prophoto", "cie1931"
};
const char *Settings::targetTrcToText[] = {
    "auto", "by.1886", "srgb", "linear", "gamma1.8", "gamma2.2", "gamma2.8",
    "prophoto"
};
const char *Settings::audioRendererToText[] = {
    "pulse", "alsa", "oss", "null"
};
const char *Settings::framedropToText[] = {
    "no", "vo", "decoder", "decoder+vo"
};
const char *Settings::decoderDropToText[] = {
    "none", "default", "nonref", "bidir", "nonkey", "all"
};
const char *Settings::syncModeToText[] = {
    "audio", "display-resample", "display-resample-vdrop",
    "display-resample-desync", "display-adrop", "display-vdrop"
};
const char *Settings::subtitlePlacementXToText[] = {
    "left", "center", "right"
};
const char *Settings::subtitlePlacementYToText[] = {
    "top", "center", "bottom"
};
const char *Settings::assOverrideToText[] = {
    "no", "yes", "force", "signfs"
};
const char *Settings::subtitleAlignmentToText[][2] = {
    { "top", "center" }, { "top", "right" }, { "center", "right" },
    { "bottom", "right" }, { "bottom", "center" }, { "bottom", "left" },
    { "center", "left" }, { "top", "left" }, { "center", "center" }
};

Q_DECLARE_METATYPE(Settings::ShaderPresetList);
Q_DECLARE_METATYPE(Settings::XrandrModeList);
#define STORE_PROP(X) m[__STRING(X)] = qVariantFromValue(X)
#define STORE_PROP_T(X,Y) m[__STRING(X)] = qVariantFromValue((Y)X);
#define READ_PROP(X,Y) \
    if (m.contains(__STRING(X))) \
        X = m[__STRING(X)].value<__typeof__(X)>(); \
    else \
        X = Y;
#define READ_PROP_T(X,Y,T) \
    if (m.contains(__STRING(X))) \
        X = static_cast<__typeof__(X)>(m[__STRING(X)].value<T>()); \
    else \
        X = Y;

QVariantMap Settings::toVMap()
{
    QVariantMap m;
    STORE_PROP_T(playerOpenMode, int);
    STORE_PROP(useTrayIcon);
    STORE_PROP(showOSD);
    STORE_PROP(limitWindowProportions);
    STORE_PROP(disableDiscMenu);
    STORE_PROP_T(titleBar, int);
    STORE_PROP(replaceFileNameInTitleBar);
    STORE_PROP(rememberLastPlaylist);
    STORE_PROP(rememberWindowPosition);
    STORE_PROP(rememberWindowSize);
    STORE_PROP(rememberPanScanZoom);
    STORE_PROP_T(logoDisplayed, int);
    STORE_PROP(logoLocation);
    STORE_PROP_T(playbackProgress, int);
    STORE_PROP(playbackTimes);
    STORE_PROP(rewindPlaylist);
    STORE_PROP(volumeStep);
    STORE_PROP(speedStep);
    STORE_PROP(subtitleTracks);
    STORE_PROP(audioTracks);
    STORE_PROP_T(zoomMethod, int);
    STORE_PROP(autoFitFactor);
    STORE_PROP(autoZoom);
    STORE_PROP(autoloadAudio);
    STORE_PROP(autoloadSubtitles);
    STORE_PROP(audioBalance);
    STORE_PROP(audioVolume);
    STORE_PROP(videoIsDumb);
    STORE_PROP_T(videoBackend, int);
    STORE_PROP_T(framebufferDepth, int);
    STORE_PROP(framebufferAlpha);
    STORE_PROP_T(alphaMode, int);
    STORE_PROP(sharpen);
    STORE_PROP(dither);
    STORE_PROP(ditherDepth);
    STORE_PROP_T(ditherType, int);
    STORE_PROP(ditherFruitSize);
    STORE_PROP(temporalDither);
    STORE_PROP(temporalPeriod);
    STORE_PROP(downscaleCorrectly);
    STORE_PROP(scaleInLinearLight);
    STORE_PROP(temporalInterpolation);
    STORE_PROP(blendSubtitles);
    STORE_PROP(sigmoidizedUpscaling);
    STORE_PROP(sigmoidCenter);
    STORE_PROP(sigmoidSlope);
    STORE_PROP_T(scaleScalar, int);
    STORE_PROP(scaleParam1);
    STORE_PROP(scaleParam2);
    STORE_PROP(scaleRadius);
    STORE_PROP(scaleAntiRing);
    STORE_PROP(scaleBlur);
    STORE_PROP(scaleWindowParam);
    STORE_PROP_T(scaleWindow, int);
    STORE_PROP(scaleParam1Set);
    STORE_PROP(scaleParam2Set);
    STORE_PROP(scaleRadiusSet);
    STORE_PROP(scaleAntiRingSet);
    STORE_PROP(scaleBlurSet);
    STORE_PROP(scaleWindowParamSet);
    STORE_PROP(scaleWindowSet);
    STORE_PROP(scaleClamp);
    STORE_PROP_T(dscaleScalar, int);
    STORE_PROP(dscaleParam1);
    STORE_PROP(dscaleParam2);
    STORE_PROP(dscaleRadius);
    STORE_PROP(dscaleAntiRing);
    STORE_PROP(dscaleBlur);
    STORE_PROP(dscaleWindowParam);
    STORE_PROP_T(dscaleWindow, int);
    STORE_PROP(dscaleParam1Set);
    STORE_PROP(dscaleParam2Set);
    STORE_PROP(dscaleRadiusSet);
    STORE_PROP(dscaleAntiRingSet);
    STORE_PROP(dscaleBlurSet);
    STORE_PROP(dscaleWindowParamSet);
    STORE_PROP(dscaleWindowSet);
    STORE_PROP(dscaleClamp);
    STORE_PROP_T(cscaleScalar, int);
    STORE_PROP(cscaleParam1);
    STORE_PROP(cscaleParam2);
    STORE_PROP(cscaleRadius);
    STORE_PROP(cscaleAntiRing);
    STORE_PROP(cscaleBlur);
    STORE_PROP(cscaleWindowParam);
    STORE_PROP_T(cscaleWindow, int);
    STORE_PROP(cscaleParam1Set);
    STORE_PROP(cscaleParam2Set);
    STORE_PROP(cscaleRadiusSet);
    STORE_PROP(cscaleAntiRingSet);
    STORE_PROP(cscaleBlurSet);
    STORE_PROP(cscaleWindowParamSet);
    STORE_PROP(cscaleWindowSet);
    STORE_PROP(cscaleClamp);
    STORE_PROP_T(tscaleScalar, int);
    STORE_PROP(tscaleParam1);
    STORE_PROP(tscaleParam2);
    STORE_PROP(tscaleRadius);
    STORE_PROP(tscaleAntiRing);
    STORE_PROP(tscaleBlur);
    STORE_PROP(tscaleWindowParam);
    STORE_PROP_T(tscaleWindow, int);
    STORE_PROP(tscaleParam1Set);
    STORE_PROP(tscaleParam2Set);
    STORE_PROP(tscaleRadiusSet);
    STORE_PROP(tscaleAntiRingSet);
    STORE_PROP(tscaleBlurSet);
    STORE_PROP(tscaleWindowParamSet);
    STORE_PROP(tscaleWindowSet);
    STORE_PROP(tscaleClamp);
    STORE_PROP_T(prescalar, int);
    STORE_PROP(prescalarPasses);
    STORE_PROP(prescalarThreshold);
    STORE_PROP(superxbrSharpness);
    STORE_PROP(superxbrEdgeStrength);
    STORE_PROP_T(nnedi3Neurons, int);
    STORE_PROP_T(nnedi3Window, int);
    STORE_PROP_T(nnedi3UploadMethod, int);
    STORE_PROP(debanding);
    STORE_PROP(debandIterations);
    STORE_PROP(debandThreshold);
    STORE_PROP(debandRange);
    STORE_PROP(debandGrain);
    STORE_PROP(gamma);
    STORE_PROP_T(targetPrim, int);
    STORE_PROP_T(targetTrc, int);
    STORE_PROP(iccAutodetect);
    STORE_PROP(iccLocation);
    STORE_PROP_T(audioRenderer, int);
    STORE_PROP(pulseHost);
    STORE_PROP(pulseSink);
    STORE_PROP(pulseBuffer);
    STORE_PROP(pulseLatencyHack);
    STORE_PROP(alsaMixer);
    STORE_PROP(alsaMixerDevice);
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
    STORE_PROP_T(whenToShowControls, int);
    STORE_PROP(controlShowDuration);
    STORE_PROP(xrandrModes);
    STORE_PROP(xrandrChangeDelay);
    STORE_PROP(switchXrandrBackToOldMode);
    STORE_PROP(restoreXrandrOnExit);
    STORE_PROP_T(framedroppingMode, int);
    STORE_PROP_T(decoderDroppingMode, int);
    STORE_PROP_T(syncMode, int);
    STORE_PROP(audioDropSize);
    STORE_PROP(maxAudioChange);
    STORE_PROP(maxVideoChange);
    STORE_PROP_T(subPlacementX, int);
    STORE_PROP_T(subPlacementY, int);
    STORE_PROP(overrideSubPlacement);
    STORE_PROP(subtitlePosition);
    STORE_PROP(subtitlesUseMargins);
    STORE_PROP(subtitlesInGrayscale);
    STORE_PROP(subtitlesWithFixedTiming);
    STORE_PROP(subtitlesClearOnSeek);
    STORE_PROP_T(assOverrideMode, int);
    STORE_PROP(subsFont);
    STORE_PROP(fontBold);
    STORE_PROP(fontSize);
    STORE_PROP(borderSize);
    STORE_PROP(borderShadowOffset);
    STORE_PROP_T(subsAlignment, int);
    STORE_PROP(subsMarginX);
    STORE_PROP(subsMarginY);
    STORE_PROP(subsAreRelativeToVideoFrame);
    STORE_PROP(subsColor);
    STORE_PROP(borderColor);
    STORE_PROP(shadowColor);
    STORE_PROP(preferDefaultSubs);
    STORE_PROP(preferExternalSubs);
    STORE_PROP(ignoreEmbeddedSubs);
    STORE_PROP(subtitleAutoloadPath);
    STORE_PROP(subtitleDatabase);
    STORE_PROP(seekFast);
    STORE_PROP(showChapterMarks);
    STORE_PROP(openNextFile);
    STORE_PROP(showTimeTooltip);
    STORE_PROP_T(timeTooltipLocation, int);
    STORE_PROP(osdFont);
    STORE_PROP(osdSize);
    STORE_PROP(brightness);
    STORE_PROP(contrast);
    STORE_PROP(hue);
    STORE_PROP(saturation);
    return m;
}

void Settings::fromVMap(const QVariantMap &m)
{
    READ_PROP_T(playerOpenMode, OpenSame, int);
    READ_PROP(useTrayIcon, false);
    READ_PROP(showOSD, false);
    READ_PROP(limitWindowProportions, false);
    READ_PROP(disableDiscMenu, false);
    READ_PROP_T(titleBar, FileNameOnly, int);
    READ_PROP(replaceFileNameInTitleBar, false);
    READ_PROP(rememberRecentlyOpened, false)
    READ_PROP(rememberLastPlaylist, false);
    READ_PROP(rememberWindowPosition, false);
    READ_PROP(rememberWindowSize, false);
    READ_PROP(rememberPanScanZoom, false);
    READ_PROP_T(logoDisplayed, InternalLogo, int);
    READ_PROP_T(logoLocation, QString(), int);
    READ_PROP_T(playbackProgress, PlayTimes, int);
    READ_PROP(playbackTimes, 1);
    READ_PROP(rewindPlaylist, false);
    READ_PROP(volumeStep, 10);
    READ_PROP(speedStep, 0);
    READ_PROP(subtitleTracks, QString());
    READ_PROP(audioTracks, QString());
    READ_PROP_T(zoomMethod, Zoom100, int);
    READ_PROP(autoFitFactor, 75);
    READ_PROP(autoZoom, true);
    READ_PROP(autoloadAudio, false);
    READ_PROP(autoloadSubtitles, false);
    READ_PROP(audioBalance,0);
    READ_PROP(audioVolume, 100);
    READ_PROP(videoIsDumb, false);
    READ_PROP_T(videoBackend, AutoBackend, int);
    READ_PROP_T(framebufferDepth, DepthOf16, int);
    READ_PROP(framebufferAlpha, false);
    READ_PROP_T(alphaMode, BlendAlpha, int);
    READ_PROP(sharpen, 0);
    READ_PROP(dither, false);
    READ_PROP(ditherDepth, 0);
    READ_PROP_T(ditherType, Fruit, int);
    READ_PROP(ditherFruitSize, 4);
    READ_PROP(temporalDither, false);
    READ_PROP(temporalPeriod, 1);
    READ_PROP(downscaleCorrectly, false);
    READ_PROP(scaleInLinearLight, false);
    READ_PROP(temporalInterpolation, false);
    READ_PROP(blendSubtitles, false);
    READ_PROP(sigmoidizedUpscaling, false);
    READ_PROP(sigmoidCenter, 0.75);
    READ_PROP(sigmoidSlope, 6.5);
    READ_PROP_T(scaleScalar, Bilinear, int);
    READ_PROP(scaleParam1, 0);
    READ_PROP(scaleParam2, 0);
    READ_PROP(scaleRadius, 0);
    READ_PROP(scaleAntiRing, 0);
    READ_PROP(scaleBlur, 0);
    READ_PROP(scaleWindowParam, 0);
    READ_PROP_T(scaleWindow, BoxWindow, int);
    READ_PROP(scaleParam1Set, false);
    READ_PROP(scaleParam2Set, false);
    READ_PROP(scaleRadiusSet, false);
    READ_PROP(scaleAntiRingSet, false);
    READ_PROP(scaleBlurSet, false);
    READ_PROP(scaleWindowParamSet, false);
    READ_PROP(scaleWindowSet, false);
    READ_PROP(scaleClamp, false);
    READ_PROP_T(dscaleScalar, Unset, int);
    READ_PROP(dscaleParam1, 0);
    READ_PROP(dscaleParam2, 0);
    READ_PROP(dscaleRadius, 0);
    READ_PROP(dscaleAntiRing, 0);
    READ_PROP(dscaleBlur, 0);
    READ_PROP(dscaleWindowParam, 0);
    READ_PROP_T(dscaleWindow, BoxWindow, int);
    READ_PROP(dscaleParam1Set, false);
    READ_PROP(dscaleParam2Set, false);
    READ_PROP(dscaleRadiusSet, false);
    READ_PROP(dscaleAntiRingSet, false);
    READ_PROP(dscaleBlurSet, false);
    READ_PROP(dscaleWindowParamSet, false);
    READ_PROP(dscaleWindowSet, false);
    READ_PROP(dscaleClamp, false);
    READ_PROP_T(cscaleScalar, Bilinear, int);
    READ_PROP(cscaleParam1, 0);
    READ_PROP(cscaleParam2, 0);
    READ_PROP(cscaleRadius, 0);
    READ_PROP(cscaleAntiRing, 0);
    READ_PROP(cscaleBlur, 0);
    READ_PROP(cscaleWindowParam, 0);
    READ_PROP_T(cscaleWindow, BoxWindow, int);
    READ_PROP(cscaleParam1Set, false);
    READ_PROP(cscaleParam2Set, false);
    READ_PROP(cscaleRadiusSet, false);
    READ_PROP(cscaleAntiRingSet, false);
    READ_PROP(cscaleBlurSet, false);
    READ_PROP(cscaleWindowParamSet, false);
    READ_PROP(cscaleWindowSet, false);
    READ_PROP(cscaleClamp, false);
    READ_PROP_T(tscaleScalar, tOversample, int);
    READ_PROP(tscaleParam1, 0);
    READ_PROP(tscaleParam2, 0);
    READ_PROP(tscaleRadius, 0);
    READ_PROP(tscaleAntiRing, 0);
    READ_PROP(tscaleBlur, 0);
    READ_PROP(tscaleWindowParam, 0);
    READ_PROP_T(tscaleWindow, BoxWindow, int);
    READ_PROP(tscaleParam1Set, false);
    READ_PROP(tscaleParam2Set, false);
    READ_PROP(tscaleRadiusSet, false);
    READ_PROP(tscaleAntiRingSet, false);
    READ_PROP(tscaleBlurSet, false);
    READ_PROP(tscaleWindowParamSet, false);
    READ_PROP(tscaleWindowSet, false);
    READ_PROP(tscaleClamp, false);
    READ_PROP_T(prescalar, NoPrescalar, int);
    READ_PROP(prescalarPasses, 1);
    READ_PROP(prescalarThreshold, 2.0);
    READ_PROP(superxbrSharpness, 1.0);
    READ_PROP(superxbrEdgeStrength, 1.0);
    READ_PROP_T(nnedi3Neurons, NeuronsOf32, int);
    READ_PROP_T(nnedi3Window, WindowOf8x4, int);
    READ_PROP_T(nnedi3UploadMethod, UboUploading, int);
    READ_PROP(debanding, false);
    READ_PROP(debandIterations, 1);
    READ_PROP(debandThreshold, 64.0);
    READ_PROP(debandRange, 16.0);
    READ_PROP(debandGrain, 48.0);
    READ_PROP(gamma, 1.0);
    READ_PROP_T(targetPrim, AutoPrim, int);
    READ_PROP_T(targetTrc, AutoTrc, int);
    READ_PROP(iccAutodetect, true);
    READ_PROP(iccLocation, QString());
    READ_PROP_T(audioRenderer, PulseAudio, int);
    READ_PROP(pulseHost, QString());
    READ_PROP(pulseSink, QString());
    READ_PROP(pulseBuffer, 250);
    READ_PROP(pulseLatencyHack, false);
    READ_PROP(alsaMixer, QString());
    READ_PROP(alsaMixerDevice, QString("default"));
    READ_PROP(alsaMixerName, QString("Master"));
    READ_PROP(alsaMixerIndex, 0);
    READ_PROP(alsaResample, false);
    READ_PROP(alsaIgnoreChmap, false);
    READ_PROP(ossDspDevice, QString("/dev/dsp"));
    READ_PROP(ossMixerDevice, QString("/dev/mixer"));
    READ_PROP(ossMixerChannel, QString("pcm"));
    READ_PROP(nullIsUntimed, false);
    READ_PROP(nullOutburst, 256);
    READ_PROP(nullBufferLength, 0.2);
    READ_PROP(shaderFiles, QStringList());
    READ_PROP(preShaders, QStringList());
    READ_PROP(postShaders, QStringList());
    READ_PROP(shaderPresets, ShaderPresetList());
    READ_PROP(fullscreenMonitor, QString("Current"));
    READ_PROP(fullscreenLaunch, false);
    READ_PROP(exitFullscreenAtEnd, false);
    READ_PROP(hidePanelsInFullscreen, false);
    READ_PROP(hideControlsInFullscreen, true);
    READ_PROP_T(whenToShowControls, NeverShow, int)
    READ_PROP(controlShowDuration, 0);
    READ_PROP(xrandrModes, XrandrModeList());
    READ_PROP(xrandrChangeDelay, 0);
    READ_PROP(switchXrandrBackToOldMode, false);
    READ_PROP(restoreXrandrOnExit, false);
    READ_PROP_T(framedroppingMode, VideoDrops, int);
    READ_PROP_T(decoderDroppingMode, DecoderDefault, int);
    READ_PROP_T(syncMode, SyncToAudio, int);
    READ_PROP(audioDropSize, 0.02);
    READ_PROP(maxAudioChange, 0.12);
    READ_PROP(maxVideoChange, 1.0);
    READ_PROP_T(subPlacementX, SubtitlesInCenter, int);
    READ_PROP_T(subPlacementY, SubtitlesAtBottom, int);
    READ_PROP(overrideSubPlacement, false);
    READ_PROP(subtitlePosition, 100);
    READ_PROP(subtitlesUseMargins, true);
    READ_PROP(subtitlesInGrayscale, false);
    READ_PROP(subtitlesWithFixedTiming, true);
    READ_PROP(subtitlesClearOnSeek, false);
    READ_PROP_T(assOverrideMode, NoOverride, int);
    READ_PROP(subsFont, QString("sans-serif"));
    READ_PROP(fontBold, false);
    READ_PROP(fontSize, 55);
    READ_PROP(borderSize, 3);
    READ_PROP(borderShadowOffset, 0);
    READ_PROP_T(subsAlignment, Bottom, int);
    READ_PROP(subsMarginX, 0);
    READ_PROP(subsMarginY, 0);
    READ_PROP(subsAreRelativeToVideoFrame, true);
    READ_PROP(subsColor, QColor(255,255,255));
    READ_PROP(borderColor, QColor());
    READ_PROP(shadowColor, QColor());
    READ_PROP(preferDefaultSubs, true);
    READ_PROP(preferExternalSubs, true);
    READ_PROP(ignoreEmbeddedSubs, false);
    READ_PROP(subtitleAutoloadPath, QString(".;./subtitles;./subs"));
    READ_PROP(subtitleDatabase, QString("www.opensubtitles.org/isdb"));
    READ_PROP(seekFast, true);
    READ_PROP(showChapterMarks, true);
    READ_PROP(openNextFile, false);
    READ_PROP(showTimeTooltip, false);
    READ_PROP_T(timeTooltipLocation, AboveSeekbar, int);
    READ_PROP(osdFont, QString("sans-serif"));
    READ_PROP(osdSize, 55);
    READ_PROP(brightness, 0);
    READ_PROP(contrast, 0);
    READ_PROP(hue, 0);
    READ_PROP(saturation, 0);
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

    // Expand every item on pageTree
    QList<QTreeWidgetItem*> stack;
    stack.append(ui->pageTree->invisibleRootItem());
    while (!stack.isEmpty()) {
        QTreeWidgetItem* item = stack.takeFirst();
        item->setExpanded(true);
        for (int i = 0; i < item->childCount(); ++i)
            stack.push_front(item->child(i));
    }
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}

void SettingsWindow::updateAcceptedSettings() {
    Settings &s = acceptedSettings;
    s.videoIsDumb = ui->videoDumbMode->isChecked();

    s.videoBackend = (Settings::VideoBackend)ui->videoBackend->currentIndex();
    s.framebufferDepth = (Settings::FBDepth)ui->videoFramebuffer->currentIndex();
    s.framebufferAlpha = ui->videoUseAlpha->isChecked();
    s.alphaMode = (Settings::AlphaMode)ui->videoAlphaMode->currentIndex();
    s.sharpen = ui->videoSharpen->value();

    s.dither = ui->ditherDithering->isChecked();
    s.ditherDepth = ui->ditherDepth->value();
    s.ditherType = (Settings::DitherType)ui->ditherType->currentIndex();
    s.ditherFruitSize = ui->ditherFruitSize->value();
    s.temporalDither = ui->ditherTemporal->isChecked();
    s.temporalPeriod = ui->ditherTemporalPeriod->value();

    s.downscaleCorrectly = ui->scalingCorrectDownscaling->isChecked();
    s.scaleInLinearLight = ui->scalingInLinearLight->isChecked();
    s.temporalInterpolation = ui->scalingTemporalInterpolation->isChecked();
    s.blendSubtitles = ui->scalingBlendSubtitles->isChecked();
    s.sigmoidizedUpscaling = ui->scalingSigmoidizedUpscaling->isChecked();
    s.sigmoidCenter = ui->sigmoidizedCenter->value();
    s.sigmoidSlope = ui->sigmoidizedSlope->value();

    s.scaleScalar = (Settings::ScaleScalar)ui->scaleScalar->currentIndex();
    s.scaleParam1 = ui->scaleParam1Value->value();
    s.scaleParam2 = ui->scaleParam2Value->value();
    s.scaleAntiRing = ui->scaleAntiRingValue->value();
    s.scaleBlur = ui->scaleBlurValue->value();
    s.scaleWindowParam = ui->scaleWindowParamValue->value();
    s.scaleWindow = (Settings::ScaleWindow)ui->scaleWindowValue->currentIndex();
    s.scaleParam1Set = ui->scaleParam1Set->isChecked();
    s.scaleParam2Set = ui->scaleParam2Set->isChecked();
    s.scaleAntiRingSet = ui->scaleAntiRingSet->isChecked();
    s.scaleBlurSet = ui->scaleBlurSet->isChecked();
    s.scaleWindowParamSet = ui->scaleWindowParamSet->isChecked();
    s.scaleWindowSet = ui->scaleWindowSet->isChecked();
    s.scaleClamp = ui->scaleClamp->isChecked();

    s.dscaleScalar = (Settings::ScaleScalar)(ui->dscaleScalar->currentIndex() - 1);
    s.dscaleParam1 = ui->dscaleParam1Value->value();
    s.dscaleParam2 = ui->dscaleParam2Value->value();
    s.dscaleAntiRing = ui->dscaleAntiRingValue->value();
    s.dscaleBlur = ui->dscaleBlurValue->value();
    s.dscaleWindowParam = ui->dscaleWindowParamValue->value();
    s.dscaleWindow = (Settings::ScaleWindow)ui->dscaleWindowValue->currentIndex();
    s.dscaleParam1Set = ui->dscaleParam1Set->isChecked();
    s.dscaleParam2Set = ui->dscaleParam2Set->isChecked();
    s.dscaleAntiRingSet = ui->dscaleAntiRingSet->isChecked();
    s.dscaleBlurSet = ui->dscaleBlurSet->isChecked();
    s.dscaleWindowParamSet = ui->dscaleWindowParamSet->isChecked();
    s.dscaleWindowSet = ui->dscaleWindowSet->isChecked();
    s.dscaleClamp = ui->dscaleClamp->isChecked();

    s.cscaleScalar = (Settings::ScaleScalar)ui->cscaleScalar->currentIndex();
    s.cscaleParam1 = ui->cscaleParam1Value->value();
    s.cscaleParam2 = ui->cscaleParam2Value->value();
    s.cscaleAntiRing = ui->cscaleAntiRingValue->value();
    s.cscaleBlur = ui->cscaleBlurValue->value();
    s.cscaleWindowParam = ui->cscaleWindowParamValue->value();
    s.cscaleWindow = (Settings::ScaleWindow)ui->cscaleWindowValue->currentIndex();
    s.cscaleParam1Set = ui->cscaleParam1Set->isChecked();
    s.cscaleParam2Set = ui->cscaleParam2Set->isChecked();
    s.cscaleAntiRingSet = ui->cscaleAntiRingSet->isChecked();
    s.cscaleBlurSet = ui->cscaleBlurSet->isChecked();
    s.cscaleWindowParamSet = ui->cscaleWindowParamSet->isChecked();
    s.cscaleWindowSet = ui->cscaleWindowSet->isChecked();
    s.cscaleClamp = ui->cscaleClamp->isChecked();

    s.tscaleScalar = (Settings::TimeScalar)ui->tscaleScalar->currentIndex();
    s.tscaleParam1 = ui->tscaleParam1Value->value();
    s.tscaleParam2 = ui->tscaleParam2Value->value();
    s.tscaleAntiRing = ui->tscaleAntiRingValue->value();
    s.tscaleBlur = ui->tscaleBlurValue->value();
    s.tscaleWindowParam = ui->tscaleWindowParamValue->value();
    s.tscaleWindow = (Settings::ScaleWindow)ui->tscaleWindowValue->currentIndex();
    s.tscaleParam1Set = ui->tscaleParam1Set->isChecked();
    s.tscaleParam2Set = ui->tscaleParam2Set->isChecked();
    s.tscaleAntiRingSet = ui->tscaleAntiRingSet->isChecked();
    s.tscaleBlurSet = ui->tscaleBlurSet->isChecked();
    s.tscaleWindowParamSet = ui->tscaleWindowParamSet->isChecked();
    s.tscaleWindowSet = ui->tscaleWindowSet->isChecked();
    s.tscaleClamp = ui->tscaleClamp->isChecked();

    s.debanding = ui->debandEnabled->isChecked();

    s.framedroppingMode = (Settings::FramedropMode)ui->framedroppingMode->currentIndex();
    s.decoderDroppingMode = (Settings::DecoderDropMode)ui->framedroppingDecoderMode->currentIndex();
    s.syncMode = (Settings::SyncMode)ui->syncMode->currentIndex();
    s.audioDropSize = ui->syncAudioDropSize->value();
    s.maxAudioChange = ui->syncMaxAudioChange->value();
    s.maxVideoChange = ui->syncMaxVideoChange->value();

    s.subtitlesInGrayscale = ui->subtitlesForceGrayscale->isChecked();
}

void SettingsWindow::takeSettings(const Settings &s)
{
    ui->videoDumbMode->setChecked(s.videoIsDumb);

    ui->videoBackend->setCurrentIndex(s.videoBackend);
    ui->videoFramebuffer->setCurrentIndex(s.framebufferDepth);
    ui->videoUseAlpha->setChecked(s.framebufferAlpha);
    ui->videoAlphaMode->setCurrentIndex(s.alphaMode);
    ui->videoSharpen->setValue(s.sharpen);

    ui->ditherDepth->setValue(s.ditherDepth);
    ui->ditherType->setCurrentIndex(s.ditherType);
    ui->ditherFruitSize->setValue(s.ditherFruitSize);
    ui->ditherTemporal->setChecked(s.temporalDither);
    ui->ditherTemporalPeriod->setValue(s.temporalPeriod);

    ui->scalingCorrectDownscaling->setChecked(s.downscaleCorrectly);
    ui->scalingInLinearLight->setChecked(s.scaleInLinearLight);
    ui->scalingTemporalInterpolation->setChecked(s.temporalInterpolation);
    ui->scalingBlendSubtitles->setChecked(s.blendSubtitles);
    ui->scalingSigmoidizedUpscaling->setChecked(s.sigmoidizedUpscaling);
    ui->sigmoidizedCenter->setValue(s.sigmoidCenter);
    ui->sigmoidizedSlope->setValue(s.sigmoidSlope);

    ui->scaleScalar->setCurrentIndex(s.scaleScalar);
    ui->scaleParam1Value->setValue(s.scaleParam1);
    ui->scaleParam2Value->setValue(s.scaleParam2);
    ui->scaleRadiusValue->setValue(s.scaleRadius);
    ui->scaleAntiRingValue->setValue(s.scaleAntiRing);
    ui->scaleBlurValue->setValue(s.scaleBlur);
    ui->scaleWindowParamValue->setValue(s.scaleWindowParam);
    ui->scaleWindowValue->setCurrentIndex(s.scaleWindow);
    ui->scaleParam1Set->setChecked(s.scaleParam1Set);
    ui->scaleParam2Set->setChecked(s.scaleParam2Set);
    ui->scaleRadiusSet->setChecked(s.scaleRadiusSet);
    ui->scaleAntiRingSet->setChecked(s.scaleAntiRingSet);
    ui->scaleBlurSet->setChecked(s.scaleBlurSet);
    ui->scaleWindowParamSet->setChecked(s.scaleWindowParamSet);
    ui->scaleWindowSet->setChecked(s.scaleWindowSet);
    ui->scaleClamp->setChecked(s.scaleClamp);

    ui->dscaleScalar->setCurrentIndex(s.dscaleScalar + 1);
    ui->dscaleParam1Value->setValue(s.dscaleParam1);
    ui->dscaleParam2Value->setValue(s.dscaleParam2);
    ui->dscaleRadiusValue->setValue(s.dscaleRadius);
    ui->dscaleAntiRingValue->setValue(s.dscaleAntiRing);
    ui->dscaleBlurValue->setValue(s.dscaleBlur);
    ui->dscaleWindowParamValue->setValue(s.dscaleWindowParam);
    ui->dscaleWindowValue->setCurrentIndex(s.dscaleWindow);
    ui->dscaleParam1Set->setChecked(s.dscaleParam1Set);
    ui->dscaleParam2Set->setChecked(s.dscaleParam2Set);
    ui->dscaleRadiusSet->setChecked(s.dscaleRadiusSet);
    ui->dscaleAntiRingSet->setChecked(s.dscaleAntiRingSet);
    ui->dscaleBlurSet->setChecked(s.dscaleBlurSet);
    ui->dscaleWindowParamSet->setChecked(s.dscaleWindowParamSet);
    ui->dscaleWindowSet->setChecked(s.dscaleWindowSet);
    ui->dscaleClamp->setChecked(s.dscaleClamp);

    ui->cscaleScalar->setCurrentIndex(s.cscaleScalar);
    ui->cscaleParam1Value->setValue(s.cscaleParam1);
    ui->cscaleParam2Value->setValue(s.cscaleParam2);
    ui->cscaleRadiusValue->setValue(s.cscaleRadius);
    ui->cscaleAntiRingValue->setValue(s.cscaleAntiRing);
    ui->cscaleBlurValue->setValue(s.cscaleBlur);
    ui->cscaleWindowParamValue->setValue(s.cscaleWindowParam);
    ui->cscaleWindowValue->setCurrentIndex(s.cscaleWindow);
    ui->cscaleParam1Set->setChecked(s.cscaleParam1Set);
    ui->cscaleParam2Set->setChecked(s.cscaleParam2Set);
    ui->cscaleRadiusSet->setChecked(s.cscaleRadiusSet);
    ui->cscaleAntiRingSet->setChecked(s.cscaleAntiRingSet);
    ui->cscaleBlurSet->setChecked(s.cscaleBlurSet);
    ui->cscaleWindowParamSet->setChecked(s.cscaleWindowParamSet);
    ui->cscaleWindowSet->setChecked(s.cscaleWindowSet);
    ui->cscaleClamp->setChecked(s.cscaleClamp);

    ui->tscaleScalar->setCurrentIndex(s.tscaleScalar);
    ui->tscaleParam1Value->setValue(s.tscaleParam1);
    ui->tscaleParam2Value->setValue(s.tscaleParam2);
    ui->tscaleRadiusValue->setValue(s.tscaleRadius);
    ui->tscaleAntiRingValue->setValue(s.tscaleAntiRing);
    ui->tscaleBlurValue->setValue(s.tscaleBlur);
    ui->tscaleWindowParamValue->setValue(s.tscaleWindowParam);
    ui->tscaleWindowValue->setCurrentIndex(s.tscaleWindow);
    ui->tscaleParam1Set->setChecked(s.tscaleParam1Set);
    ui->tscaleParam2Set->setChecked(s.tscaleParam2Set);
    ui->tscaleRadiusSet->setChecked(s.tscaleRadiusSet);
    ui->tscaleAntiRingSet->setChecked(s.tscaleAntiRingSet);
    ui->tscaleBlurSet->setChecked(s.tscaleBlurSet);
    ui->tscaleWindowParamSet->setChecked(s.tscaleWindowParamSet);
    ui->tscaleWindowSet->setChecked(s.tscaleWindowSet);
    ui->tscaleClamp->setChecked(s.tscaleClamp);

    ui->debandEnabled->setChecked(s.debanding);

    ui->framedroppingMode->setCurrentIndex(s.framedroppingMode);
    ui->framedroppingDecoderMode->setCurrentIndex(s.framedroppingMode);
    ui->syncMode->setCurrentIndex(s.syncMode);
    ui->syncMaxAudioChange->setValue(s.maxAudioChange);
    ui->syncMaxVideoChange->setValue(s.maxVideoChange);

    ui->subtitlesForceGrayscale->setChecked(s.subtitlesInGrayscale);

    on_prescalarMethod_currentIndexChanged(s.prescalar);
    on_audioRenderer_currentIndexChanged(s.audioRenderer);
    on_videoDumbMode_toggled(s.videoIsDumb);

    acceptedSettings = s;
}

void SettingsWindow::sendSignals()
{
    QMap<QString,QString> params;
    QStringList cmdline;

    params["backend"] = Settings::videoBackendToText[acceptedSettings.videoBackend];
    params["fbo-format"] = Settings::fbDepthToText[acceptedSettings.framebufferDepth][acceptedSettings.framebufferAlpha];
    params["alpha"] = Settings::alphaModeToText[acceptedSettings.alphaMode];
    params["sharpen"] = QString::number(acceptedSettings.sharpen);

    if (acceptedSettings.dither) {
        params["dither-depth"] = acceptedSettings.ditherDepth ?
                    QString::number(acceptedSettings.ditherDepth) :
                    "auto";
        params["dither"] = Settings::ditherTypeToText[acceptedSettings.ditherType];
        if (acceptedSettings.ditherFruitSize)
            params["dither-size-fruit"] = QString::number(acceptedSettings.ditherFruitSize);
    }
    if (acceptedSettings.temporalDither) {
        params["temporal-dither"] = QString();
        params["temporal-dither-period"] = QString::number(acceptedSettings.temporalPeriod);
    }

    if (acceptedSettings.downscaleCorrectly)
        params["correct-downscaling"] = QString();
    if (acceptedSettings.scaleInLinearLight)
        params["linear-scaling"] = QString();
    if (acceptedSettings.temporalInterpolation)
        params["interpolation"] = QString();
    if (acceptedSettings.blendSubtitles)
        params["blend-subtitles"] = QString();
    if (acceptedSettings.sigmoidizedUpscaling) {
        params["sigmoid-upscaling"] = QString();
        params["sigmoid-center"] = QString::number(acceptedSettings.sigmoidCenter);
        params["sigmoid-slope"] = QString::number(acceptedSettings.sigmoidSlope);
    }

    params["scale"] = Settings::scaleScalarToText[acceptedSettings.scaleScalar];
    if (acceptedSettings.scaleParam1Set)
        params["scale-param1"] = QString::number(acceptedSettings.scaleParam1);
    if (acceptedSettings.scaleParam2Set)
        params["scale-param2"] = QString::number(acceptedSettings.scaleParam2);
    if (acceptedSettings.scaleRadiusSet)
        params["scale-radius"] = QString::number(acceptedSettings.scaleRadius);
    if (acceptedSettings.scaleAntiRingSet)
        params["scale-antiring"] = QString::number(acceptedSettings.scaleAntiRing);
    if (acceptedSettings.scaleBlurSet)
        params["scale-blur"] = QString::number(acceptedSettings.scaleBlur);
    if (acceptedSettings.scaleWindowParamSet)
        params["scale-wparam"] = QString::number(acceptedSettings.scaleWindowParam);
    if (acceptedSettings.scaleWindowSet)
        params["scale-window"] = Settings::scaleWindowToText[acceptedSettings.scaleWindow];
    if (acceptedSettings.scaleClamp)
        params["scale-clamp"] = QString();

    if (acceptedSettings.dscaleScalar != Settings::Unset)
        params["dscale"] = Settings::scaleScalarToText[acceptedSettings.dscaleScalar];
    if (acceptedSettings.dscaleParam1Set)
        params["dscale-param1"] = QString::number(acceptedSettings.dscaleParam1);
    if (acceptedSettings.dscaleParam2Set)
        params["dscale-param2"] = QString::number(acceptedSettings.dscaleParam2);
    if (acceptedSettings.dscaleRadiusSet)
        params["dscale-radius"] = QString::number(acceptedSettings.dscaleRadius);
    if (acceptedSettings.dscaleAntiRingSet)
        params["dscale-antiring"] = QString::number(acceptedSettings.dscaleAntiRing);
    if (acceptedSettings.dscaleBlurSet)
        params["dscale-blur"] = QString::number(acceptedSettings.dscaleBlur);
    if (acceptedSettings.dscaleWindowParamSet)
        params["dscale-wparam"] = QString::number(acceptedSettings.dscaleWindowParam);
    if (acceptedSettings.dscaleWindowSet)
        params["dscale-window"] = Settings::scaleWindowToText[acceptedSettings.dscaleWindow];
    if (acceptedSettings.dscaleClamp)
        params["dscale-clamp"] = QString();

    params["cscale"] = Settings::scaleScalarToText[acceptedSettings.cscaleScalar];
    if (acceptedSettings.cscaleParam1Set)
        params["cscale-param1"] = QString::number(acceptedSettings.cscaleParam1);
    if (acceptedSettings.cscaleParam2Set)
        params["cscale-param2"] = QString::number(acceptedSettings.cscaleParam2);
    if (acceptedSettings.cscaleRadiusSet)
        params["cscale-radius"] = QString::number(acceptedSettings.cscaleRadius);
    if (acceptedSettings.cscaleAntiRingSet)
        params["cscale-antiring"] = QString::number(acceptedSettings.cscaleAntiRing);
    if (acceptedSettings.cscaleBlurSet)
        params["cscale-blur"] = QString::number(acceptedSettings.cscaleBlur);
    if (acceptedSettings.cscaleWindowParamSet)
        params["cscale-wparam"] = QString::number(acceptedSettings.cscaleWindowParam);
    if (acceptedSettings.cscaleWindowSet)
        params["cscale-window"] = Settings::scaleWindowToText[acceptedSettings.cscaleWindow];
    if (acceptedSettings.cscaleClamp)
        params["cscale-clamp"] = QString();

    params["tscale"] = Settings::timeScalarToText[acceptedSettings.tscaleScalar];
    if (acceptedSettings.tscaleParam1Set)
        params["tscale-param1"] = QString::number(acceptedSettings.tscaleParam1);
    if (acceptedSettings.tscaleParam2Set)
        params["tscale-param2"] = QString::number(acceptedSettings.tscaleParam2);
    if (acceptedSettings.tscaleRadiusSet)
        params["tscale-radius"] = QString::number(acceptedSettings.tscaleRadius);
    if (acceptedSettings.tscaleAntiRingSet)
        params["tscale-antiring"] = QString::number(acceptedSettings.tscaleAntiRing);
    if (acceptedSettings.tscaleBlurSet)
        params["tscale-blur"] = QString::number(acceptedSettings.tscaleBlur);
    if (acceptedSettings.tscaleWindowParamSet)
        params["tscale-wparam"] = QString::number(acceptedSettings.tscaleWindowParam);
    if (acceptedSettings.tscaleWindowSet)
        params["tscale-window"] = Settings::scaleWindowToText[acceptedSettings.tscaleWindow];
    if (acceptedSettings.tscaleClamp)
        params["tscale-clamp"] = QString();

    if (acceptedSettings.debanding)
        params["deband"] = QString();

    QMapIterator<QString,QString> i(params);
    while (i.hasNext()) {
        i.next();
        if (!i.value().isEmpty()) {
            cmdline.append(QString("%1=%2").arg(i.key(),i.value()));
        } else {
            cmdline.append(i.key());
        }
    }
    voCommandLine(acceptedSettings.videoIsDumb ? "dumb-mode"
                                               : cmdline.join(':'));

    framedropMode(Settings::framedropToText[acceptedSettings.framedroppingMode]);
    decoderDropMode(Settings::decoderDropToText[acceptedSettings.decoderDroppingMode]);
    displaySyncMode(Settings::syncModeToText[acceptedSettings.syncMode]);
    audioDropSize(acceptedSettings.audioDropSize);
    maximumAudioChange(acceptedSettings.maxAudioChange);
    maximumVideoChange(acceptedSettings.maxVideoChange);
    subsAreGray(acceptedSettings.subtitlesInGrayscale);
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
            buttonRole == QDialogButtonBox::AcceptRole) {\
        updateAcceptedSettings();
        emit settingsData(acceptedSettings);
        sendSignals();
    }
    if (buttonRole == QDialogButtonBox::AcceptRole ||
            buttonRole == QDialogButtonBox::RejectRole)
        close();
}

void SettingsWindow::on_prescalarMethod_currentIndexChanged(int index)
{
    ui->prescalarStack->setCurrentIndex(index);
}

void SettingsWindow::on_audioRenderer_currentIndexChanged(int index)
{
    ui->audioRendererStack->setCurrentIndex(index);
}

void SettingsWindow::on_videoDumbMode_toggled(bool checked)
{
    ui->videoTabs->setEnabled(!checked);
}
