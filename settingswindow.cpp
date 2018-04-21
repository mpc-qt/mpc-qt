#include <QDebug>
#include <QStandardPaths>
#include <QFileInfo>
#include <QFileDialog>
#include <QColorDialog>
#include <QProcess>
#include <QProcessEnvironment>
#include <QList>
#include "settingswindow.h"
#include "ui_settingswindow.h"
#include "qactioneditor.h"
#include "paletteeditor.h"
#include "platform/unify.h"

#define SCALER_SCALERS \
    "bilinear", "bicubic_fast", "oversample", "spline16", "spline36",\
    "spline64", "sinc", "lanczos", "ginseng", "jinc", "ewa_lanczos",\
    "ewa_hanning", "ewa_ginseng", "ewa_lanczossharp", "ewa_lanczossoft",\
    "haasnsoft",  "bicubic", "bcspline", "catmull_rom", "mitchell",\
    "robidoux", "robidouxsharp", "ewa_robidoux", "ewa_robidouxsharp",\
    "box", "nearest", "triangle", "gaussian"

#define SCALER_WINDOWS \
    "box", "triable", "bartlett", "hanning", "hamming", "quadric", "welch",\
    "kaiser", "blackman", "gaussian", "sinc", "jinc", "sphinx"

#define TIME_SCALERS \
    "oversample", "linear",  "spline16", "spline36", "spline64", "sinc", \
    "lanczos", "ginseng", "bicubic", "bcspline", "catmull_rom", "mitchell", \
    "robidoux", "robidouxsharp", "box", "nearest", "triangle", "gaussian"


QHash<QString, QStringList> SettingMap::indexedValueToText = {
    {"interfaceIconsFallback", { ":/images/theme/black/", \
                                 ":/images/theme/white/", "" }},
    {"videoFramebuffer", {"rgb8-rgba8", "rgb10-rgb10_a2", "rgba12-rgba12",\
                          "rgb16-rgba16", "rgb16f-rgba16f",\
                          "rgb32f-rgba32f"}},
    {"videoAlphaMode", {"blend", "yes", "no"}},
    {"ditherType", {"fruit", "ordered", "no"}},
    {"scaleScaler", {SCALER_SCALERS}},
    {"scaleWindowValue", {SCALER_WINDOWS}},
    {"dscaleScaler", {"unset", SCALER_SCALERS}},
    {"dscaleWindowValue", {SCALER_WINDOWS}},
    {"cscaleScaler", {SCALER_SCALERS}},
    {"cscaleWindowValue", {SCALER_WINDOWS}},
    {"tscaleScaler", {TIME_SCALERS}},
    {"tscaleWindowValue", {SCALER_WINDOWS}},
    {"ccTargetPrim", {"auto", "bt.601-525", "bt.601-625", "bt.709",\
                      "bt.2020", "bt.470m", "apple", "adobe", "prophoto",\
                      "cie1931", "dvi-p3"}},
    {"ccTargetTRC", {"auto", "by.1886", "srgb", "linear", "gamma1.8",\
                     "gamma2.2", "gamma2.8", "prophoto", "pq", "hlg", "v-log",
                     "s-log1", "s-log2"}},
    {"ccHdrMapper", {"clip", "mobius", "reinhard", "hable", "gamma", \
                     "linear"}},
    {"audioChannels", {"auto-safe", "auto", "stereo"}},
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
    {"subtitlesAutoloadMatch", { "exact", "fuzzy", "all" }},
    {"screenshotFormat", {"jpg", "png"}},
    {"debugMpv", { "no", "fatal", "error", "warn", "info", "v", "debug",\
                   "trace"}}
};


QMap<QString, const char *> Setting::classToProperty = {
    { "QCheckBox", "checked" },
    { "QRadioButton", "checked" },
    { "QLineEdit", "text" },
    { "QSpinBox", "value" },
    { "QDoubleSpinBox", "value" },
    { "QComboBox", "currentIndex" },
    { "QFontComboBox", "currentText" },
    { "QScrollBar", "value" },
    { "QSlider", "value" },
    { "PaletteEditor", "value" }
};

QMap<QString, std::function<QVariant(QObject *)>> Setting::classFetcher([]() {
    QMap<QString, std::function<QVariant(QObject*)>> fetchers;
    for (auto it = classToProperty.begin(); it != classToProperty.end(); it++) {
        const char *property = it.value();
        fetchers.insert(it.key(), [property](QObject *w) {
            return w->property(property);
        });
    }
    fetchers.insert("QListWidget", [](QObject *w) {
        QListWidget* lw = static_cast<QListWidget*>(w);
        if (!lw)
            return QVariant();
        int count = lw->count();
        QStringList items;
        for (int i = 0; i < count; i++)
            items.append(lw->item(i)->text());
        return QVariant(items);
    });
    return fetchers;
}());

QMap<QString, std::function<void (QObject *, const QVariant &)> > Setting::classSetter([]() {
    QMap<QString, std::function<void(QObject*, const QVariant &)>> setters;
    for (auto it = classToProperty.begin(); it != classToProperty.end(); it++) {
        const char *property = it.value();
        setters.insert(it.key(), [property](QObject *w, const QVariant &v) {
            w->setProperty(property, v);
        });
    }
    setters.insert("QListWidget", [](QObject *w, const QVariant &v) {
        QListWidget* l = static_cast<QListWidget*>(w);
        QStringList list = v.toStringList();
        if (!l)
            return;
        l->clear();
        for (auto &listItem : list)
            l->addItem(listItem);
    });
    return setters;
}());



static QStringList internalLogos = {
    ":/not-a-real-resource.png",
    ":/images/logo/film-color.svg",
    ":/images/logo/film-gray.svg",
    ":/images/logo/triangle-circle.svg",
    ":/images/logo/mpv-vlc.svg"
};



void Setting::sendToControl()
{
    if (!widget) {
        qDebug() << "[Settings] attempted to send data to null widget!";
        return;
    }
    classSetter[widget->metaObject()->className()](widget, value);
}

