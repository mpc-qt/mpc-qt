#include <QDebug>
#include <QStandardPaths>
#include <QFileInfo>
#include <QFileDialog>
#include <QProcess>
#include <QProcessEnvironment>
#include "settingswindow.h"
#include "ui_settingswindow.h"
#include "qactioneditor.h"

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
                     "gamma2.2", "gamma2.8", "prophoto", "st2084"}},
    {"ccHdrMapper", {"clip", "reinhard", "hable", "gamma", "linear"}},
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
    { "QSlider", "value" }
};

QMap<QString, std::function<QVariant(QObject *)>> Setting::classFetcher([]() {
    QMap<QString, std::function<QVariant(QObject*)>> fetchers;
    for (const QString &i : Setting::classToProperty.keys()) {
        const char *property = Setting::classToProperty[i];
        fetchers.insert(i, [property](QObject *w) {
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
    for (const QString &i : Setting::classToProperty.keys()) {
        const char *property = Setting::classToProperty[i];
        setters.insert(i, [property](QObject *w, const QVariant &v) {
            w->setProperty(property, v);
        });
    }
    setters.insert("QListWidget", [](QObject *w, const QVariant &v) {
        QListWidget* l = static_cast<QListWidget*>(w);
        QStringList list = v.toStringList();
        if (!l)
            return;
        l->clear();
        for (auto listItem : list)
            l->addItem(listItem);
    });
    return setters;
}());



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

    defaultSettings = generateSettingMap(this);
    acceptedSettings = defaultSettings;
    generateVideoPresets();

    ui->pageStack->setCurrentIndex(0);
    ui->videoTabs->setCurrentIndex(0);
    ui->scalingTabs->setCurrentIndex(0);
    ui->audioTabs->setCurrentIndex(0);

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    // Detect a tiling desktop, and disable autozoom for the default.
    // Note that this only changes the default; if autozoom is already enabled
    // in the user's config, the application may still try to use an
    // autozooming in a tiling context.  And, in fact, it may still do so if
    // autozoom is enabled after-the-fact.
    auto isTilingDesktop =[]() {
        QProcessEnvironment env;
        QStringList tilers({ "awesome", "bspwm", "dwm", "i3", "larswm", "ion",
            "qtile", "ratpoison", "stumpwm", "wmii", "xmonad"});
        QString desktop = env.value("XDG_DESKTOP_SESSION");
        if (tilers.contains(desktop))
            return true;
        desktop = env.value("XDG_DATA_DIRS");
        for (QString wm : tilers)
            if (desktop.contains(wm))
                return true;
        for (QString wm: tilers) {
            QProcess process;
            process.start("pgrep", QStringList({wm}));
            process.waitForFinished();
            if (!process.readAllStandardOutput().isEmpty())
                return true;
        }
        return false;
    };
    if (isTilingDesktop())
        ui->playbackAutoZoom->setChecked(false);
#else
    ui->playbackAutozoomWarn->setVisible(false);
#endif

#ifndef Q_OS_MAC
    ui->ccGammaAutodetect->setEnabled(false);
#else
    ui->ccGammaAutodetect->setChecked(true);
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    ui->ipcMpris->setVisible(false);
#endif

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
    ui->pageTree->setMaximumWidth(pageTreeWidth);
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
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
        if (Setting::classFetcher.keys().contains(item->metaObject()->className())
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
    emit titleBarFormat(WIDGET_LOOKUP(ui->playerTitleDisplayFullPath).toBool() ? Helpers::PrefixFullPath
                        : WIDGET_LOOKUP(ui->playerTitleFileNameOnly).toBool() ? Helpers::PrefixFileName : Helpers::NoPrefix);
    emit titleUseMediaTitle(WIDGET_LOOKUP(ui->playerTitleReplaceName).toBool());
    emit rememberHistory(WIDGET_LOOKUP(ui->playerKeepHistory).toBool());
    emit rememberSelectedPlaylist(WIDGET_LOOKUP(ui->playerRememberLastPlaylist).toBool());
    emit rememberWindowGeometry(WIDGET_LOOKUP(ui->playerRememberWindowGeometry).toBool());
    emit rememberPanNScan(WIDGET_LOOKUP(ui->playerRememberPanScanZoom).toBool());

    emit logoSource(selectedLogo());

    emit volume(WIDGET_LOOKUP(ui->playbackVolume).toInt());

    emit playbackPlayTimes(WIDGET_LOOKUP(ui->playbackRepeatForever).toBool() ?
                           0 : WIDGET_LOOKUP(ui->playbackPlayAmount).toInt());
    emit playbackLoopImages(WIDGET_LOOKUP(ui->playbackLoopImages).toBool());

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

    displaySyncMode(WIDGET_TO_TEXT(ui->syncMode));
    voOption("opengl-dumb-mode", WIDGET_LOOKUP(ui->videoDumbMode));
    voOption("opengl-fbo-format", WIDGET_TO_TEXT(ui->videoFramebuffer).split('-').value(WIDGET_LOOKUP(ui->videoUseAlpha).toBool()));
    voOption("alpha", WIDGET_TO_TEXT(ui->videoAlphaMode));
    voOption("sharpen", WIDGET_LOOKUP(ui->videoSharpen).toString());

    if (WIDGET_LOOKUP(ui->ditherDithering).toBool()) {
        voOption("dither-depth", WIDGET_LOOKUP(ui->ditherDepth).toString());
        voOption("dither", WIDGET_TO_TEXT(ui->ditherType));
        voOption("dither-size-fruit", WIDGET_LOOKUP(ui->ditherFruitSize).toString());
    } else {
        voOption("dither", "no");
    }
    voOption("temporal-dither", WIDGET_LOOKUP(ui->ditherTemporal));
    voOption("temporal-dither-period", WIDGET_LOOKUP2(ui->ditherTemporal, ui->ditherTemporalPeriod, 1));
    voOption("correct-downscaling", WIDGET_LOOKUP(ui->scalingCorrectDownscaling));
    voOption("linear-scaling", WIDGET_LOOKUP(ui->scalingInLinearLight));
    voOption("interpolation", WIDGET_LOOKUP(ui->scalingTemporalInterpolation));
    voOption("blend-subtitles", WIDGET_LOOKUP(ui->scalingBlendSubtitles));
    if (WIDGET_LOOKUP(ui->scalingSigmoidizedUpscaling).toBool()) {
        voOption("sigmoid-upscaling", true);
        voOption("sigmoid-center", WIDGET_LOOKUP(ui->sigmoidizedCenter));
        voOption("sigmoid-slope", WIDGET_LOOKUP(ui->sigmoidizedSlope));
    } else {
        voOption("sigmoid-upscaling", false);
    }

    // Is this the right way to fall back to (the scaler's) defaults?
    // Bear in mind we're not setting what hasn't changed since last time.
    // Perhaps would should pass a blank QVariant or empty string instead.
    voOption("scale", WIDGET_TO_TEXT(ui->scaleScaler));
    voOption("scale-param1", WIDGET_LOOKUP2(ui->scaleParam1Set, ui->scaleParam1Value, "nan"));
    voOption("scale-param2", WIDGET_LOOKUP2(ui->scaleParam2Set, ui->scaleParam2Value, "nan"));
    voOption("scale-radius", WIDGET_LOOKUP2(ui->scaleRadiusSet, ui->scaleRadiusValue, 0.0));
    voOption("scale-antiring", WIDGET_LOOKUP2(ui->scaleAntiRingSet, ui->scaleAntiRingValue, 0.0));
    voOption("scale-blur",   WIDGET_LOOKUP2(ui->scaleBlurSet,   ui->scaleBlurValue,  "nan"));
    voOption("scale-wparam", WIDGET_LOOKUP2(ui->scaleWindowParamSet, ui->scaleWindowParamValue, "nan"));
    voOption("scale-window", WIDGET_LOOKUP2_TEXT(ui->scaleWindowSet, ui->scaleWindowValue, ""));
    voOption("scale-clamp", WIDGET_LOOKUP(ui->scaleClamp));

    voOption("dscale", WIDGET_TO_TEXT(ui->dscaleScaler));
    voOption("dscale-param1", WIDGET_LOOKUP2(ui->dscaleParam1Set, ui->dscaleParam1Value, "nan"));
    voOption("dscale-param2", WIDGET_LOOKUP2(ui->dscaleParam2Set, ui->dscaleParam2Value, "nan"));
    voOption("dscale-radius", WIDGET_LOOKUP2(ui->dscaleRadiusSet, ui->dscaleRadiusValue, 0.0));
    voOption("dscale-antiring", WIDGET_LOOKUP2(ui->dscaleAntiRingSet, ui->dscaleAntiRingValue, 0.0));
    voOption("dscale-blur",   WIDGET_LOOKUP2(ui->dscaleBlurSet,   ui->dscaleBlurValue,  "nan"));
    voOption("dscale-wparam", WIDGET_LOOKUP2(ui->dscaleWindowParamSet, ui->dscaleWindowParamValue, "nan"));
    voOption("dscale-window", WIDGET_LOOKUP2_TEXT(ui->dscaleWindowSet, ui->dscaleWindowValue, ""));
    voOption("dscale-clamp", WIDGET_LOOKUP(ui->dscaleClamp));

    voOption("cscale", WIDGET_TO_TEXT(ui->cscaleScaler));
    voOption("cscale-param1", WIDGET_LOOKUP2(ui->cscaleParam1Set, ui->cscaleParam1Value, "nan"));
    voOption("cscale-param2", WIDGET_LOOKUP2(ui->cscaleParam2Set, ui->cscaleParam2Value, "nan"));
    voOption("cscale-radius", WIDGET_LOOKUP2(ui->cscaleRadiusSet, ui->cscaleRadiusValue, 0.0));
    voOption("cscale-antiring", WIDGET_LOOKUP2(ui->cscaleAntiRingSet, ui->cscaleAntiRingValue, 0.0));
    voOption("cscale-blur",   WIDGET_LOOKUP2(ui->cscaleBlurSet,   ui->cscaleBlurValue,  "nan"));
    voOption("cscale-wparam", WIDGET_LOOKUP2(ui->cscaleWindowParamSet, ui->cscaleWindowParamValue, "nan"));
    voOption("cscale-window", WIDGET_LOOKUP2_TEXT(ui->cscaleWindowSet, ui->cscaleWindowValue, ""));
    voOption("cscale-clamp", WIDGET_LOOKUP(ui->cscaleClamp));

    voOption("tscale", WIDGET_TO_TEXT(ui->tscaleScaler));
    voOption("tscale-param1", WIDGET_LOOKUP2(ui->tscaleParam1Set, ui->tscaleParam1Value, "nan"));
    voOption("tscale-param2", WIDGET_LOOKUP2(ui->tscaleParam2Set, ui->tscaleParam2Value, "nan"));
    voOption("tscale-radius", WIDGET_LOOKUP2(ui->tscaleRadiusSet, ui->tscaleRadiusValue, 0.0));
    voOption("tscale-antiring", WIDGET_LOOKUP2(ui->tscaleAntiRingSet, ui->tscaleAntiRingValue, 0.0));
    voOption("tscale-blur",   WIDGET_LOOKUP2(ui->tscaleBlurSet,   ui->tscaleBlurValue,  "nan"));
    voOption("tscale-wparam", WIDGET_LOOKUP2(ui->tscaleWindowParamSet, ui->tscaleWindowParamValue, "nan"));
    voOption("tscale-window", WIDGET_LOOKUP2_TEXT(ui->tscaleWindowSet, ui->tscaleWindowValue, ""));
    voOption("tscale-clamp", WIDGET_LOOKUP(ui->tscaleClamp));

    if (WIDGET_LOOKUP(ui->debandEnabled).toBool()) {
        voOption("deband", true);
        voOption("deband-iterations", WIDGET_LOOKUP(ui->debandIterations));
        voOption("deband-threshold", WIDGET_LOOKUP(ui->debandThreshold));
        voOption("deband-range", WIDGET_LOOKUP(ui->debandRange));
        voOption("deband-grain", WIDGET_LOOKUP(ui->debandGrain));
    } else {
        voOption("deband", false);
    }

    voOption("gamma", WIDGET_LOOKUP(ui->ccGamma));
#ifdef Q_OS_MAC
    voOption("gamma-auto", WIDGET_LOOKUP(ui->ccGammaAutodetect));
#endif
    voOption("target-prim", WIDGET_TO_TEXT(ui->ccTargetPrim));
    voOption("target-trc", WIDGET_TO_TEXT(ui->ccTargetTRC));
    voOption("target-brightness", WIDGET_LOOKUP(ui->ccTargetBrightness));
    voOption("hdr-tone-mapping", WIDGET_TO_TEXT(ui->ccHdrMapper));
    {
        QList<QDoubleSpinBox*> boxen {NULL, ui->ccHdrReinhardParam, NULL, ui->ccHdrGammaParam, ui->ccHdrLinearParam};
        QDoubleSpinBox* toneParam = boxen[WIDGET_LOOKUP(ui->ccHdrMapper).toInt()];
        voOption("tone-mapping-param", toneParam ? WIDGET_LOOKUP(toneParam) : QVariant("nan"));
    }
    if (WIDGET_LOOKUP(ui->ccICCAutodetect).toBool()) {
        voOption("icc-profile", "");
        voOption("icc-profile-auto", true);
    } else {
        voOption("icc-profile-auto", false);
        voOption("icc-profile", WIDGET_LOOKUP(ui->ccICCLocation));
    }

    int index = WIDGET_LOOKUP(ui->audioDevice).toInt();
    aoOption("audio-device", audioDevices.value(index).deviceName());
    index = WIDGET_LOOKUP(ui->audioChannels).toInt();
    aoOption("audio-channels", index < 3 ? SettingMap::indexedValueToText[ui->audioChannels->objectName()][index]
                                         : channelSwitcher());
    bool flag = WIDGET_LOOKUP(ui->audioStreamSilence).toBool();
    aoOption("stream-silence", flag);
    aoOption("audio-wait-open", flag ? WIDGET_LOOKUP(ui->audioWaitTime).toDouble() : 0.0);
    aoOption("audio-pitch-correction", WIDGET_LOOKUP(ui->audioPitchCorrection).toBool());
    aoOption("audio-exclusive", WIDGET_LOOKUP(ui->audioExclusiveMode).toBool());
    aoOption("audio-normalize-downmix", WIDGET_LOOKUP(ui->audioNormalizeDownmix).toBool());
    aoOption("pulse-buffer", WIDGET_LOOKUP(ui->pulseBuffer).toInt());
    aoOption("pulse-latency-hacks", WIDGET_LOOKUP(ui->pulseLatency).toBool());
    aoOption("alsa-resample", WIDGET_LOOKUP(ui->alsaResample).toBool());
    aoOption("alsa-ignore-chmap", WIDGET_LOOKUP(ui->alsaIgnoreChannelMap).toBool());
    aoOption("oss-mixer-channel", WIDGET_LOOKUP(ui->ossMixerChannel).toString());
    aoOption("oss-mixer-device", WIDGET_LOOKUP(ui->ossMixerDevice).toString());
    aoOption("jack-autostart", WIDGET_LOOKUP(ui->jackAutostart).toBool());
    aoOption("jack-connect", WIDGET_LOOKUP(ui->jackConnect).toBool());
    aoOption("jack-name", WIDGET_LOOKUP(ui->jackName).toString());
    aoOption("jack-port", WIDGET_LOOKUP(ui->jackPort).toString());

    // FIXME: add icc-intent etc
    voOption("opengl-shaders", WIDGET_LOOKUP(ui->shadersActiveList).toStringList());

    if (WIDGET_LOOKUP(ui->fullscreenHideControls).toBool()) {
        Helpers::ControlHiding method = static_cast<Helpers::ControlHiding>(WIDGET_LOOKUP(ui->fullscreenShowWhen).toInt());
        int timeOut = WIDGET_LOOKUP(ui->fullscreenShowWhenDuration).toInt();
        if (method == Helpers::ShowWhenMoving && !timeOut) {
            hideMethod(Helpers::ShowWhenHovering);
            hideTime(0);
        } else {
            hideMethod(method);
            hideTime(timeOut);
        }
        hidePanels(WIDGET_LOOKUP(ui->fullscreenHidePanels).toBool());
    } else {
        hideMethod(Helpers::AlwaysShow);
        hidePanels(false);
    }
    framedropMode(WIDGET_TO_TEXT(ui->framedroppingMode));
    decoderDropMode(WIDGET_TO_TEXT(ui->framedroppingDecoderMode));
    audioDropSize(WIDGET_LOOKUP(ui->syncAudioDropSize).toDouble());
    maximumAudioChange(WIDGET_LOOKUP(ui->syncMaxAudioChange).toDouble());
    maximumVideoChange(WIDGET_LOOKUP(ui->syncMaxVideoChange).toDouble());
    playlistFormat(WIDGET_PLACEHOLD_LOOKUP(ui->playlistFormat));
    subsAreGray(WIDGET_LOOKUP(ui->subtitlesForceGrayscale).toBool());

    screenshotDirectory(
                WIDGET_LOOKUP(ui->screenshotDirectorySet).toBool() ?
                QFileInfo(WIDGET_PLACEHOLD_LOOKUP(ui->screenshotDirectoryValue)).absoluteFilePath() : QString());

    encodeDirectory(
                WIDGET_LOOKUP(ui->encodeDirectorySet).toBool() ?
                QFileInfo(WIDGET_PLACEHOLD_LOOKUP(ui->encodeDirectoryValue)).absoluteFilePath() : QString());
    screenshotTemplate(WIDGET_PLACEHOLD_LOOKUP(ui->screenshotTemplate));
    encodeTemplate(WIDGET_PLACEHOLD_LOOKUP(ui->encodeTemplate));
    screenshotFormat(WIDGET_TO_TEXT(ui->screenshotFormat));
    screenshotJpegQuality(WIDGET_LOOKUP(ui->jpgQuality).toInt());
    screenshotJpegSmooth(WIDGET_LOOKUP(ui->jpgSmooth).toInt());
    screenshotJpegSourceChroma(WIDGET_LOOKUP(ui->jpgSourceChroma).toBool());
    screenshotPngCompression(WIDGET_LOOKUP(ui->pngCompression).toInt());
    screenshotPngFilter(WIDGET_LOOKUP(ui->pngFilter).toInt());
    screenshotPngColorspace(WIDGET_LOOKUP(ui->pngColorspace).toBool());
    clientDebuggingMessages(WIDGET_LOOKUP(ui->debugClient).toBool());
    timeTooltip(WIDGET_LOOKUP(ui->tweaksTimeTooltip).toBool(),
                WIDGET_LOOKUP(ui->tweaksTimeTooltipLocation).toInt() == 0);
    mpvLogLevel(WIDGET_TO_TEXT(ui->debugMpv));
}

void SettingsWindow::setServerName(const QString &name)
{
    ui->ipcNotice->setText(ui->ipcNotice->text().arg(name));
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

void SettingsWindow::on_pageTree_itemSelectionChanged()
{
    QModelIndex modelIndex = ui->pageTree->currentIndex();
    if (!modelIndex.isValid())
        return;

    static int parentIndex[] = { 0, 4, 10, 13, 15, 16 };
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
        emit keyMapData(acceptedKeyMap);
        actionEditor->updateActions();
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
