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
    "oversample", "spline16", "spline36", "spline64", "sinc", "lanczos",
    "gingseng", "catmull_rom", "mitchell", "robidoux", "robidouxsharp",
    "box", "nearest", "triangle", "gaussian"
};

Q_DECLARE_METATYPE(settings::ShaderPresetList);
Q_DECLARE_METATYPE(settings::XrandrModeList);
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

QVariantMap settings::toVMap()
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

void settings::fromVMap(const QVariantMap &m)
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

settings SettingsWindow::buildSettings() {
    settings s;
    s.videoIsDumb = ui->videoDumbMode->isChecked();
    s.temporalInterpolation = ui->scalingTemporalInterpolation->isChecked();
    s.blendSubtitles = ui->scalingBlendSubtitles->isChecked();
    s.dither = ui->ditherDithering->isChecked();
    s.ditherDepth = ui->ditherDepth->value();
    s.ditherType = (settings::DitherType)ui->ditherType->currentIndex();
    s.ditherFruitSize = ui->ditherFruitSize->value();
    s.temporalDither = ui->ditherTemporal->isChecked();
    s.temporalPeriod = ui->ditherTemporalPeriod->value();
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
    ui->scalingBlendSubtitles->setChecked(s.blendSubtitles);
    ui->ditherDepth->setValue(s.ditherDepth);
    ui->ditherType->setCurrentIndex(s.ditherType);
    ui->ditherFruitSize->setValue(s.ditherFruitSize);
    ui->ditherTemporal->setChecked(s.temporalDither);
    ui->ditherTemporalPeriod->setValue(s.temporalPeriod);
    ui->debandEnabled->setChecked(s.debanding);
    ui->scaleScalar->setCurrentIndex(s.scaleScalar);
    ui->dscaleScalar->setCurrentIndex(s.dscaleScalar + 1);
    ui->cscaleScalar->setCurrentIndex(s.cscaleScalar);
    ui->tscaleScalar->setCurrentIndex(s.tscaleScalar);

    on_prescalarMethod_currentIndexChanged(s.prescalar);
    on_audioRenderer_currentIndexChanged(s.audioRenderer);
    on_videoDumbMode_toggled(s.videoIsDumb);
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
