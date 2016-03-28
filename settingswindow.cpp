#include <QDebug>
#include <QStandardPaths>
#include <QFileInfo>
#include "settingswindow.h"
#include "ui_settingswindow.h"
#include "helpers.h"

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


QHash<QString, QStringList> SettingMap::indexedValueToText = {
    {"videoFramebuffer", {"rgb8-rgba", "rgb10-rgb10_a2", "rgba12-rgba12",\
                          "rgb16-rgba16", "rgb16f-rgba16f",\
                          "rgb32f-rgba32f"}},
    {"videoAlphaMode", {"blend", "yes", "no"}},
    {"ditherType", {"fruit", "ordered", "no"}},
    {"scaleScalar", {SCALAR_SCALARS}},
    {"scaleWindowValue", {SCALAR_WINDOWS}},
    {"dscaleScalar", {"unset", SCALAR_SCALARS}},
    {"dscaleWindowValue", {SCALAR_WINDOWS}},
    {"cscaleScalar", {SCALAR_SCALARS}},
    {"cscaleWindowValue", {SCALAR_WINDOWS}},
    {"tscaleScalar", {TIME_SCALARS}},
    {"tscaleWindowValue", {SCALAR_WINDOWS}},
    {"prescalarMethod", {"none", "superxbr", "nnedi3"}},
    {"nnedi3Neurons", {"16", "32", "64", "128"}},
    {"nnedi3Window", {"8x4", "8x6"}},
    {"nnedi3Upload", {"ubo", "shader"}},
    {"ccTargetPrim", {"auto", "bt.601-525", "bt.601-625", "bt.709",\
                      "bt.2020", "bt.470m", "apple", "adobe", "prophoto",\
                      "cie1931"}},
    {"ccTargetTRC", {"auto", "by.1886", "srgb", "linear", "gamma1.8",\
                     "gamma2.2", "gamma2.8", "prophoto"}},
    {"audioRenderer", {"pulse", "alsa", "oss", "null"}},
    {"framedroppingMode", {"no", "vo", "decoder", "decoder+vo"}},
    {"framedroppingDecoderMode", {"none", "default", "nonref", "bidir",\
                                  "nonkey", "all"}},
    {"syncMode", {"audio", "display-resample", "display-resample-vdrop",\
                  "display-resample-desync", "display-adrop",\
                  "display-vdrop"}},
    {"subtitlePlacementX", {"left", "center", "right"}},
    {"subtitlePlacementY", {"top", "center", "bottom"}},
    {"subtitlesAssOverride", {"no", "yes", "force", "signfs"}},
    {"subtitleAlignment", { "top-center", "top-right", "center-right",\
                            "bottom-right", "bottom-center", "bottom-left",\
                            "center-left", "top-left", "center-center" }},
    {"screenshotFormat", {"jpg", "png"}},
    {"debugMpv", { "no", "fatal", "error", "warn", "info", "status", "v",\
                   "debug", "trace"}}
};

QMap<QString, QString> Setting::classToProperty = {
    { "QCheckBox", "checked" },
    { "QRadioButton", "checked" },
    { "QLineEdit", "text" },
    { "QSpinBox", "value" },
    { "QDoubleSpinBox", "value" },
    { "QComboBox", "currentIndex" },
    { "QListWidget", "currentRow" },
    { "QFontComboBox", "currentText" },
    { "QScrollBar", "value" }
};

QStringList internalLogos = {
    ":/not-a-real-resource.png",
    ":/images/bitmaps/blank-screen.png",
    ":/images/bitmaps/blank-screen-gray.png",
    ":/images/bitmaps/blank-screen-triangle-circle.png"
};

void Setting::sendToControl()
{
    if (!widget) {
        qDebug() << "[Settings] attempted to send data to null widget!";
        return;
    }
    QString property = classToProperty[widget->metaObject()->className()];
    widget->setProperty(property.toUtf8().data(), value);
}

void Setting::fetchFromControl()
{
    if (!widget) {
        qDebug() << "[Settings] attempted to get data from null widget!";
        return;
    }
    QString property = classToProperty[widget->metaObject()->className()];
    value = widget->property(property.toUtf8().data());
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
        this->insert(s.name, {s.name, s.widget, i.value()});
    }
}