void Setting::fetchFromControl()
{
    if (!widget) {
        qDebug() << "[Settings] attempted to get data from null widget!";
        return;
    }
    value = classFetcher[widget->metaObject()->className()](widget);
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

    actionEditor = new QActionEditor(this);
    ui->keysHost->addWidget(actionEditor);
    connect(actionEditor, &QActionEditor::mouseWindowedMap,
            this, &SettingsWindow::mouseWindowedMap);
    connect(actionEditor, &QActionEditor::mouseFullscreenMap,
            this, &SettingsWindow::mouseFullscreenMap);

    logoWidget = new LogoWidget(this);
    ui->logoImageHost->layout()->addWidget(logoWidget);

    setupPlatformWidgets();
    setupPaletteEditor();

    defaultSettings = generateSettingMap(this);
    acceptedSettings = defaultSettings;
    generateVideoPresets();

    ui->pageStack->setCurrentIndex(0);
    ui->videoTabs->setCurrentIndex(0);
    ui->scalingTabs->setCurrentIndex(0);
    ui->audioTabs->setCurrentIndex(0);
    ui->hwdecTabs->setCurrentIndex(0);

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

    int pageTreeWidth = ui->pageTree->fontMetrics().width(tr("MMMMMMMMMMMMM"));
    if (Platform::isWindows)
        pageTreeWidth *= 1.3;
    ui->pageTree->setMaximumWidth(pageTreeWidth);

    setupColorPickers();
    setupUnimplementedWidgets();
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}

void SettingsWindow::setupPlatformWidgets()
{
    // Detect a tiling desktop, and disable autozoom for the default.
    // Note that this only changes the default; if autozoom is already enabled
    // in the user's config, the application may still try to use an
    // autozooming in a tiling context.  And, in fact, it may still do so if
    // autozoom is enabled after-the-fact.
    if (Platform::tilingDesktopActive()) {
        ui->playbackAutoZoom->setChecked(false);
    }
    if (!Platform::tiledDesktopsExist()) {
        ui->playbackAutozoomWarn->setVisible(false);
    }

    if (Platform::isMac) {
        ui->ccGammaAutodetect->setEnabled(false);
    } else {
        ui->ccGammaAutodetect->setChecked(true);
    }

    ui->ipcMpris->setVisible(Platform::isUnix);
    ui->hwdecBackendVaapi->setEnabled(Platform::isUnix);
    ui->hwdecBackendVdpau->setEnabled(Platform::isUnix);
    ui->hwdecBackendDxva2->setEnabled(Platform::isWindows);
    ui->hwdecBackendD3d11va->setEnabled(Platform::isWindows);
    ui->hwdecBackendRaspberryPi->setEnabled(Platform::isUnix);
    ui->hwdecBackendVideoToolbox->setEnabled(Platform::isMac);
}

void SettingsWindow::setupPaletteEditor()
{
    paletteEditor = new PaletteEditor(this);
    paletteEditor->setObjectName("interfaceWidgetCustomPalette");
    ui->interfaceWidgetCustomHost->layout()->addWidget(paletteEditor);
}

void SettingsWindow::setupColorPickers()
{
    struct ValuePick { QLineEdit *value; QPushButton *pick; };
    QVector<ValuePick> colors {
        { ui->subsColorValue, ui->subsColorPick },
        { ui->subsBorderColorValue, ui->subsBorderColorPick },
        { ui->subsShadowColorValue, ui->subsShadowColorPick }
    };
    for (const ValuePick c : colors) {
        connect(c.pick, &QPushButton::clicked,
                this, [this,c]() { colorPick_clicked(c.value); });
        connect(c.value, &QLineEdit::textChanged,
                this, [this,c]() { colorPick_changed(c.value, c.pick); });
    }
}

void SettingsWindow::setupUnimplementedWidgets()
{
    ui->playerOpenBox->setEnabled(false);
    ui->playerTrayIcon->setEnabled(false);
    ui->playerOSD->setEnabled(false);
    ui->playerLimitProportions->setEnabled(false);
    ui->playerDisableOpenDisc->setEnabled(false);
    ui->playerTitleBox->setEnabled(false);
    ui->playerKeepHistory->setEnabled(false);
    ui->playerRememberLastPlaylist->setEnabled(false);
    ui->playerRememberPanScanZoom->setEnabled(false);

    ui->formatPage->setEnabled(false);

    ui->playbackBalance->setEnabled(false);
    ui->playbackTracksBox->setEnabled(false);
    ui->playbackAutoloadBox->setEnabled(false);

    ui->shadersWikiTab->setEnabled(false);
    ui->shadersPresetsBox->setEnabled(false);

    ui->fullscreenMonitorBox->setEnabled(false);
    ui->xrandrBox->setEnabled(false);

    ui->subtitlePlacementBox->setEnabled(false);
    ui->subtitlesFixTiming->setEnabled(false);
    ui->subtitlesClearOnSeek->setEnabled(false);
    ui->subtitlesAssOverride->setEnabled(false);

    ui->subtitlesDatabaseBox->setEnabled(false);

    ui->encodeBox->setEnabled(false);

    ui->tweaksShowChapterMarks->setEnabled(false);
    ui->tweaksTimeTooltipLocation->setEnabled(false);
    ui->tweaksOsdFont->setEnabled(false);
    ui->tweaksOsdSize->setEnabled(false);

    ui->miscColorBox->setEnabled(false);
    ui->miscExportKeys->setEnabled(false);
    ui->miscExportSettings->setEnabled(false);

}

void SettingsWindow::updateAcceptedSettings() {
    acceptedSettings = generateSettingMap(this);
    acceptedKeyMap = actionEditor->toVMap();
}

