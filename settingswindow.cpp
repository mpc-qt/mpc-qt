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

static QMap<QString, QString> Setting::classToProperty = {
    { "QCheckBox", "checked" },
    { "QRadioButton", "checked" },
    { "QLineEdit", "text" },
    { "QSpinBox", "value" },
    { "QComboBox", "currentIndex" },
    { "QListWidget", "currentRow" },
    { "QFontComboBox", "currentText" },
    { "QScrollBar", "value" }
};

void Setting::sendToControl()
{
    QString property = classToProperty(widget->metaObject()->className());
    widget->setProperty(property, value);
}

void Setting::fetchFromControl()
{
    QString property = classToProperty(widget->metaObject()->className());
    value = widget->property(property);
}

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

    defaultSettings = generateSettingMap();
    acceptedSettings = defaultSettings();

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
    acceptedSettings = generateSettingMap();
}

SettingMap SettingsWindow::generateSettingMap()
{
    SettingMap settingMap;


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
            QString property = Setting::classToProperty.value(className, QString());
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

void SettingsWindow::takeSettings(QVariantMap payload)
{
    acceptedSettings.fromVMap(payload);
    foreach (Setting &s, acceptedSettings) {
        s.sendToControl();
    }
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