SettingsWindow::SettingsWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SettingsWindow)
{
    ui->setupUi(this);

    logoWidget = new LogoWidget(this);
    ui->logoImageHost->addWidget(logoWidget);

    defaultSettings = generateSettingMap();
    acceptedSettings = defaultSettings;

    ui->pageStack->setCurrentIndex(0);
    ui->videoTabs->setCurrentIndex(0);
    ui->scalingTabs->setCurrentIndex(0);
    ui->prescalarStack->setCurrentIndex(0);
    ui->audioRendererStack->setCurrentIndex(0);
    setNnedi3Available(false);

    ui->screenshotDirectoryValue->setPlaceholderText(
                QStandardPaths::writableLocation(
                    QStandardPaths::PicturesLocation) + "/mpc_shots");
    ui->encodeDirectoryValue->setPlaceholderText(
                QStandardPaths::writableLocation(
                    QStandardPaths::PicturesLocation) + "/mpc_encodes");

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
        if (Setting::classToProperty.keys().contains(item->metaObject()->className())
            && !item->objectName().isEmpty()
            && item->objectName() != "qt_spinbox_lineedit") {
            QString name = item->objectName();
            QString className = item->metaObject()->className();
            QString property = Setting::classToProperty.value(className, QString());
            QVariant value = item->property(property.toUtf8().data());
            settingMap.insert(name, {name, qobject_cast<QWidget*>(item), value});
            continue;
        }
        QObjectList children = item->children();
        foreach(QObject *child, children) {
            if (child->inherits("QWidget") || child->inherits("QLayout"))
            toParse.append(child);
        }
    };
    return settingMap;
}

void SettingsWindow::updateLogoWidget()
{
    logoWidget->setLogo(selectedLogo());
}

QString SettingsWindow::selectedLogo()
{
    return ui->logoExternal->isChecked()
                                ? ui->logoExternalLocation->text()
                                : internalLogos.value(ui->logoInternal->currentIndex());
}

void SettingsWindow::takeSettings(QVariantMap payload)
{
    acceptedSettings.fromVMap(payload);
    for (Setting &s : acceptedSettings) {
        s.sendToControl();
    }
    updateLogoWidget();
}


// The reason why we're using #define's like this instead of quoted-string
// inspection is because this way guarantees that the compile will fail if
// the names here and the names in the ui file do not match up.

#define WIDGET_LOOKUP(widget) \
    acceptedSettings[widget->objectName()].value

#define OFFSET_LOOKUP(source, widget) \
    source[widget->objectName()].value.toInt()

#define WIDGET_TO_TEXT(widget) \
    SettingMap::indexedValueToText[widget->objectName()].value(OFFSET_LOOKUP(acceptedSettings,widget), \
        SettingMap::indexedValueToText[widget->objectName()].value(OFFSET_LOOKUP(defaultSettings,widget)))

#define WIDGET_PLACEHOLD_LOOKUP(widget) \
    (WIDGET_LOOKUP(widget).toString().isEmpty() ? widget->placeholderText() : WIDGET_LOOKUP(widget).toString())