SettingMap SettingsWindow::generateSettingMap(QWidget *root)
{
    SettingMap settingMap;

    // The idea here is to discover all the widgets in the ui and only inspect
    // the widgets which we desire to know about.
    QObjectList toParse;
    toParse.append(root);
    while (!toParse.empty()) {
        QObject *item = toParse.takeFirst();
        if (Setting::classFetcher.contains(item->metaObject()->className())
            && !item->objectName().isEmpty()
            && item->objectName() != "qt_spinbox_lineedit") {
            QString name = item->objectName();
            QString className = item->metaObject()->className();
            QVariant value = Setting::classFetcher[className](item);
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

void SettingsWindow::generateVideoPresets()
{
    SettingMap videoWidgets;

    videoWidgets.unite(generateSettingMap(ui->generalTab));
    videoWidgets.unite(generateSettingMap(ui->ditherTab));
    videoWidgets.unite(generateSettingMap(ui->scalingTab));
    videoWidgets.unite(generateSettingMap(ui->debandTab));
    videoWidgets.unite(generateSettingMap(ui->syncMode));
    videoWidgets.remove(ui->videoPreset->objectName());

    auto setWidget = [&videoWidgets](QWidget *x, auto y) {
        videoWidgets[x->objectName()].value = QVariant(y);
    };

    // plain
    // nothing to see here, use the defaults
    videoPresets.append(videoWidgets);

    // low
    setWidget(ui->videoFramebuffer, 3);  // fbo=rgb16
    setWidget(ui->scaleScaler, 18); //scale=catmull_rom
    setWidget(ui->dscaleScaler, 20); //dscale=mitchell
    videoPresets.append(videoWidgets);

    // high
    setWidget(ui->scaleScaler, 4);  // scale=spine36
    setWidget(ui->scalingCorrectDownscaling, true); //correct-downscaling
    setWidget(ui->scalingSigmoidizedUpscaling, true); //sigmoidized-upscaling
    setWidget(ui->scalingInLinearLight, true); // linear-scaling
    setWidget(ui->ditherDithering, true); // dither=fruit (i.e. yes)
    videoPresets.append(videoWidgets);

    // highest
    setWidget(ui->videoUseAlpha, true);
    setWidget(ui->scaleScaler, 13); // scale=ewa_lanczossharp
    setWidget(ui->dscaleScaler, 15);  // dscale=ewa_lanczossoft
    setWidget(ui->cscaleScaler, 13); // cscale=ewa_lanczossharp
    setWidget(ui->debandEnabled, true); //deband=yes
    videoPresets.append(videoWidgets);

    // placebo
    setWidget(ui->videoFramebuffer, 5); //fbo=rgba32f
    setWidget(ui->ditherTemporal, true); //temporal-dither=yes
    setWidget(ui->scalingTemporalInterpolation, true); //interpolation
    setWidget(ui->scalingBlendSubtitles, true); //blend-subtitles
    setWidget(ui->tscaleScaler, 11); // tscale=mitchell
    setWidget(ui->syncMode, 1); //video-sync=display-resample
    videoPresets.append(videoWidgets);
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

QString SettingsWindow::channelSwitcher()
{
    //FIXME: stub
    return "2.0";
}

void SettingsWindow::takeActions(const QList<QAction *> actions)
{
    QList<Command> commandList;
    for (QAction *a : actions) {
        Command c;
        c.fromAction(a);
        commandList.append(c);
    }
    actionEditor->setCommands(commandList);
    defaultKeyMap = actionEditor->toVMap();
}

void SettingsWindow::takeSettings(QVariantMap payload)
{
    acceptedSettings.fromVMap(payload);
    for (Setting &s : acceptedSettings) {
        s.sendToControl();
    }
    updateLogoWidget();
}

void SettingsWindow::takeKeyMap(const QVariantMap &payload)
{
    actionEditor->fromVMap(payload);
    actionEditor->updateActions();
    acceptedKeyMap = actionEditor->toVMap();
}

void SettingsWindow::setMouseMapDefaults(const QVariantMap &payload)
{
    actionEditor->fromVMap(payload);
    defaultKeyMap = actionEditor->toVMap();
}

void SettingsWindow::setAudioDevices(const QList<AudioDevice> &devices)
{
    audioDevices = devices;
    ui->audioDevice->clear();
    for (const AudioDevice &device : audioDevices)
        ui->audioDevice->addItem(device.displayString());
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

#define WIDGET_LOOKUP2(option, widget, dflt) \
    (WIDGET_LOOKUP(option).toBool() ? WIDGET_LOOKUP(widget) : QVariant(dflt))

#define WIDGET_LOOKUP2_TEXT(option, widget, dflt) \
    (WIDGET_LOOKUP(option).toBool() ? WIDGET_TO_TEXT(widget) : QVariant(dflt))

void SettingsWindow::sendSignals()
{
    emit trayIcon(WIDGET_LOOKUP(ui->playerTrayIcon).toBool());
    emit showOsd(WIDGET_LOOKUP(ui->playerOSD).toBool());
    emit limitProportions(WIDGET_LOOKUP(ui->playerDisableOpenDisc).toBool());
    emit disableOpenDiscMenu(WIDGET_LOOKUP(ui->playerDisableOpenDisc).toBool());
    emit inhibitScreensaver(WIDGET_LOOKUP(ui->playerDisableScreensaver).toBool());
    emit titleBarFormat(WIDGET_LOOKUP(ui->playerTitleDisplayFullPath).toBool() ? Helpers::PrefixFullPath
                        : WIDGET_LOOKUP(ui->playerTitleFileNameOnly).toBool() ? Helpers::PrefixFileName : Helpers::NoPrefix);
    emit titleUseMediaTitle(WIDGET_LOOKUP(ui->playerTitleReplaceName).toBool());
    emit rememberHistory(WIDGET_LOOKUP(ui->playerKeepHistory).toBool());
    emit rememberSelectedPlaylist(WIDGET_LOOKUP(ui->playerRememberLastPlaylist).toBool());
    emit rememberWindowGeometry(WIDGET_LOOKUP(ui->playerRememberWindowGeometry).toBool());
    emit rememberPanNScan(WIDGET_LOOKUP(ui->playerRememberPanScanZoom).toBool());

    emit mprisIpc(WIDGET_LOOKUP(ui->ipcMpris).toBool());

    emit logoSource(selectedLogo());
    emit iconTheme(WIDGET_TO_TEXT(ui->interfaceIconsFallback),
                   WIDGET_LOOKUP(ui->interfaceIconsCustomFolder).toString());
    emit applicationPalette(WIDGET_LOOKUP(ui->interfaceWidgetCustom).toBool()
                            ? paletteEditor->variantToPalette(WIDGET_LOOKUP(paletteEditor))
                            : paletteEditor->systemPalette());
    emit videoColor(QString("#%1").arg(WIDGET_LOOKUP(ui->windowVideoValue).toString()));
    emit infoStatsColors(QString("#%1").arg(WIDGET_LOOKUP(ui->windowInfoForegroundValue).toString()),
                         QString("#%1").arg(WIDGET_LOOKUP(ui->windowInfoBackgroundValue).toString()));

    emit volume(WIDGET_LOOKUP(ui->playbackVolume).toInt());
    emit volumeStep(WIDGET_LOOKUP(ui->playbackVolumeStep).toInt());
    {
        int i = WIDGET_LOOKUP(ui->playbackSpeedStep).toInt();
        emit speedStep(i > 0 ? 1.0 + i/100.0 : 2.0);
    }
    emit stepTimeLarge(WIDGET_LOOKUP(ui->playbackTimeStep).toInt());
    emit stepTimeSmall(WIDGET_LOOKUP(ui->playbackFineStep).toInt());

    emit playbackPlayTimes(WIDGET_LOOKUP(ui->playbackPlayAmount).toInt());
    emit playbackForever(WIDGET_LOOKUP(ui->playbackRepeatForever).toBool());
    emit option("image-display-duration", WIDGET_LOOKUP(ui->playbackLoopImages).toBool() ? QVariant("inf") : QVariant(1.0));

    emit zoomCenter(WIDGET_LOOKUP(ui->playbackAutoCenterWindow).toBool());
    double factor = WIDGET_LOOKUP(ui->playbackAutoFitFactor).toInt() / 100.0;
    if (!WIDGET_LOOKUP(ui->playbackAutoZoom).toBool())
        emit zoomPreset(-1, factor);
    else {
        int preset = WIDGET_LOOKUP(ui->playbackAutoZoomMethod).toInt();
        int count = ui->playbackAutoZoomMethod->count();
        if (preset >= count - 3)
            emit zoomPreset(preset - count - 1, factor);
        else
            emit zoomPreset(preset, factor);
    }

    emit mouseHideTimeFullscreen(WIDGET_LOOKUP(ui->playbackMouseHideFullscreen).toBool()
                                 ? WIDGET_LOOKUP(ui->playbackMouseHideFullscreenDuration).toInt()
                                 : 0);
    emit mouseHideTimeWindowed(WIDGET_LOOKUP(ui->playbackMouseHideWindowed).toBool()
                               ? WIDGET_LOOKUP(ui->playbackMouseHideWindowedDuration).toInt()
                               : 0);

    emit option("video-sync", WIDGET_TO_TEXT(ui->syncMode));
    emit option("gpu-dumb-mode", WIDGET_LOOKUP(ui->videoDumbMode));
    emit option("fbo-format", WIDGET_TO_TEXT(ui->videoFramebuffer).split('-').value(WIDGET_LOOKUP(ui->videoUseAlpha).toBool()));
    emit option("alpha", WIDGET_TO_TEXT(ui->videoAlphaMode));
    emit option("sharpen", WIDGET_LOOKUP(ui->videoSharpen).toString());

    if (WIDGET_LOOKUP(ui->ditherDithering).toBool()) {
        emit option("dither-depth", WIDGET_LOOKUP(ui->ditherDepth).toString());
        emit option("dither", WIDGET_TO_TEXT(ui->ditherType));
        emit option("dither-size-fruit", WIDGET_LOOKUP(ui->ditherFruitSize).toString());
    } else {
        emit option("dither", "no");
    }
    emit option("temporal-dither", WIDGET_LOOKUP(ui->ditherTemporal));
    emit option("temporal-dither-period", WIDGET_LOOKUP2(ui->ditherTemporal, ui->ditherTemporalPeriod, 1));
    emit option("correct-downscaling", WIDGET_LOOKUP(ui->scalingCorrectDownscaling));
    emit option("linear-scaling", WIDGET_LOOKUP(ui->scalingInLinearLight));
    emit option("interpolation", WIDGET_LOOKUP(ui->scalingTemporalInterpolation));
    emit option("blend-subtitles", WIDGET_LOOKUP(ui->scalingBlendSubtitles));
    if (WIDGET_LOOKUP(ui->scalingSigmoidizedUpscaling).toBool()) {
        emit option("sigmoid-upscaling", true);
        emit option("sigmoid-center", WIDGET_LOOKUP(ui->sigmoidizedCenter));
        emit option("sigmoid-slope", WIDGET_LOOKUP(ui->sigmoidizedSlope));
    } else {
        emit option("sigmoid-upscaling", false);
    }

    // Is this the right way to fall back to (the scaler's) defaults?
    // Bear in mind we're not setting what hasn't changed since last time.
    // Perhaps would should pass a blank QVariant or empty string instead.
    emit option("scale", WIDGET_TO_TEXT(ui->scaleScaler));
    emit option("scale-param1", WIDGET_LOOKUP2(ui->scaleParam1Set, ui->scaleParam1Value, "nan"));
    emit option("scale-param2", WIDGET_LOOKUP2(ui->scaleParam2Set, ui->scaleParam2Value, "nan"));
    emit option("scale-radius", WIDGET_LOOKUP2(ui->scaleRadiusSet, ui->scaleRadiusValue, 0.0));
    emit option("scale-antiring", WIDGET_LOOKUP2(ui->scaleAntiRingSet, ui->scaleAntiRingValue, 0.0));
    emit option("scale-blur",   WIDGET_LOOKUP2(ui->scaleBlurSet,   ui->scaleBlurValue,  "nan"));
    emit option("scale-wparam", WIDGET_LOOKUP2(ui->scaleWindowParamSet, ui->scaleWindowParamValue, "nan"));
    emit option("scale-window", WIDGET_LOOKUP2_TEXT(ui->scaleWindowSet, ui->scaleWindowValue, ""));
    emit option("scale-clamp", WIDGET_LOOKUP(ui->scaleClamp));

    emit option("dscale", WIDGET_TO_TEXT(ui->dscaleScaler));
    emit option("dscale-param1", WIDGET_LOOKUP2(ui->dscaleParam1Set, ui->dscaleParam1Value, "nan"));
    emit option("dscale-param2", WIDGET_LOOKUP2(ui->dscaleParam2Set, ui->dscaleParam2Value, "nan"));
    emit option("dscale-radius", WIDGET_LOOKUP2(ui->dscaleRadiusSet, ui->dscaleRadiusValue, 0.0));
    emit option("dscale-antiring", WIDGET_LOOKUP2(ui->dscaleAntiRingSet, ui->dscaleAntiRingValue, 0.0));
    emit option("dscale-blur",   WIDGET_LOOKUP2(ui->dscaleBlurSet,   ui->dscaleBlurValue,  "nan"));
    emit option("dscale-wparam", WIDGET_LOOKUP2(ui->dscaleWindowParamSet, ui->dscaleWindowParamValue, "nan"));
    emit option("dscale-window", WIDGET_LOOKUP2_TEXT(ui->dscaleWindowSet, ui->dscaleWindowValue, ""));
    emit option("dscale-clamp", WIDGET_LOOKUP(ui->dscaleClamp));

    emit option("cscale", WIDGET_TO_TEXT(ui->cscaleScaler));
    emit option("cscale-param1", WIDGET_LOOKUP2(ui->cscaleParam1Set, ui->cscaleParam1Value, "nan"));
    emit option("cscale-param2", WIDGET_LOOKUP2(ui->cscaleParam2Set, ui->cscaleParam2Value, "nan"));
    emit option("cscale-radius", WIDGET_LOOKUP2(ui->cscaleRadiusSet, ui->cscaleRadiusValue, 0.0));
    emit option("cscale-antiring", WIDGET_LOOKUP2(ui->cscaleAntiRingSet, ui->cscaleAntiRingValue, 0.0));
    emit option("cscale-blur",   WIDGET_LOOKUP2(ui->cscaleBlurSet,   ui->cscaleBlurValue,  "nan"));
    emit option("cscale-wparam", WIDGET_LOOKUP2(ui->cscaleWindowParamSet, ui->cscaleWindowParamValue, "nan"));
    emit option("cscale-window", WIDGET_LOOKUP2_TEXT(ui->cscaleWindowSet, ui->cscaleWindowValue, ""));
    emit option("cscale-clamp", WIDGET_LOOKUP(ui->cscaleClamp));

    emit option("tscale", WIDGET_TO_TEXT(ui->tscaleScaler));
    emit option("tscale-param1", WIDGET_LOOKUP2(ui->tscaleParam1Set, ui->tscaleParam1Value, "nan"));
    emit option("tscale-param2", WIDGET_LOOKUP2(ui->tscaleParam2Set, ui->tscaleParam2Value, "nan"));
    emit option("tscale-radius", WIDGET_LOOKUP2(ui->tscaleRadiusSet, ui->tscaleRadiusValue, 0.0));
    emit option("tscale-antiring", WIDGET_LOOKUP2(ui->tscaleAntiRingSet, ui->tscaleAntiRingValue, 0.0));
    emit option("tscale-blur",   WIDGET_LOOKUP2(ui->tscaleBlurSet,   ui->tscaleBlurValue,  "nan"));
    emit option("tscale-wparam", WIDGET_LOOKUP2(ui->tscaleWindowParamSet, ui->tscaleWindowParamValue, "nan"));
    emit option("tscale-window", WIDGET_LOOKUP2_TEXT(ui->tscaleWindowSet, ui->tscaleWindowValue, ""));
    emit option("tscale-clamp", WIDGET_LOOKUP(ui->tscaleClamp));

    if (WIDGET_LOOKUP(ui->debandEnabled).toBool()) {
        emit option("deband", true);
        emit option("deband-iterations", WIDGET_LOOKUP(ui->debandIterations));
        emit option("deband-threshold", WIDGET_LOOKUP(ui->debandThreshold));
        emit option("deband-range", WIDGET_LOOKUP(ui->debandRange));
        emit option("deband-grain", WIDGET_LOOKUP(ui->debandGrain));
    } else {
        emit option("deband", false);
    }

    emit option("gamma", WIDGET_LOOKUP(ui->ccGamma));
    if (Platform::isMac)
        option("gamma-auto", WIDGET_LOOKUP(ui->ccGammaAutodetect));
    emit option("target-prim", WIDGET_TO_TEXT(ui->ccTargetPrim));
    emit option("target-trc", WIDGET_TO_TEXT(ui->ccTargetTRC));
    emit option("tone-mapping", WIDGET_TO_TEXT(ui->ccHdrMapper));
    {
        QList<QDoubleSpinBox*> boxen {ui->ccHdrClipParam,
                    ui->ccHdrMobiusParam, ui->ccHdrReinhardParam, nullptr,
                    ui->ccHdrGammaParam, ui->ccHdrLinearParam};
        QDoubleSpinBox* toneParam = boxen[WIDGET_LOOKUP(ui->ccHdrMapper).toInt()];
        emit option("tone-mapping-param", toneParam ? WIDGET_LOOKUP(toneParam) : QVariant("nan"));
    }
    emit option("tone-mapping-desaturate", WIDGET_LOOKUP(ui->ccHdrDesaturate));
    if (WIDGET_LOOKUP(ui->ccICCAutodetect).toBool()) {
        emit option("icc-profile", "");
        emit option("icc-profile-auto", true);
    } else {
        emit option("icc-profile-auto", false);
        emit option("icc-profile", WIDGET_LOOKUP(ui->ccICCLocation));
    }

    int index = WIDGET_LOOKUP(ui->audioDevice).toInt();
    emit option("audio-device", audioDevices.value(index).deviceName());
    index = WIDGET_LOOKUP(ui->audioChannels).toInt();
    emit option("audio-channels", index < 3 ? SettingMap::indexedValueToText[ui->audioChannels->objectName()][index]
                                         : channelSwitcher());
    bool flag = WIDGET_LOOKUP(ui->audioStreamSilence).toBool();
    emit option("stream-silence", flag);
    emit option("audio-wait-open", flag ? WIDGET_LOOKUP(ui->audioWaitTime).toDouble() : 0.0);
    emit option("audio-pitch-correction", WIDGET_LOOKUP(ui->audioPitchCorrection).toBool());
    emit option("audio-exclusive", WIDGET_LOOKUP(ui->audioExclusiveMode).toBool());
    emit option("audio-normalize-downmix", WIDGET_LOOKUP(ui->audioNormalizeDownmix).toBool());
    emit option("audio-spdif", WIDGET_LOOKUP(ui->audioSpdif).toBool() ? WIDGET_PLACEHOLD_LOOKUP(ui->audioSpdifCodecs) : "");
    emit option("pulse-buffer", WIDGET_LOOKUP(ui->pulseBuffer).toInt());
    emit option("pulse-latency-hacks", WIDGET_LOOKUP(ui->pulseLatency).toBool());
    emit option("alsa-resample", WIDGET_LOOKUP(ui->alsaResample).toBool());
    emit option("alsa-ignore-chmap", WIDGET_LOOKUP(ui->alsaIgnoreChannelMap).toBool());
    emit option("oss-mixer-channel", WIDGET_LOOKUP(ui->ossMixerChannel).toString());
    emit option("oss-mixer-device", WIDGET_LOOKUP(ui->ossMixerDevice).toString());
    emit option("jack-autostart", WIDGET_LOOKUP(ui->jackAutostart).toBool());
    emit option("jack-connect", WIDGET_LOOKUP(ui->jackConnect).toBool());
    emit option("jack-name", WIDGET_LOOKUP(ui->jackName).toString());
    emit option("jack-port", WIDGET_LOOKUP(ui->jackPort).toString());

    // FIXME: add icc-intent etc

    emit option("glsl-shaders", WIDGET_LOOKUP(ui->shadersActiveList).toStringList());

    if (WIDGET_LOOKUP(ui->fullscreenHideControls).toBool()) {
        Helpers::ControlHiding method = static_cast<Helpers::ControlHiding>(WIDGET_LOOKUP(ui->fullscreenShowWhen).toInt());
        int timeOut = WIDGET_LOOKUP(ui->fullscreenShowWhenDuration).toInt();
        if (method == Helpers::ShowWhenMoving && !timeOut) {
            emit hideMethod(Helpers::ShowWhenHovering);
            emit hideTime(0);
        } else {
            emit hideMethod(method);
            emit hideTime(timeOut);
        }
        emit hidePanels(WIDGET_LOOKUP(ui->fullscreenHidePanels).toBool());
    } else {
        emit hideMethod(Helpers::AlwaysShow);
        emit hidePanels(false);
    }
    emit option("framedrop", WIDGET_TO_TEXT(ui->framedroppingMode));
    emit option("vf-lavc-framedrop", WIDGET_TO_TEXT(ui->framedroppingDecoderMode));
    emit option("video-sync-adrop-size", WIDGET_LOOKUP(ui->syncAudioDropSize).toDouble());
    emit option("video-sync-max-audio-change", WIDGET_LOOKUP(ui->syncMaxAudioChange).toDouble());
    emit option("video-sync-max-video-change", WIDGET_LOOKUP(ui->syncMaxVideoChange).toDouble());
    if (WIDGET_LOOKUP(ui->hwdecEnable).toBool()) {
        QString backend = "auto-copy";
        if (WIDGET_LOOKUP(ui->hwdecBackendVaapi).toBool())
            backend = "vaapi-copy";
        if (WIDGET_LOOKUP(ui->hwdecBackendVdpau).toBool())
            backend = "vdpau-copy";
        if (WIDGET_LOOKUP(ui->hwdecBackendDxva2).toBool())
            backend = "dxva2-copy";
        if (WIDGET_LOOKUP(ui->hwdecBackendD3d11va).toBool())
            backend = "d3d11va-copy";
        if (WIDGET_LOOKUP(ui->hwdecBackendRaspberryPi).toBool())
            backend = "rpi-copy";
        if (WIDGET_LOOKUP(ui->hwdecBackendVideoToolbox).toBool())
            backend = "videotoolbox-copy";
        if (WIDGET_LOOKUP(ui->hwdecBackendCuda).toBool())
            backend = "cuda-copy";
        if (WIDGET_LOOKUP(ui->hwdecBackendCrystalHd).toBool())
            backend = "crystalhd";
        emit option("hwdec", backend);
        if (WIDGET_LOOKUP(ui->hwdecAll).toBool()) {
            emit option("hwdec-codecs", "all");
        } else {
            QStringList codecs;
            if (WIDGET_LOOKUP(ui->hwdecMJpeg).toBool()) codecs << "mjpeg";
            if (WIDGET_LOOKUP(ui->hwdecMpeg1Video).toBool()) codecs << "mpeg1video";
            if (WIDGET_LOOKUP(ui->hwdecMpeg2Video).toBool()) codecs << "mpeg2video";
            if (WIDGET_LOOKUP(ui->hwdecMpeg4).toBool()) codecs << "mpeg4";
            if (WIDGET_LOOKUP(ui->hwdecH263).toBool()) codecs << "h263";
            if (WIDGET_LOOKUP(ui->hwdecH264).toBool()) codecs << "h264";
            if (WIDGET_LOOKUP(ui->hwdecVc1).toBool()) codecs << "vc1";
            if (WIDGET_LOOKUP(ui->hwdecWmv3).toBool()) codecs << "wmv3";
            if (WIDGET_LOOKUP(ui->hwdecHevc).toBool()) codecs << "hevc";
            if (WIDGET_LOOKUP(ui->hwdecVp9).toBool()) codecs << "vp9";
            emit option("hwdec-codecs", codecs.join(','));
        }
    } else {
        emit option("hwdec", "no");
        emit option("hwdec-codecs", "");
    }

    emit playlistFormat(WIDGET_PLACEHOLD_LOOKUP(ui->playlistFormat));
    emit option("sub-gray", WIDGET_LOOKUP(ui->subtitlesForceGrayscale).toBool());

    emit option("sub-font", WIDGET_LOOKUP(ui->fontComboBox).toString());
    emit option("sub-bold", WIDGET_LOOKUP(ui->fontBold).toBool());
    emit option("sub-italic", WIDGET_LOOKUP(ui->fontItalic).toBool());
    emit option("sub-font-size", WIDGET_LOOKUP(ui->fontSize).toInt());
    emit option("sub-border-size", WIDGET_LOOKUP(ui->borderSize).toInt());
    emit option("sub-shadow-offset", WIDGET_LOOKUP(ui->borderShadowOffset).toInt());
    {
        struct AlignData { QRadioButton *btn; int x; int y; };
        QVector<AlignData> alignments {
            { ui->subsAlignmentTopLeft, -1, -1 },
            { ui->subsAlignmentTop, 0, -1 },
            { ui->subsAlignmentTopRight, 1, -1 },
            { ui->subsAlignmentLeft, -1, 0 },
            { ui->subsAlignmentCenter, 0, 0 },
            { ui->subsAlignmentRight, 1, 0 },
            { ui->subsAlignmentBottomLeft, -1, 1 },
            { ui->subsAlignmentBottom, 0, 1 },
            { ui->subsAlignmentBottomRight, 1, 1 }
        };
        static QMap<int, const char *> wx {
            { -1, "left" },
            { 0, "center" },
            { 1, "right" }
        };
        static QMap<int, const char *> wy {
            { -1, "top" },
            { 0, "center" },
            { 1, "bottom" }
        };
        for (const AlignData &a : alignments) {
            if (a.btn->isChecked()) {
                emit option("sub-align-x", wx[a.x]);
                emit option("sub-align-y", wy[a.y]);
                break;
            }
        }
    }
    emit option("sub-margin-x", WIDGET_LOOKUP(ui->subsMarginX).toInt());
    emit option("sub-margin-y", WIDGET_LOOKUP(ui->subsMarginY).toInt());
    emit option("sub-use-margins", !WIDGET_LOOKUP(ui->subsRelativeToVideoFrame).toBool());
    emit option("sub-color", QString("#%1").arg(WIDGET_LOOKUP(ui->subsColorValue).toString()));
    emit option("sub-border-color", QString("#%1").arg(WIDGET_LOOKUP(ui->subsBorderColorValue).toString()));
    emit option("sub-shadow-color", QString("#%1").arg(WIDGET_LOOKUP(ui->subsShadowColorValue).toString()));

    emit option("sub-forced-only", WIDGET_LOOKUP(ui->subtitlesPreferForced));
    bool subsAutoload = WIDGET_LOOKUP(ui->subtitlesAutoloadExternal).toBool();
    emit option("sub-auto", subsAutoload ? WIDGET_TO_TEXT(ui->subtitlesAutoloadMatch) : QString("no"));
    emit option("sub-file-paths", WIDGET_LOOKUP(ui->subtitlesAutoloadPath).toString().split(';'));

    emit screenshotDirectory(
                WIDGET_LOOKUP(ui->screenshotDirectorySet).toBool() ?
                QFileInfo(WIDGET_PLACEHOLD_LOOKUP(ui->screenshotDirectoryValue)).absoluteFilePath() : QString());

    emit encodeDirectory(
                WIDGET_LOOKUP(ui->encodeDirectorySet).toBool() ?
                QFileInfo(WIDGET_PLACEHOLD_LOOKUP(ui->encodeDirectoryValue)).absoluteFilePath() : QString());
    emit screenshotTemplate(WIDGET_PLACEHOLD_LOOKUP(ui->screenshotTemplate));
    emit encodeTemplate(WIDGET_PLACEHOLD_LOOKUP(ui->encodeTemplate));
    emit option("screenshot-high-bit-depth", WIDGET_LOOKUP(ui->screenshotFormatHighBitDepth));
    emit screenshotFormat(WIDGET_TO_TEXT(ui->screenshotFormat));
    emit option("screenshot-format", WIDGET_TO_TEXT(ui->screenshotFormat));
    emit option("screenshot-jpeg-quality", WIDGET_LOOKUP(ui->jpgQuality).toInt());
    emit option("screenshot-jpeg-smooth", WIDGET_LOOKUP(ui->jpgSmooth).toInt());
    emit option("screenshot-jpeg-source-chroma", WIDGET_LOOKUP(ui->jpgSourceChroma).toBool());
    emit option("screenshot-png-compression", WIDGET_LOOKUP(ui->pngCompression).toInt());
    emit option("screenshot-png-filter", WIDGET_LOOKUP(ui->pngFilter).toInt());
    emit option("screenshot-tag-colorspace", WIDGET_LOOKUP(ui->pngColorspace).toBool());
    emit clientDebuggingMessages(WIDGET_LOOKUP(ui->debugClient).toBool());
    emit option("hr-seek", WIDGET_LOOKUP(ui->tweaksFastSeek).toBool() ? "absolute" : "yes");
    emit option("hr-seek-framedrop", WIDGET_LOOKUP(ui->tweaksSeekFramedrop).toBool());
    emit fallbackToFolder(WIDGET_LOOKUP(ui->tweaksOpenNextFile).toBool());
    emit timeTooltip(WIDGET_LOOKUP(ui->tweaksTimeTooltip).toBool(),
                     WIDGET_LOOKUP(ui->tweaksTimeTooltipLocation).toInt() == 0);
    emit mpvLogLevel(WIDGET_TO_TEXT(ui->debugMpv));
}

void SettingsWindow::sendAcceptedSettings()
{
    emit settingsData(acceptedSettings.toVMap());
    emit keyMapData(acceptedKeyMap);
}

void SettingsWindow::setScreensaverDisablingEnabled(bool enabled)
{
    ui->playerDisableScreensaver->setEnabled(enabled);
}

void SettingsWindow::setServerName(const QString &name)
{
    ui->ipcNotice->setText(ui->ipcNotice->text().arg(name));
}

void SettingsWindow::setFreestanding(bool freestanding)
{
    bool yes = !freestanding;
    ui->ipcNotice->setVisible(yes);
    ui->ipcMpris->setVisible(yes);
    ui->playerKeepHistory->setVisible(yes);
    ui->playerRememberLastPlaylist->setVisible(yes);
    ui->playerRememberWindowGeometry->setVisible(yes);
    ui->playerRememberPanScanZoom->setVisible(yes);
    ui->playerHistoryBox->setEnabled(yes);
}

void SettingsWindow::setVolume(int level)
{
    WIDGET_LOOKUP(ui->playbackVolume).setValue(level);
    ui->playbackVolume->setValue(level);

    emit settingsData(acceptedSettings.toVMap());
}

void SettingsWindow::setZoomPreset(int which)
{
    bool autoZoom = which != -1;
    int zoomMethod = which >= 0 ? which :
                     which == -1 ? 1
                                 : which + ui->playbackAutoZoomMethod->count() + 1;

    WIDGET_LOOKUP(ui->playbackAutoZoom).setValue(autoZoom);
    WIDGET_LOOKUP(ui->playbackAutoZoomMethod).setValue(zoomMethod);
    ui->playbackAutoZoom->setChecked(autoZoom);
    ui->playbackAutoZoomMethod->setCurrentIndex(zoomMethod);

    emit settingsData(acceptedSettings.toVMap());
}

void SettingsWindow::colorPick_clicked(QLineEdit *colorValue)
{
    QColor initial = QString("#%1").arg(colorValue->text());
    QColor selected = QColorDialog::getColor(initial, this);
    if (!selected.isValid())
        return;
    QString asText = selected.name().mid(1).toUpper();
    colorValue->setText(asText);
    colorValue->setFocus();
}

void SettingsWindow::colorPick_changed(QLineEdit *colorValue, QPushButton *colorPick)
{
    colorPick->setStyleSheet(QString("background: #%1").arg(colorValue->text()));
}

void SettingsWindow::on_pageTree_itemSelectionChanged()
{
    QModelIndex modelIndex = ui->pageTree->currentIndex();
    if (!modelIndex.isValid())
        return;

    static int parentIndex[] = { 0, 5, 12, 15, 17, 18 };
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
        sendAcceptedSettings();
        actionEditor->updateActions(); // FIXME: multiwindow
        sendSignals();
    }
    if (buttonRole == QDialogButtonBox::AcceptRole ||
            buttonRole == QDialogButtonBox::RejectRole)
        close();
}

void SettingsWindow::on_ccHdrMapper_currentIndexChanged(int index)
{
    ui->ccHdrStack->setCurrentIndex(index);
}

void SettingsWindow::on_videoDumbMode_toggled(bool checked)
{
    ui->videoTabs->setEnabled(!checked);
}

void SettingsWindow::on_logoExternalBrowse_clicked()
{
    QString file = WIDGET_LOOKUP(ui->logoExternalLocation).toString();
    file = QFileDialog::getOpenFileName(this, tr("Open Logo Image"), file);
    if (file.isEmpty())
        return;

    ui->logoExternalLocation->setText(file);
    if (ui->logoExternal->isChecked())
        updateLogoWidget();
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

void SettingsWindow::on_videoPresetLoad_clicked()
{
    for (Setting &s : videoPresets[ui->videoPreset->currentIndex()])
        s.sendToControl();
}

void SettingsWindow::on_screenshotFormat_currentIndexChanged(int index)
{
    ui->screenshotFormatStack->setCurrentIndex(index);
}

void SettingsWindow::on_jpgQuality_valueChanged(int value)
{
    ui->jpgQualityValue->setText(QString("%1").arg(value, 3, 10, QChar('0')));
}

void SettingsWindow::on_jpgSmooth_valueChanged(int value)
{
    ui->jpgSmoothValue->setText(QString("%1").arg(value, 3, 10, QChar('0')));
}

void SettingsWindow::on_pngCompression_valueChanged(int value)
{
    ui->pngCompressionValue->setText(QString::number(value));
}

void SettingsWindow::on_pngFilter_valueChanged(int value)
{
    ui->pngFilterValue->setText(QString::number(value));
}

void SettingsWindow::on_keysReset_clicked()
{
    actionEditor->fromVMap(defaultKeyMap);
    actionEditor->updateActions();
}

void SettingsWindow::on_shadersAddFile_clicked()
{
    QStringList files = QFileDialog::getOpenFileNames(this);
    if (files.isEmpty())
        return;

    ui->shadersFileList->addItems(files);
}

void SettingsWindow::on_shadersRemoveFile_clicked()
{
    qDeleteAll(ui->shadersFileList->selectedItems());
}

void SettingsWindow::on_shadersAddToShaders_clicked()
{
    QList<QListWidgetItem *> items = ui->shadersFileList->selectedItems();
    QStringList files;
    for (auto item : items)
        files.append(item->text());
    ui->shadersActiveList->addItems(files);
}

void SettingsWindow::on_shadersActiveRemove_clicked()
{
    qDeleteAll(ui->shadersActiveList->selectedItems());
}


void SettingsWindow::on_shadersWikiAdd_clicked()
{

}

void SettingsWindow::on_shadersWikiSync_clicked()
{

}

void SettingsWindow::on_fullscreenHideControls_toggled(bool checked)
{
    ui->fullscreenShowWhen->setEnabled(checked);
    ui->fullscreenShowWhenDuration->setEnabled(checked);
    ui->fullscreenHidePanels->setEnabled(checked);
}

void SettingsWindow::on_playbackAutoZoom_toggled(bool checked)
{
    ui->playbackAutoZoomMethod->setEnabled(checked);
    ui->playbackAutoFitFactorLabel->setEnabled(checked);
    ui->playbackAutoFitFactor->setEnabled(checked);
    ui->playbackAutoCenterWindow->setEnabled(checked);
    ui->playbackAutozoomWarn->setEnabled(checked);
}

void SettingsWindow::on_playbackMouseHideFullscreen_toggled(bool checked)
{
    ui->playbackMouseHideFullscreenDuration->setEnabled(checked);
}

void SettingsWindow::on_playbackMouseHideWindowed_toggled(bool checked)
{
    ui->playbackMouseHideWindowedDuration->setEnabled(checked);
}

void SettingsWindow::on_miscResetSettings_clicked()
{
    for (Setting &s : defaultSettings) {
        s.sendToControl();
    }
    updateLogoWidget();
}

void SettingsWindow::on_windowVideoValue_textChanged(const QString &arg1)
{
    logoWidget->setLogoBackground(QString("#%1").arg(ui->windowVideoValue->text()));
}

void SettingsWindow::on_subtitlesAutoloadReset_clicked()
{
    ui->subtitlesAutoloadPath->setText(".;./subtitles;./subs");
}
