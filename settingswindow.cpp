#include "settingswindow.h"
#include "ui_settingswindow.h"
#include <QDebug>
#include <QDialogButtonBox>
#include <QHash>

#define SCALAR_SCALARS \
    "bilinear", "bicubic_fast", "oversample", "spline16", "spline36",\
    "spline64", "sinc", "lanczos", "gingseng", "jinc", "ewa_lanczos",\
    "ewa_hanning", "ewa_gingseng", "ewa_lanczossharp", "ewa_lanczossoft",\
    "hassnsoft",  "bicubic", "bcspline", "catmull_rom", "mitchell",\
    "robidoux", "robidouxsharp", "ewa_robidoux", "ewa_robidouxsharp",\
    "box", "nearest", "triangle", "gaussian"

#define SCALAR_WINDOWS \
    "box", "triable", "bartlett", "hanning", "hamming", "quadric", "welch",\
    "kaiser", "blackman", "gaussian", "sinc", "jinc", "sphinx"

#define TIME_SCALARS \
    "oversample", "spline16", "spline36", "spline64", "sinc", "lanczos",\
    "gingseng", "catmull_rom", "mitchell", "robidoux", "robidouxsharp",\
    "box", "nearest", "triangle", "gaussian"


QHash<QString, QStringList> Settings::indexedValueToText = {
    {"videoFramebuffer", {"rgb8-rgba", "rgb10-rgb10_a2", "rgba12-rgba12", "rgb16-rgba16", "rgb16f-rgba16f", "rgb32f-rgba32f"}},
    {"videoAlphaMode", {"blend", "yes", "no"}},
    {"ditherType", {"fruit", "ordered", "no"}},
    {"scaleScalar", {SCALAR_SCALARS}},
    {"scaleWindow", {SCALAR_WINDOWS}},
    {"dscaleScalar", {SCALAR_SCALARS}},
    {"dscaleWindow", {SCALAR_WINDOWS}},
    {"cscaleScalar", {SCALAR_SCALARS}},
    {"cscaleWindow", {SCALAR_WINDOWS}},
    {"tscaleScalar", {TIME_SCALARS}},
    {"tscaleWindow", {SCALAR_WINDOWS}},
    {"prescalarMethod", {"none", "superxbr", "needi3"}},
    {"nnedi3Neurons", {"16", "32", "64", "128"}},
    {"nnedi3Window", {"8x4", "8x6"}},
    {"nnedi3Upload", {"ubo", "shader"}},
    {"ccTargetPrim", {"auto", "bt.601-525", "bt.601-625", "bt.709", "bt.2020", "bt.470m", "apple", "adobe", "prophoto", "cie1931"}},
    {"ccTargetTRC", {"auto", "by.1886", "srgb", "linear", "gamma1.8", "gamma2.2", "gamma2.8", "prophoto"}},
    {"audioRenderer", {"pulse", "alsa", "oss", "null"}},
    {"framedroppingMode", {"no", "vo", "decoder", "decoder+vo"}},
    {"framedroppingDecoderMode", {"none", "default", "nonref", "bidir", "nonkey", "all"}},
    {"syncMode", {"audio", "display-resample", "display-resample-vdrop", "display-resample-desync", "display-adrop", "display-vdrop"}},
    {"subtitlePlacementX", {"left", "center", "right"}},
    {"subtitlePlacementY", {"top", "center", "bottom"}},
    {"subtitlesAssOverride", {"no", "yes", "force", "signfs"}},
    {"subtitleAlignment", { "top-center", "top-right", "center-right", "bottom-right", "bottom-center", "bottom-left", "center-left", "top-left", "center-center" }}
};

QVariantMap SettingMap::toVMap()
{
    QVariantMap m;
    foreach (Setting s, *this)
        m.insert(s.name, s.value);
    return m;
}

void SettingMap::fromVMap(const QVariantMap &m)
{
    // read settings from variant, but only try to insert if they are already
    // there. (don't accept nonsense.)  To use this function properly, it is
    // necessary to call SettingsWindow::generateSettingsMap first.
    QMapIterator<QString, QVariant> i(m);
    while (i.hasNext()) {
        i.next();
        if (!this->contains(i.key()))
            continue;
        Setting s = this->value(i.key());
        this->insert({s.name, s.widget, i.value()});
    }
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
    s.debandIterations = ui->debandIterations->value();
    s.debandThreshold = ui->debandThreshold->value();
    s.debandRange = ui->debandRange->value();
    s.debandGrain = ui->debandGrain->value();

    s.framedroppingMode = (Settings::FramedropMode)ui->framedroppingMode->currentIndex();
    s.decoderDroppingMode = (Settings::DecoderDropMode)ui->framedroppingDecoderMode->currentIndex();
    s.syncMode = (Settings::SyncMode)ui->syncMode->currentIndex();
    s.audioDropSize = ui->syncAudioDropSize->value();
    s.maxAudioChange = ui->syncMaxAudioChange->value();
    s.maxVideoChange = ui->syncMaxVideoChange->value();

    s.subtitlesInGrayscale = ui->subtitlesForceGrayscale->isChecked();
}