void SettingsWindow::sendSignals()
{
    emit logoSource(selectedLogo());


    QMap<QString,QString> params;
    QStringList cmdline;

    params["fbo-format"] = WIDGET_TO_TEXT(ui->videoFramebuffer).split('-').value(WIDGET_LOOKUP(ui->videoUseAlpha).toBool());
    params["alpha"] = WIDGET_TO_TEXT(ui->videoAlphaMode);
    params["sharpen"] = WIDGET_LOOKUP(ui->videoSharpen).toString();

    if (WIDGET_LOOKUP(ui->ditherDithering).toBool()) {
        params["dither-depth"] = WIDGET_LOOKUP(ui->ditherDepth).toString();
        params["dither"] = WIDGET_TO_TEXT(ui->ditherType);
        params["dither-size-fruit"] = WIDGET_LOOKUP(ui->ditherFruitSize).toString();
    }
    if (WIDGET_LOOKUP(ui->ditherTemporal).toBool()) {
        params["temporal-dither"] = QString();
        params["temporal-dither-period"] = WIDGET_LOOKUP(ui->ditherTemporalPeriod).toString();
    }
    // WIDGET_LOOKUP().toString();
    // WIDGET_LOOKUP().toBool()
    if (WIDGET_LOOKUP(ui->scalingCorrectDownscaling).toBool())
        params["correct-downscaling"] = QString();
    if (WIDGET_LOOKUP(ui->scalingInLinearLight).toBool())
        params["linear-scaling"] = QString();
    if (WIDGET_LOOKUP(ui->scalingTemporalInterpolation).toBool())
        params["interpolation"] = QString();
    if (WIDGET_LOOKUP(ui->scalingBlendSubtitles).toBool())
        params["blend-subtitles"] = QString();
    if (WIDGET_LOOKUP(ui->scalingSigmoidizedUpscaling).toBool()) {
        params["sigmoid-upscaling"] = QString();
        params["sigmoid-center"] = WIDGET_LOOKUP(ui->sigmoidizedCenter).toString();
        params["sigmoid-slope"] = WIDGET_LOOKUP(ui->sigmoidizedSlope).toString();
    }

    params["scale"] = WIDGET_TO_TEXT(ui->scaleScalar);
    if (WIDGET_LOOKUP(ui->scaleParam1Set).toBool())
        params["scale-param1"] = WIDGET_LOOKUP(ui->scaleParam1Value).toString();
    if (WIDGET_LOOKUP(ui->scaleParam2Set).toBool())
        params["scale-param2"] = WIDGET_LOOKUP(ui->scaleParam2Value).toString();
    if (WIDGET_LOOKUP(ui->scaleRadiusSet).toBool())
        params["scale-radius"] = WIDGET_LOOKUP(ui->scaleRadiusValue).toString();
    if (WIDGET_LOOKUP(ui->scaleAntiRingSet).toBool())
        params["scale-antiring"] = WIDGET_LOOKUP(ui->scaleAntiRingValue).toString();
    if (WIDGET_LOOKUP(ui->scaleBlurSet).toBool())
        params["scale-blur"] = WIDGET_LOOKUP(ui->scaleBlurValue).toString();
    if (WIDGET_LOOKUP(ui->scaleWindowParamSet).toBool())
        params["scale-wparam"] = WIDGET_LOOKUP(ui->scaleWindowParamValue).toString();
    if (WIDGET_LOOKUP(ui->scaleWindowSet).toBool())
        params["scale-window"] = WIDGET_TO_TEXT(ui->scaleWindowValue);
    if (WIDGET_LOOKUP(ui->scaleClamp).toBool())
        params["scale-clamp"] = QString();

    if (WIDGET_LOOKUP(ui->dscaleScalar).toInt())
        params["dscale"] = WIDGET_TO_TEXT(ui->dscaleScalar);
    if (WIDGET_LOOKUP(ui->dscaleParam1Set).toBool())
        params["dscale-param1"] = WIDGET_LOOKUP(ui->scaleParam1Value).toString();
    if (WIDGET_LOOKUP(ui->dscaleParam2Set).toBool())
        params["dscale-param2"] = WIDGET_LOOKUP(ui->scaleParam2Value).toString();
    if (WIDGET_LOOKUP(ui->dscaleRadiusSet).toBool())
        params["dscale-radius"] = WIDGET_LOOKUP(ui->scaleRadiusValue).toString();
    if (WIDGET_LOOKUP(ui->dscaleAntiRingSet).toBool())
        params["dscale-antiring"] = WIDGET_LOOKUP(ui->scaleAntiRingValue).toString();
    if (WIDGET_LOOKUP(ui->dscaleBlurSet).toBool())
        params["dscale-blur"] = WIDGET_LOOKUP(ui->scaleBlurValue).toString();
    if (WIDGET_LOOKUP(ui->dscaleWindowParamSet).toBool())
        params["dscale-wparam"] = WIDGET_LOOKUP(ui->scaleWindowParamValue).toString();
    if (WIDGET_LOOKUP(ui->dscaleWindowSet).toBool())
        params["dscale-window"] = WIDGET_TO_TEXT(ui->scaleWindowValue);
    if (WIDGET_LOOKUP(ui->dscaleClamp).toBool())
        params["dscale-clamp"] = QString();

    params["cscale"] = WIDGET_TO_TEXT(ui->cscaleScalar);
    if (WIDGET_LOOKUP(ui->cscaleParam1Set).toBool())
        params["ccale-param1"] = WIDGET_LOOKUP(ui->cscaleParam1Value).toString();
    if (WIDGET_LOOKUP(ui->cscaleParam2Set).toBool())
        params["cscale-param2"] = WIDGET_LOOKUP(ui->cscaleParam2Value).toString();
    if (WIDGET_LOOKUP(ui->cscaleRadiusSet).toBool())
        params["cscale-radius"] = WIDGET_LOOKUP(ui->cscaleRadiusValue).toString();
    if (WIDGET_LOOKUP(ui->cscaleAntiRingSet).toBool())
        params["cscale-antiring"] = WIDGET_LOOKUP(ui->cscaleAntiRingValue).toString();
    if (WIDGET_LOOKUP(ui->cscaleBlurSet).toBool())
        params["cscale-blur"] = WIDGET_LOOKUP(ui->cscaleBlurValue).toString();
    if (WIDGET_LOOKUP(ui->cscaleWindowParamSet).toBool())
        params["cscale-wparam"] = WIDGET_LOOKUP(ui->cscaleWindowParamValue).toString();
    if (WIDGET_LOOKUP(ui->cscaleWindowSet).toBool())
        params["cscale-window"] = WIDGET_TO_TEXT(ui->cscaleWindowValue);
    if (WIDGET_LOOKUP(ui->cscaleClamp).toBool())
        params["cscale-clamp"] = QString();

    params["tscale"] = WIDGET_TO_TEXT(ui->tscaleScalar);
    if (WIDGET_LOOKUP(ui->tscaleParam1Set).toBool())
        params["tscale-param1"] = WIDGET_LOOKUP(ui->tscaleParam1Value).toString();
    if (WIDGET_LOOKUP(ui->tscaleParam2Set).toBool())
        params["tscale-param2"] = WIDGET_LOOKUP(ui->tscaleParam2Value).toString();
    if (WIDGET_LOOKUP(ui->tscaleRadiusSet).toBool())
        params["tscale-radius"] = WIDGET_LOOKUP(ui->tscaleRadiusValue).toString();
    if (WIDGET_LOOKUP(ui->tscaleAntiRingSet).toBool())
        params["tscale-antiring"] = WIDGET_LOOKUP(ui->tscaleAntiRingValue).toString();
    if (WIDGET_LOOKUP(ui->tscaleBlurSet).toBool())
        params["tscale-blur"] = WIDGET_LOOKUP(ui->tscaleBlurValue).toString();
    if (WIDGET_LOOKUP(ui->tscaleWindowParamSet).toBool())
        params["tscale-wparam"] = WIDGET_LOOKUP(ui->tscaleWindowParamValue).toString();
    if (WIDGET_LOOKUP(ui->tscaleWindowSet).toBool())
        params["tscale-window"] = WIDGET_TO_TEXT(ui->tscaleWindowValue);
    if (WIDGET_LOOKUP(ui->tscaleClamp).toBool())
        params["tscale-clamp"] = QString();

    if (WIDGET_LOOKUP(ui->debandEnabled).toBool()) {
        params["deband"] = QString();
        params["deband-iterations"] = WIDGET_LOOKUP(ui->debandIterations).toString();
        params["deband-threshold"] = WIDGET_LOOKUP(ui->debandThreshold).toString();
        params["deband-range"] = WIDGET_LOOKUP(ui->debandRange).toString();
        params["deband-grain"] = WIDGET_LOOKUP(ui->debandGrain).toString();
    }

    int prescalar = WIDGET_LOOKUP(ui->prescalarMethod).toInt();
    if (prescalar == 2 && !parseNnedi3Fields) {
        prescalar = 0;
    }
    if (prescalar) {
        params["prescale"] = WIDGET_TO_TEXT(ui->prescalarMethod);
        params["prescale-passes"] = WIDGET_LOOKUP(ui->prescalarPasses).toString();
        params["prescale-downscaling-threshold"] = WIDGET_LOOKUP(ui->prescalarThreshold).toString();
    }
    switch (prescalar) {
    case 1:
        params["superxbr-sharpness"] = WIDGET_LOOKUP(ui->superxbrSharpness).toString();
        params["superxbr-edge-strength"] = WIDGET_LOOKUP(ui->superxbrEdgeStrength).toString();
        break;
    case 2:
        params["nnedi3-neurons"] = WIDGET_TO_TEXT(ui->nnedi3Neurons);
        params["nnedi3-window"] = WIDGET_TO_TEXT(ui->nnedi3Window);
        params["nnedi3-upload"] = WIDGET_TO_TEXT(ui->nnedi3Upload);
        break;
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
    voCommandLine(WIDGET_LOOKUP(ui->videoDumbMode).toBool()
                  ? "dumb-mode" : cmdline.join(':'));

    framedropMode(WIDGET_TO_TEXT(ui->framedroppingMode));
    decoderDropMode(WIDGET_TO_TEXT(ui->framedroppingDecoderMode));
    displaySyncMode(WIDGET_TO_TEXT(ui->syncMode));
    audioDropSize(WIDGET_LOOKUP(ui->syncAudioDropSize).toDouble());
    maximumAudioChange(WIDGET_LOOKUP(ui->syncMaxAudioChange).toDouble());
    maximumVideoChange(WIDGET_LOOKUP(ui->syncMaxVideoChange).toDouble());
    subsAreGray(WIDGET_LOOKUP(ui->subtitlesForceGrayscale).toDouble());

    screenshotDirectory(
                WIDGET_LOOKUP(ui->screenshotDirectorySet).toBool() ?
                QFileInfo(WIDGET_PLACEHOLD_LOOKUP(ui->screenshotDirectoryValue)).absoluteFilePath() : QString());

    encodeDirectory(
                WIDGET_LOOKUP(ui->encodeDirectorySet).toBool() ?
                QFileInfo(WIDGET_PLACEHOLD_LOOKUP(ui->encodeDirectoryValue)).absoluteFilePath() : QString());
    screenshotTemplate(WIDGET_PLACEHOLD_LOOKUP(ui->screenshotTemplate));
    encodeTemplate(WIDGET_PLACEHOLD_LOOKUP(ui->encodeTemplate));
    screenshotFormat(WIDGET_TO_TEXT(ui->screenshotFormat));
    clientDebuggingMessages(WIDGET_LOOKUP(ui->debugClient).toBool());
    mpvLogLevel(WIDGET_TO_TEXT(ui->debugMpv));

}

void SettingsWindow::setNnedi3Available(bool yes)
{
    parseNnedi3Fields = yes;
    ui->nnedi3Unavailable->setVisible(!yes);
    ui->nnedi3Page->setEnabled(yes);
}

void SettingsWindow::on_pageTree_itemSelectionChanged()
{
    QModelIndex modelIndex = ui->pageTree->currentIndex();
    if (!modelIndex.isValid())
        return;

    static int parentIndex[] = { 0, 4, 9, 12, 14, 15 };
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
        emit settingsData(acceptedSettings.toVMap());
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

void SettingsWindow::on_logoExternalBrowse_clicked()
{
    AsyncFileDialog *afd = new AsyncFileDialog(this);
    afd->setMode(AsyncFileDialog::SingleFile);
    connect(afd, &AsyncFileDialog::fileOpened, [&](QUrl file) {
        ui->logoExternalLocation->setText(file.toLocalFile());
        if (ui->logoExternal->isChecked())
            updateLogoWidget();
    });
    afd->show();
}

void SettingsWindow::on_logoUseInternal_clicked()
{
    updateLogoWidget();
}

void SettingsWindow::on_logoExternal_clicked()
{
    updateLogoWidget();
}

void SettingsWindow::on_logoInternal_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    updateLogoWidget();
}

void SettingsWindow::on_screenshotFormat_currentIndexChanged(int index)
{
    ui->screenshotFormatStack->setCurrentIndex(index);
}
