#include <QDebug>
#include <QStandardPaths>
#include <QFileInfo>
#include <QFileDialog>
#include <QProcess>
#include <QProcessEnvironment>
#include "settingswindow.h"
#include "ui_settingswindow.h"
#include "qactioneditor.h"

#define SCALAR_SCALARS \
    "bilinear", "bicubic_fast", "oversample", "spline16", "spline36",\
    "spline64", "sinc", "lanczos", "ginseng", "jinc", "ewa_lanczos",\
    "ewa_hanning", "ewa_ginseng", "ewa_lanczossharp", "ewa_lanczossoft",\
    "haasnsoft",  "bicubic", "bcspline", "catmull_rom", "mitchell",\
    "robidoux", "robidouxsharp", "ewa_robidoux", "ewa_robidouxsharp",\
    "box", "nearest", "triangle", "gaussian"

#define SCALAR_WINDOWS \
    "box", "triable", "bartlett", "hanning", "hamming", "quadric", "welch",\
    "kaiser", "blackman", "gaussian", "sinc", "jinc", "sphinx"

#define TIME_SCALARS \
    "oversample", "linear",  "spline16", "spline36", "spline64", "sinc", \
    "lanczos", "ginseng", "bicubic", "bcspline", "catmull_rom", "mitchell", \
    "robidoux", "robidouxsharp", "box", "nearest", "triangle", "gaussian"


QHash<QString, QStringList> SettingMap::indexedValueToText = {
    {"videoFramebuffer", {"rgb8-rgba8", "rgb10-rgb10_a2", "rgba12-rgba12",\
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
                      "cie1931", "dvi-p3"}},
    {"ccTargetTRC", {"auto", "by.1886", "srgb", "linear", "gamma1.8",\
                     "gamma2.2", "gamma2.8", "prophoto", "st2084"}},
    {"ccHdrMapper", {"clip", "reinhard", "hable", "gamma", "linear"}},
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

    defaultSettings = generateSettingMap();
    acceptedSettings = defaultSettings;

    ui->pageStack->setCurrentIndex(0);
    ui->videoTabs->setCurrentIndex(0);
    ui->scalingTabs->setCurrentIndex(0);
    ui->audioRendererStack->setCurrentIndex(0);

#ifdef Q_OS_LINUX
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

#ifndef Q_OS_LINUX
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
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}

void SettingsWindow::updateAcceptedSettings() {
    acceptedSettings = generateSettingMap();
    acceptedKeyMap = actionEditor->toVMap();
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
        params["cscale-param1"] = WIDGET_LOOKUP(ui->cscaleParam1Value).toString();
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

    params["gamma"] = WIDGET_LOOKUP(ui->ccGamma).toString();
#ifdef Q_OS_MAC
    if (WIDGET_LOOKUP(ui->ccGammaAutodetect).toBool()) {
        params["gamma-autodetect"] = QString();
        params.remove("gamma");
    }
#endif
    params["target-prim"] = WIDGET_TO_TEXT(ui->ccTargetPrim);
    params["target-trc"] = WIDGET_TO_TEXT(ui->ccTargetTRC);
    params["target-brightness"] = WIDGET_LOOKUP(ui->ccTargetBrightness).toString();
    params["hdr-tone-mapping"] = WIDGET_TO_TEXT(ui->ccHdrMapper);
    {
        QList<QDoubleSpinBox*> boxen {NULL, ui->ccHdrReinhardParam, NULL, ui->ccHdrGammaParam, ui->ccHdrLinearParam};
        QDoubleSpinBox* toneParam = boxen[WIDGET_LOOKUP(ui->ccHdrMapper).toInt()];
        if (toneParam)
            params["tone-mapping-param"] = WIDGET_LOOKUP(toneParam).toString();
    }
    if (WIDGET_LOOKUP(ui->ccICCAutodetect).toBool())
        params["icc-profile-auto"] = QString();
    else
        params["icc-profile"] = WIDGET_LOOKUP(ui->ccICCLocation).toString();
    if (!WIDGET_LOOKUP(ui->shadersActiveList).toStringList().isEmpty()) {
        params["user-shaders"] = "[" + WIDGET_LOOKUP(ui->shadersActiveList).toStringList().join(",") + "]";
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
    screenshotJpegQuality(WIDGET_LOOKUP(ui->jpgQuality).toInt());
    screenshotJpegSmooth(WIDGET_LOOKUP(ui->jpgSmooth).toInt());
    screenshotJpegSourceChroma(WIDGET_LOOKUP(ui->jpgSourceChroma).toBool());
    screenshotPngCompression(WIDGET_LOOKUP(ui->pngCompression).toInt());
    screenshotPngFilter(WIDGET_LOOKUP(ui->pngFilter).toInt());
    screenshotPngColorspace(WIDGET_LOOKUP(ui->pngColorspace).toBool());
    clientDebuggingMessages(WIDGET_LOOKUP(ui->debugClient).toBool());
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