SettingMap SettingsWindow::generateSettingMap()
{
    SettingMap settingMap;
    QMap<QString> classToProperty = {
        { "QCheckBox", "checked" },
        { "QRadioButton", "checked" },
        { "QLineEdit", "text" },
        { "QSpinBox", "value" },
        { "QComboBox", "currentIndex" },
        { "QListWidget", "currentRow" },
        { "QFontComboBox", "currentText" },
        { "QScrollBar", "value" }
    };

    // The idea here is to discover all the widgets in the ui and only inspect
    // the widgets which we desire to know about.
    QObjectList toParse;
    toParse.append(this);
    while (!toParse.empty()) {
        QObject *item = toParse.takeFirst();
        if (QStringList({"QCheckBox", "QRadioButton", "QLineEdit", "QSpinBox",
                        "QComboBox", "QListWidget", "QFontComboBox",
                        "QScrollBar"}).contains(item->metaObject()->className())
            && !item->objectName().isEmpty()
            && item->objectName() != "qt_spinbox_lineedit") {
            QString name = item->objectName();
            QString className = item->metaObject()->className();
            QString property = classToProperty.value(className, QString());
            QVariant value = item->property(property);
            settingMap.insert(name, {name, item, value});
            return;
        }
        QObjectList children = item->children();
        foreach(QObject *child, children) {
            if (child->inherits("QWidget") || child->inherits("QLayout"))
            toParse.append(child);
        }
    };
    return settingMap;
}

void SettingsWindow::takeSettings(const SettingMap &s)
{
    ui->videoDumbMode->setChecked(s.videoIsDumb);

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
    ui->debandIterations->setValue(s.debandIterations);
    ui->debandThreshold->setValue(s.debandThreshold);
    ui->debandRange->setValue(s.debandRange);
    ui->debandGrain->setValue(s.debandGrain);

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

#define TEXT_LOOKUP(array, field, dflt) \
    Settings::array.value(acceptedSettings.field, Settings::array[dflt])
#define TEXT_LOOKUP2(array, field1, dflt1, field2, dflt2) \
    Settings::array.value(acceptedSettings.field1, Settings::array[dflt1]) \
                   .value(acceptedSettings.field2, TEXT_LOOKUP(array,field1,dflt1).value(dflt2))


void SettingsWindow::sendSignals()
{
    QMap<QString,QString> params;
    QStringList cmdline;

    params["fbo-format"] = TEXT_LOOKUP2(fbDepthToText, framebufferDepth, Settings::DepthOf16, framebufferAlpha, false);
    params["alpha"] = Settings::alphaModeToText.value(acceptedSettings.alphaMode, "blend");
    params["sharpen"] = QString::number(acceptedSettings.sharpen);

    if (acceptedSettings.dither) {
        params["dither-depth"] = acceptedSettings.ditherDepth ?
                    QString::number(acceptedSettings.ditherDepth) :
                    "auto";
        params["dither"] = TEXT_LOOKUP(ditherTypeToText, ditherType, Settings::Fruit);
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

    params["scale"] = TEXT_LOOKUP(scaleScalarToText, scaleScalar, Settings::Bilinear);
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
        params["scale-window"] = TEXT_LOOKUP(scaleWindowToText, scaleWindow, Settings::BoxWindow);
    if (acceptedSettings.scaleClamp)
        params["scale-clamp"] = QString();

    if (acceptedSettings.dscaleScalar != Settings::Unset)
        params["dscale"] = TEXT_LOOKUP(scaleScalarToText, dscaleScalar, Settings::Bilinear);
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
        params["dscale-window"] = TEXT_LOOKUP(scaleWindowToText, dscaleWindow, Settings::BoxWindow);
    if (acceptedSettings.dscaleClamp)
        params["dscale-clamp"] = QString();

    params["cscale"] = TEXT_LOOKUP(scaleScalarToText, cscaleScalar, Settings::Bilinear);
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
        params["cscale-window"] = TEXT_LOOKUP(scaleWindowToText, cscaleWindow, Settings::BoxWindow);
    if (acceptedSettings.cscaleClamp)
        params["cscale-clamp"] = QString();

    params["tscale"] = TEXT_LOOKUP(timeScalarToText, tscaleScalar, Settings::Oversample);
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
        params["tscale-window"] = TEXT_LOOKUP(scaleWindowToText, tscaleWindow, Settings::BoxWindow);
    if (acceptedSettings.tscaleClamp)
        params["tscale-clamp"] = QString();

    if (acceptedSettings.debanding) {
        params["deband"] = QString();
        params["deband-iterations"] = QString::number(acceptedSettings.debandIterations);
        params["deband-threshold"] = QString::number(acceptedSettings.debandThreshold);
        params["deband-range"] = QString::number(acceptedSettings.debandRange);
        params["deband-grain"] = QString::number(acceptedSettings.debandGrain);
    }

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

    framedropMode(Settings::framedropToText.value(acceptedSettings.framedroppingMode));
    decoderDropMode(Settings::decoderDropToText.value(acceptedSettings.decoderDroppingMode));
    displaySyncMode(Settings::syncModeToText.value(acceptedSettings.syncMode));
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
