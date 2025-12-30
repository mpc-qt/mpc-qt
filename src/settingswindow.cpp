#include <cmath>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QFileInfo>
#include <QFileDialog>
#include <QColorDialog>
#include <QToolTip>
#include "logger.h"
#include "platform/unify.h"
#include "settingswindow.h"
#include "ui_settingswindow.h"
#include "widgets/screencombo.h"

// No designated initializers until c++2a, so use factory method instead
struct FilterWindow {
    //QString name;
    double radius = 0.0;
    bool resizable = false;
    double params[2] = { 0.0, 0.0 };
    double blur = 0.0;
    double taper = 0.0;

    //FilterWindow() {}
    //FilterWindow(const QString &name) : name(name) {}
    //inline FilterWindow &name_(const QString &v) { name = v; return *this; }
    inline FilterWindow &radius_(double v) { radius = v; return *this; }
    inline FilterWindow &params_(double v0, double v1 = 0) { params[0] = v0; params[1] = v1; return *this; }
    inline FilterWindow &param_(double v) { return params_(v,0.0); }
    inline FilterWindow &blur_(double v) { blur = v; return *this; }
    inline FilterWindow &taper_(double v) { taper = v; return *this; }
    inline FilterWindow &resizable_() { resizable = true; return *this; }
};

static const QMap<QString,FilterWindow> filterWindows {
    { "box",        FilterWindow().radius_(1) },
    { "triangle",   FilterWindow().radius_(1) },
    { "bartlett",   FilterWindow().radius_(1) },
    { "hanning",    FilterWindow().radius_(1) },
    { "tukey",      FilterWindow().radius_(1).taper_(0.5) },
    { "hamming",    FilterWindow().radius_(1) },
    { "quadric",    FilterWindow().radius_(1.5) },
    { "welch",      FilterWindow().radius_(1) },
    { "kaiser",     FilterWindow().radius_(1).param_(6.33) },
    { "blackman",   FilterWindow().radius_(1).param_(0.16) },
    { "gaussian",   FilterWindow().radius_(2).param_(1.00) },
    { "sinc",       FilterWindow().radius_(1) },
    { "jinc",       FilterWindow().radius_(1.2196698912665045) },
    { "sphinx",     FilterWindow().radius_(1.4302966531242027) },
};

struct FilterKernel : public FilterWindow {
    QString windowName;
    FilterWindow window;
    double antiring = 0.0;
    double clamp = 1.0;
    double cutoff = 0.0;

    //FilterKernel() {}
    //FilterKernel(const QString &name) : FilterWindow(name) {}
    //inline FilterKernel &name_(const QString &v) { name = v; return *this; }
    inline FilterKernel &radius_(double v) { radius = v; return *this; }
    inline FilterKernel &param1_(double v) { params[0] = v; return *this; }
    inline FilterKernel &param2_(double v) { params[1] = v; return *this; }
    inline FilterKernel &params_(double v0, double v1 = 0) { params[0] = v0; params[1] = v1; return *this; }
    inline FilterKernel &param_(double v) { return params_(v,0.0); }
    inline FilterKernel &antiring_(double v) { antiring = v; return *this; }
    inline FilterKernel &blur_(double v) { blur = v; return *this; }
    inline FilterKernel &taper_(double v) { taper = v; return *this; }
    inline FilterKernel &resizable_() { resizable = true; return *this; }

    inline FilterKernel &window_(const QString &wname) { windowName = wname; window = filterWindows.value(wname); return *this; }
    inline FilterKernel &clamp_(double v) { clamp = v; return *this; }
    inline FilterKernel &cutoff_(double v) { cutoff = v; return *this; }
};

static const QMap<QString,FilterKernel> filterKernels {
    { "bilinear",           FilterKernel() },
    { "bicubic_fast",       FilterKernel() },
    { "oversample",         FilterKernel() },
    { "linear",             FilterKernel() },
    { "spline16",           FilterKernel().radius_(2) },
    { "spline36",           FilterKernel().radius_(3) },
    { "spline64",           FilterKernel().radius_(4) },
    { "sinc",               FilterKernel().radius_(2).resizable_() },
    { "lanczos",            FilterKernel().radius_(3).resizable_().window_("jinc") },
    { "ginseng",            FilterKernel().radius_(3).resizable_().window_("hanning") },
    { "jinc",               FilterKernel().radius_(3).resizable_()},
    { "ewa_lanczos",        FilterKernel().radius_(3).resizable_().window_("jinc")  },
    { "ewa_hanning",        FilterKernel().radius_(3).resizable_().window_("hanning")  },
    { "ewa_ginseng",        FilterKernel().radius_(3).resizable_().window_("sinc")  },
    { "ewa_lanczossharp",   FilterKernel().radius_(3.2383154841662362).resizable_().window_("jinc").blur_(0.9812505644269356) },
    { "ewa_lanczossoft",    FilterKernel().radius_(3.2383154841662362).resizable_().window_("jinc").blur_(1.015) },
    { "haasnsoft",          FilterKernel().radius_(3.2383154841662362).resizable_().window_("hanning").blur_(1.11) },
    { "bicubic",            FilterKernel().radius_(2).resizable_() },
    { "bcspline",           FilterKernel().radius_(2).resizable_().params_(0.5, 0.5) },
    { "catmull_rom",        FilterKernel().radius_(2).resizable_().params_(0.0, 0.5) },
    { "mitchell",           FilterKernel().radius_(2).resizable_().params_(1.0/3.0, 1.0/3.0) },
    { "robidoux",           FilterKernel().radius_(2).resizable_().params_(12 / (19 + 9 * M_SQRT2),
                                                                           113 / (58 + 216 * M_SQRT2)) },
    { "robidouxsharp",      FilterKernel().radius_(2).resizable_().params_(6 / (13 + 7 * M_SQRT2),
                                                                           7 / (2 + 12 * M_SQRT2)) },
    { "ewa_robidoux",       FilterKernel().radius_(2).resizable_().params_(12 / (19 + 9 * M_SQRT2),
                                                                           113 / (58 + 216 * M_SQRT2)) },
    { "ewa_robidouxsharp",  FilterKernel().radius_(2).resizable_().params_(6 / (13 + 7 * M_SQRT2),
                                                                           7 / (2 + 12 * M_SQRT2)) },
    { "box",                FilterKernel().radius_(1).resizable_() },
    { "nearest",            FilterKernel().radius_(0.5) },
    { "triangle",           FilterKernel().radius_(1).resizable_() },
    { "gaussian",           FilterKernel().radius_(2).resizable_().params_(1.0, 0.0) },
};

#define SCALER_SCALERS \
    "bilinear", "bicubic_fast", "oversample", "spline16", "spline36",\
    "spline64", "sinc", "lanczos", "ginseng", "jinc", "ewa_lanczos",\
    "ewa_hanning", "ewa_ginseng", "ewa_lanczossharp", "ewa_lanczossoft",\
    "haasnsoft",  "bicubic", "bcspline", "catmull_rom", "mitchell",\
    "robidoux", "robidouxsharp", "ewa_robidoux", "ewa_robidouxsharp",\
    "box", "nearest", "triangle", "gaussian"

#define SCALER_WINDOWS \
    "box", "triangle", "bartlett", "hanning", "hamming", "quadric", "welch",\
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
    {"ccGamutMapping", {"auto", "clip", "perceptual", "relative", "saturation",\
                        "absolute", "desaturate", "darken", "warn", "linear"}},
    {"ccTargetGamut", {"auto", "bt.601-525", "bt.601-625", "bt.709",\
                       "bt.2020", "bt.470m", "apple", "adobe", "prophoto",\
                       "cie1931", "dci-p3", "v-gamut", "s-gamut", "ebu3213",\
                       "film-c", "aces-ap0", "aces-ap1"}},
    {"ccTargetPrim", {"auto", "bt.601-525", "bt.601-625", "bt.709",\
                      "bt.2020", "bt.470m", "apple", "adobe", "prophoto",\
                      "cie1931", "dci-p3", "v-gamut", "s-gamut", "ebu3213",\
                      "film-c", "aces-ap0", "aces-ap1"}},
    {"ccTargetTrc_v2", {"auto", "bt.1886", "srgb", "linear", "gamma1.8",\
                        "gamma2.0", "gamma2.2", "gamma2.4", "gamma2.6",\
                        "gamma2.8", "prophoto", "pq", "hlg", "v-log",\
                        "s-log1", "s-log2", "st428"}},
    {"ccHdrMapper", {"clip", "mobius", "reinhard", "hable", "gamma", \
                     "linear"}},
    {"ccHdrCompute", {"auto", "yes", "no"}},
    {"ytdlpMaxHeight", {"240", "360", "480", "720", "1080", "1440",
                             "2160", "2880", "4320"}},
    {"audioChannels", {"auto-safe", "auto", "stereo"}},
    {"audioRenderer", {"pulse", "alsa", "oss", "null"}},
    {"audioAutoloadMatch", { "exact", "fuzzy", "all" }},
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
    { "QPlainTextEdit", "plainText" },
    { "QSpinBox", "value" },
    { "QDoubleSpinBox", "value" },
    { "QComboBox", "currentIndex" },
    { "QFontComboBox", "currentText" },
    { "QScrollBar", "value" },
    { "QSlider", "value" },
    { "PaletteEditor", "value" },
    { "ScreenCombo", "currentScreenName" }
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



static const QStringList internalLogos = {
    ":/not-a-real-resource.png",
    ":/images/logo/cinema-screen.svg",
    ":/images/logo/triangle-circle.svg",
    ":/images/logo/mpv-vlc.svg"
};
static constexpr char logModule[] =  "settings";



Setting &Setting::operator =(const Setting &s) = default;

void Setting::sendToControl()
{
    if (!widget) {
        Logger::log(logModule, "attempted to send data to null widget!");
        return;
    }
    classSetter[widget->metaObject()->className()](widget, value);
}

void Setting::fetchFromControl()
{
    if (!widget) {
        Logger::log(logModule, "attempted to get data from null widget!");
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
    Logger::log(logModule, "creating ui");
    ui->setupUi(this);
    Logger::log(logModule, "finished creating ui");

    actionEditor = new ActionEditor(this);
    ui->keysHost->addWidget(actionEditor);
    connect(actionEditor, &ActionEditor::mouseWindowedMap,
            this, &SettingsWindow::mouseWindowedMap);
    connect(actionEditor, &ActionEditor::mouseFullscreenMap,
            this, &SettingsWindow::mouseFullscreenMap);
    connect(ui->keysSearchField, &QLineEdit::textChanged,
            actionEditor, &ActionEditor::filterActions);

    actionEditor->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    actionEditor->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    Logger::log(logModule, "creating logo widget");
    logoWidget = new LogoWidget(this);
    ui->logoImageHost->layout()->addWidget(logoWidget);

    Logger::log(logModule, "setting up platform widgets");
    setupPlatformWidgets();
    Logger::log(logModule, "setting up palette editor");
    setupPaletteEditor();
    Logger::log(logModule, "setting up fullscreen combo");
    setupFullscreenCombo();

    Logger::log(logModule, "generating settings map");
    defaultSettings = generateSettingMap(this);
    acceptedSettings = defaultSettings;
    generateVideoPresets();
    Logger::log(logModule, "finished generating settings");

    ui->pageStack->setCurrentIndex(0);
    ui->videoTabs->setCurrentIndex(0);
    ui->scalingTabs->setCurrentIndex(0);
    ui->audioTabs->setCurrentIndex(0);
    ui->hwdecTabs->setCurrentIndex(0);

    ui->videoPresetApplied->clear();

    ui->screenshotDirectoryValue->setPlaceholderText(
                QStandardPaths::writableLocation(
                    QStandardPaths::PicturesLocation) + "/mpc_shots");
    ui->encodeDirectoryValue->setPlaceholderText(
                QStandardPaths::writableLocation(
                    QStandardPaths::PicturesLocation) + "/mpc_encodes");
    ui->logFilePathValue->setPlaceholderText(
                QStandardPaths::writableLocation(
                    QStandardPaths::DocumentsLocation) + "/mpc-qt.log");

    setupPageTree();
    setupColorPickers();
    setupUnimplementedWidgets();
    ui->pageStack->installEventFilter(this);
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}

bool SettingsWindow::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED (obj)
    Q_UNUSED (event)
    ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(settingsOrKeyMapChanged());
    return false;
}

QVariantMap SettingsWindow::settings()
{
    return acceptedSettings.toVMap();
}

void SettingsWindow::disableWindowManagment()
{
    // Wayland breaks applications
    ui->playerLimitProportions->setDisabled(true);
    ui->playbackAutoCenterWindow->setDisabled(true);
}

void SettingsWindow::setupPageTree()
{
    // Expand every item on pageTree
    QList<QTreeWidgetItem*> stack;
    stack.append(ui->pageTree->invisibleRootItem());
    while (!stack.isEmpty()) {
        QTreeWidgetItem* item = stack.takeFirst();
        item->setExpanded(true);
        for (int i = 0; i < item->childCount(); ++i)
            stack.push_front(item->child(i));
    }

    // Set pageTree pane to be fixed size in resizes
    ui->splitter->setStretchFactor(0,0);
    ui->splitter->setStretchFactor(1,1);

    // Calculate sane default for pageTree width
    ui->pageTree->header()->setStretchLastSection(false);
    ui->pageTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    int pageTreeWidth = ui->pageTree->columnWidth(0);
    ui->pageTree->setMinimumWidth(pageTreeWidth * 11 / 10);
    ui->pageTree->header()->setStretchLastSection(true);

    ui->splitter->setSizes({pageTreeWidth + 10, width() - pageTreeWidth});
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

    ui->stylesheetFusion->setChecked(Platform::isWindows);
    ui->ipcMpris->setVisible(Platform::isUnix);
    ui->hwdecBackendVaapi->setEnabled(Platform::isUnix);
    ui->hwdecBackendNvdec->setEnabled(Platform::isWindows || Platform::isUnix);
    ui->hwdecBackendVdpau->setEnabled(Platform::isUnix);
    ui->hwdecBackendDxva2->setEnabled(Platform::isWindows);
    ui->hwdecBackendD3d11va->setEnabled(Platform::isWindows);
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
        { ui->windowVideoValue, ui->windowVideoPick },
        { ui->windowInfoBackgroundValue, ui->windowInfoBackgroundPick },
        { ui->windowInfoForegroundValue, ui->windowInfoForegroundPick },
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

void SettingsWindow::setupFullscreenCombo()
{
    screenCombo = new ScreenCombo(this);
    screenCombo->setObjectName("fullscreenMonitor");
    ui->fullscreenMonitorLayout->addWidget(screenCombo);
}

void SettingsWindow::setupUnimplementedWidgets()
{
    // Please update the values in setFreestanding
    // when updating this list.

    ui->playerOSD->setVisible(false);
    ui->playerLimitProportions->setVisible(false);
    ui->playerDisableOpenDisc->setVisible(false);
    ui->playerRememberPanScanZoom->setVisible(false);

    ui->shadersWikiTab->setVisible(false);
    ui->shadersPresetsBox->setVisible(false);

    ui->subtitlePlacementBox->setVisible(false);
    ui->subtitlesAssOverride->setVisible(false);
    ui->subtitlesAssOverrideLabel->setVisible(false);

    ui->subtitlesDatabaseBox->setVisible(false);

    ui->encodeTab->setEnabled(false);

    ui->tweaksShowChapterMarks->setVisible(false);
    ui->tweaksPreferWayland->setVisible(Platform::isUnix);

    ui->miscExportKeys->setVisible(false);
    ui->miscExportSettings->setVisible(false);

}

void SettingsWindow::updateAcceptedSettings() {
    acceptedSettings = generateSettingMap(this);
    acceptedKeyMap = actionEditor->toVMap();
}

SettingMap SettingsWindow::generateSettingMap(QWidget *root) const
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
            && item->objectName() != "qt_spinbox_lineedit"
            && item->objectName() != "qt_keysequenceedit_lineedit"
            && item->objectName() != "keysSearchField"
            && item->objectName() != "videoPreset") {
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
    }
    return settingMap;
}

void SettingsWindow::generateVideoPresets()
{
    SettingMap videoWidgets;

    videoWidgets.insert(generateSettingMap(ui->generalTab));
    videoWidgets.insert(generateSettingMap(ui->ditherTab));
    videoWidgets.insert(generateSettingMap(ui->scalingTab));
    videoWidgets.insert(generateSettingMap(ui->debandTab));
    videoWidgets.insert(generateSettingMap(ui->syncMode));

    videoWidgets.remove(ui->videoPreset->objectName());

    auto setWidget = [&videoWidgets](QWidget *x, auto y) {
        videoWidgets[x->objectName()].value = QVariant(y);
    };

    // plain
    // nothing to see here, use the defaults
    videoPresets.append(videoWidgets);

    // low
    setWidget(ui->syncMode, 1); //video-sync=display-resample
    setWidget(ui->scalingTemporalInterpolation, true); //interpolation
    videoPresets.append(videoWidgets);

    // medium
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
    videoPresets.append(videoWidgets);

    // placebo
    setWidget(ui->videoFramebuffer, 5); //fbo=rgba32f
    setWidget(ui->ditherTemporal, true); //temporal-dither=yes
    setWidget(ui->scalingBlendSubtitles, true); //blend-subtitles
    setWidget(ui->tscaleScaler, 11); // tscale=mitchell
    videoPresets.append(videoWidgets);
}

void SettingsWindow::updateLogoWidget()
{
    logoWidget->setLogo(selectedLogo());
}

QString SettingsWindow::selectedLogo() const
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
    updateAcceptedSettings();
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
    for (const AudioDevice &device : std::as_const(audioDevices))
        ui->audioDevice->addItem(device.displayString());
}


// The reason why we're using #define's like this instead of quoted-string
// inspection is because this way guarantees that the app will not break from
// the names here and the names in the ui file not matching up.

#define WIDGET_LOOKUP(widget) \
    acceptedSettings[widget->objectName()].value

#define WIDGET_LOOKUP_PREFIX(prefix, widget) \
    acceptedSettings[prefix + widget->objectName()].value

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
    auto widgetToPrefixHelper = [this](QString wprefix, QString wsuffix)
    {
        auto offsetLookup = [](const SettingMap &source, QString objectName) {
            return source[objectName].value.toInt();
        };
        QString objectName = wprefix + wsuffix;
        return SettingMap::indexedValueToText[objectName].value(offsetLookup(acceptedSettings,objectName),
            SettingMap::indexedValueToText[objectName].value(offsetLookup(defaultSettings,objectName)));
    };
#define WIDGET_TO_TEXT_PREFIX(wp,w) widgetToPrefixHelper(wp,w->objectName())

    // This function is usually ordered by the order they appear in the ui.
    // However some times this is not the case: logging for example should
    // be turned on early.

    bool loggingIsEnabled = WIDGET_LOOKUP(ui->loggingEnabled).toBool();
    emit loggingEnabled(loggingIsEnabled);
    if (loggingIsEnabled) {
        emit logFilePath(WIDGET_LOOKUP(ui->logFileCreate).toBool()
                         ? WIDGET_PLACEHOLD_LOOKUP(ui->logFilePathValue)
                         : QString());
        emit clientDebuggingMessages(WIDGET_LOOKUP(ui->debugClient).toBool());
        emit mpvLogLevel(WIDGET_TO_TEXT(ui->debugMpv));
        emit logDelay(WIDGET_LOOKUP(ui->logUpdateDelayed).toBool() ?
                    WIDGET_LOOKUP(ui->logUpdateInterval).toInt() : -1);
        emit logHistory(WIDGET_LOOKUP(ui->logHistoryTrim).toBool() ?
                        WIDGET_LOOKUP(ui->logHistoryLines).toInt() : 0);
    }

    emit appendToQuickPlaylist(WIDGET_LOOKUP(ui->playerAppendToQuickPlaylist).toBool());
    emit trayIcon(WIDGET_LOOKUP(ui->playerTrayIcon).toBool());
    emit showOsd(WIDGET_LOOKUP(ui->playerOSD).toBool());
    emit limitProportions(WIDGET_LOOKUP(ui->playerLimitProportions).toBool());
    emit disableOpenDiscMenu(WIDGET_LOOKUP(ui->playerDisableOpenDisc).toBool());
    emit inhibitScreensaver(WIDGET_LOOKUP(ui->playerDisableScreensaver).toBool());
    emit titleBarFormat(WIDGET_LOOKUP(ui->playerTitleDisplayFullPath).toBool() ? Helpers::PrefixFullPath
                        : WIDGET_LOOKUP(ui->playerTitleFileNameOnly).toBool() ? Helpers::PrefixFileName
                        : WIDGET_LOOKUP(ui->playerTitleFileTitle).toBool() ? Helpers::PrefixFileTitle : Helpers::NoPrefix);
    emit rememberHistory(WIDGET_LOOKUP(ui->playerKeepHistory).toBool(),
                         WIDGET_LOOKUP(ui->playerKeepHistoryOnlyForVideos).toBool());
    emit rememberFilePosition(WIDGET_LOOKUP(ui->playerRememberFilePosition).toBool());
    if (!WIDGET_LOOKUP(ui->playerKeepHistory).toBool())
        emit rememberQuickPlaylist(false);
    else
        emit rememberQuickPlaylist(WIDGET_LOOKUP(ui->playerRememberQuickPlaylist).toBool());
    emit rememberSelectedPlaylist(WIDGET_LOOKUP(ui->playerRememberLastPlaylist).toBool());
    emit rememberWindowGeometry(WIDGET_LOOKUP(ui->playerRememberWindowGeometry).toBool());
    emit rememberPanels(WIDGET_LOOKUP(ui->playerRememberPanels).toBool());
    emit rememberPanNScan(WIDGET_LOOKUP(ui->playerRememberPanScanZoom).toBool());

    emit mprisIpc(WIDGET_LOOKUP(ui->ipcMpris).toBool());

    emit logoSource(selectedLogo());
    emit iconTheme(static_cast<IconThemer::FolderMode>(WIDGET_LOOKUP(ui->interfaceIconsTheme).toInt()),
                   autoIcons,
                   WIDGET_LOOKUP(ui->interfaceIconsCustomFolder).toString());
    emit highContrastWidgets(WIDGET_LOOKUP(ui->interfaceWidgetHighContast).toBool());
    emit applicationPalette(WIDGET_LOOKUP(ui->interfaceWidgetCustom).toBool()
                            ? paletteEditor->variantToPalette(WIDGET_LOOKUP(paletteEditor))
                            : paletteEditor->systemPalette());
    emit videoColor(QString("#%1").arg(WIDGET_LOOKUP(ui->windowVideoValue).toString()));
    emit option("background-color", QString("#%1").arg(WIDGET_LOOKUP(ui->windowVideoValue).toString()));
    emit infoStatsColors(QString("#%1").arg(WIDGET_LOOKUP(ui->windowInfoForegroundValue).toString()),
                         QString("#%1").arg(WIDGET_LOOKUP(ui->windowInfoBackgroundValue).toString()));

    emit stylesheetIsFusion(WIDGET_LOOKUP(ui->stylesheetFusion).toBool());
    emit stylesheetText(WIDGET_LOOKUP(ui->stylesheetText).toString());

    emit webserverListening(WIDGET_LOOKUP(ui->webEnableServer).toBool());
    emit webserverPort(WIDGET_LOOKUP(ui->webPort).toInt());
    emit webserverLocalhost(WIDGET_LOOKUP(ui->webLocalhost_v2).toBool());
    emit webserverServePages(WIDGET_LOOKUP(ui->webServePages).toBool());
    emit webserverRoot(WIDGET_PLACEHOLD_LOOKUP(ui->webRoot));
    emit webserverDefaultPage(WIDGET_PLACEHOLD_LOOKUP(ui->webDefaultPage));

    int volmax = WIDGET_LOOKUP(ui->tweaksVolumeLimit).toBool() ? 100 : 130;
    emit volumeMax(volmax);
    emit volumeStep(WIDGET_LOOKUP(ui->playbackVolumeStep).toInt());
    {
        int i = WIDGET_LOOKUP(ui->playbackSpeedStep).toInt();
        emit speedStep(i > 0 ? 1.0 + i/100.0 : 1.25);
        emit speedStepAdditive(WIDGET_LOOKUP(ui->playbackSpeedStepAdditive).toBool());
    }
    emit stepTimeNormal(WIDGET_LOOKUP(ui->playbackNormalStep).toInt());
    emit stepTimeLarge(WIDGET_LOOKUP(ui->playbackLargeStep).toInt());

    emit playbackPlayTimes(WIDGET_LOOKUP(ui->playbackPlayAmount).toInt());
    emit playbackForever(WIDGET_LOOKUP(ui->playbackRepeatForever).toBool());
    emit option("image-display-duration", WIDGET_LOOKUP(ui->playbackLoopImages).toBool() ? QVariant("inf") : QVariant(1.0));

    emit afterPlaybackDefault(Helpers::AfterPlayback(WIDGET_LOOKUP(ui->afterPlaybackDefault).toInt()));

    emit option("ytdl-format", QString("bestvideo[height<=?%1]+bestaudio/best").arg(WIDGET_TO_TEXT(ui->ytdlpMaxHeight)));
    emit option("ytdl-raw-options", "js-runtimes=quickjs,remote-components=ejs:github");

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

    emit trackSubtitlePreference(WIDGET_PLACEHOLD_LOOKUP(ui->playbackSubtitleTracks));
    emit trackAudioPreference(WIDGET_PLACEHOLD_LOOKUP(ui->playbackAudioTracks));

    emit option("keep-open", true);
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
    emit option("linear-downscaling", WIDGET_LOOKUP(ui->scalingInLinearLight));
    emit option("linear-upscaling", WIDGET_LOOKUP(ui->scalingUpInLinearLight));
    emit option("interpolation", WIDGET_LOOKUP(ui->scalingTemporalInterpolation));
    emit option("blend-subtitles", WIDGET_LOOKUP(ui->scalingBlendSubtitles));
    if (WIDGET_LOOKUP(ui->scalingSigmoidizedUpscaling).toBool()) {
        emit option("sigmoid-upscaling", true);
        emit option("sigmoid-center", WIDGET_LOOKUP(ui->sigmoidizedCenter));
        emit option("sigmoid-slope", WIDGET_LOOKUP(ui->sigmoidizedSlope));
    } else {
        emit option("sigmoid-upscaling", false);
    }

    QString scaler;
    FilterKernel filter;
    auto fetchFilter = [&](QString prefix, bool temporal = false) {
        scaler = WIDGET_TO_TEXT_PREFIX(prefix, ui->scaleScaler);
        filter = filterKernels.value(scaler);
        filter.cutoff_(temporal ? 0.0 : 0.01);
        filter.clamp_(temporal ? 1.0 : 0.0);
        if (WIDGET_LOOKUP_PREFIX(prefix, ui->scaleParam1Set).toBool())    filter.param1_(WIDGET_LOOKUP_PREFIX(prefix, ui->scaleParam1Value).toDouble());
        if (WIDGET_LOOKUP_PREFIX(prefix, ui->scaleParam2Set).toBool())    filter.param2_(WIDGET_LOOKUP_PREFIX(prefix, ui->scaleParam2Value).toDouble());
        if (WIDGET_LOOKUP_PREFIX(prefix, ui->scaleRadiusSet).toBool())    filter.radius_(WIDGET_LOOKUP_PREFIX(prefix, ui->scaleRadiusValue).toDouble());
        if (WIDGET_LOOKUP_PREFIX(prefix, ui->scaleAntiRingSet).toBool())  filter.antiring_(WIDGET_LOOKUP_PREFIX(prefix, ui->scaleAntiRingValue).toDouble());
        if (WIDGET_LOOKUP_PREFIX(prefix, ui->scaleBlurSet).toBool())      filter.blur_(WIDGET_LOOKUP_PREFIX(prefix, ui->scaleBlurValue).toDouble());
        if (WIDGET_LOOKUP_PREFIX(prefix, ui->scaleWindowSet).toBool())    filter.window_(WIDGET_TO_TEXT_PREFIX(prefix, ui->scaleWindowValue));
        if (WIDGET_LOOKUP_PREFIX(prefix, ui->scaleWindowParamSet).toBool())   filter.window.param_(WIDGET_LOOKUP_PREFIX(prefix, ui->scaleWindowValue).toDouble());
        if (WIDGET_LOOKUP_PREFIX(prefix, ui->scaleClampSet).toBool())     filter.clamp_(WIDGET_TO_TEXT_PREFIX(prefix, ui->scaleClampValue).toDouble());
    };
    auto applyFilter = [&](QString prefix) {
        emit option(prefix + "scale", scaler);
        emit option(prefix + "scale-param1", filter.params[0]);
        emit option(prefix + "scale-param2", filter.params[1]);
        emit option(prefix + "scale-radius", filter.radius);
        emit option(prefix + "scale-antiring", filter.antiring);
        emit option(prefix + "scale-blur", filter.blur);
        emit option(prefix + "scale-window", filter.windowName);
        emit option(prefix + "scale-wparam", filter.window.params[0]);
        emit option(prefix + "scale-clamp", filter.clamp);
    };

    fetchFilter("");
    applyFilter("");

    if (OFFSET_LOOKUP(acceptedSettings, ui->dscaleScaler) != 0)
        fetchFilter("d");
    applyFilter("d");

    fetchFilter("c");
    applyFilter("c");

    fetchFilter("t", true);
    applyFilter("t");

    if (WIDGET_LOOKUP(ui->debandEnabled).toBool()) {
        emit option("deband", true);
        emit option("deband-iterations", WIDGET_LOOKUP(ui->debandIterations));
        emit option("deband-threshold", WIDGET_LOOKUP(ui->debandThreshold));
        emit option("deband-range", WIDGET_LOOKUP(ui->debandRange));
        emit option("deband-grain", WIDGET_LOOKUP(ui->debandGrain));
    } else {
        emit option("deband", false);
    }

    emit option("gamut-mapping-mode", WIDGET_TO_TEXT(ui->ccGamutMapping));
    emit option("target-gamut", WIDGET_TO_TEXT(ui->ccTargetGamut));
    emit option("target-prim", WIDGET_TO_TEXT(ui->ccTargetPrim));
    emit option("target-trc", WIDGET_TO_TEXT(ui->ccTargetTrc_v2));
    int targetPeak = WIDGET_LOOKUP(ui->ccTargetPeak).toInt();
    emit option("target-peak", targetPeak >= 10 ? QString::number(targetPeak) : QString("auto"));
    emit option("tone-mapping", WIDGET_TO_TEXT(ui->ccHdrMapper));
    {
        QList<QDoubleSpinBox*> boxen {ui->ccHdrClipParam,
                    ui->ccHdrMobiusParam, ui->ccHdrReinhardParam, nullptr,
                    ui->ccHdrGammaParam, ui->ccHdrLinearParam};
        QDoubleSpinBox* toneParam = boxen[WIDGET_LOOKUP(ui->ccHdrMapper).toInt()];
        emit option("tone-mapping-param", toneParam ? WIDGET_LOOKUP(toneParam) : QVariant(NAN));
    }
    emit option("hdr-compute-peak", WIDGET_TO_TEXT(ui->ccHdrCompute));
    if (WIDGET_LOOKUP(ui->ccICCAutodetect).toBool()) {
        emit option("icc-profile", "");
        emit option("icc-profile-auto", true);
    } else {
        emit option("icc-profile-auto", false);
        emit option("icc-profile", WIDGET_LOOKUP(ui->ccICCLocation));
    }
    // FIXME: add icc-intent etc

    emit option("glsl-shaders", WIDGET_LOOKUP(ui->shadersActiveList).toStringList());

    emit fullscreenScreen(WIDGET_LOOKUP(screenCombo).toString());
    emit fullscreenAtLaunch(WIDGET_LOOKUP(ui->fullscreenLaunch).toBool());
    emit fullscreenExitAtEnd(WIDGET_LOOKUP(ui->fullscreenWindowedAtEnd).toBool());
    emit fullscreenHideControls(WIDGET_LOOKUP(ui->fullscreenHideControls).toBool(), \
        WIDGET_LOOKUP(ui->fullscreenShowWhen).toInt(), WIDGET_LOOKUP(ui->fullscreenShowWhenDuration).toInt());
    emit hidePanels(WIDGET_LOOKUP(ui->fullscreenHidePanels).toBool());
    emit option("framedrop", WIDGET_TO_TEXT(ui->framedroppingMode));
    emit option("vf-lavc-framedrop", WIDGET_TO_TEXT(ui->framedroppingDecoderMode));
    emit option("video-sync-adrop-size", WIDGET_LOOKUP(ui->syncAudioDropSize).toDouble());
    emit option("video-sync-max-audio-change", WIDGET_LOOKUP(ui->syncMaxAudioChange).toDouble());
    emit option("video-sync-max-video-change", WIDGET_LOOKUP(ui->syncMaxVideoChange).toDouble());
    if (WIDGET_LOOKUP(ui->hwdecEnable).toBool()) {
        QString backend = "auto";
        if (WIDGET_LOOKUP(ui->hwdecBackendVaapi).toBool())
            backend = "vaapi,vaapi-copy";
        if (WIDGET_LOOKUP(ui->hwdecBackendNvdec).toBool())
            backend = "nvdec,nvdec-copy";
        if (WIDGET_LOOKUP(ui->hwdecBackendVdpau).toBool())
            backend = "vdpau,vdpau-copy";
        if (WIDGET_LOOKUP(ui->hwdecBackendDxva2).toBool())
            backend = "dxva2,dxva2-copy";
        if (WIDGET_LOOKUP(ui->hwdecBackendD3d11va).toBool())
            backend = "d3d11va,d3d11va-copy";
        if (WIDGET_LOOKUP(ui->hwdecBackendCuda).toBool())
            backend = "cuda,cuda-copy";
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
    emit option("pipewire-buffer", WIDGET_LOOKUP(ui->pipewireBuffer).toInt());
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
    bool audioAutoload = WIDGET_LOOKUP(ui->audioAutoloadExternal).toBool();
    emit option("audio-file-auto", audioAutoload ? WIDGET_TO_TEXT(ui->audioAutoloadMatch) : "no");
    emit option("audio-file-paths", WIDGET_PLACEHOLD_LOOKUP(ui->audioAutoloadPath).split(';'));

    emit option("sub-gray", WIDGET_LOOKUP(ui->subtitlesForceGrayscale).toBool());
    emit option("sub-font", WIDGET_LOOKUP(ui->fontComboBox).toString());
    emit option("sub-bold", WIDGET_LOOKUP(ui->fontBold).toBool());
    emit option("sub-italic", WIDGET_LOOKUP(ui->fontItalic).toBool());
    emit option("sub-font-size", WIDGET_LOOKUP(ui->fontSize).toInt());
    emit option("sub-border-size", WIDGET_LOOKUP(ui->borderSize).toInt());

    emit option("sub-fix-timing", WIDGET_LOOKUP(ui->subtitlesFixTiming).toBool());
    emit option("sub-clear-on-seek", WIDGET_LOOKUP(ui->subtitlesClearOnSeek).toBool());
    emit subtitlesDelayStep(WIDGET_LOOKUP(ui->subtitlesDelayStep).toInt());
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
    emit option("sub-ass", !WIDGET_LOOKUP(ui->subsAssoverride).toBool());
    emit option("sub-margin-x", WIDGET_LOOKUP(ui->subsMarginX).toInt());
    emit option("sub-margin-y", WIDGET_LOOKUP(ui->subsMarginY).toInt());
    emit option("sub-use-margins", !WIDGET_LOOKUP(ui->subsRelativeToVideoFrame).toBool());
    emit option("sub-ass-force-margins", !WIDGET_LOOKUP(ui->subsAssRelativeToVideoFrame).toBool());
    emit option("sub-color", QString("#%1").arg(WIDGET_LOOKUP(ui->subsColorValue).toString()));
    emit option("sub-border-color", QString("#%1").arg(WIDGET_LOOKUP(ui->subsBorderColorValue).toString()));
    emit option("sub-shadow-offset", WIDGET_LOOKUP(ui->subsShadowOffset).toInt());
    if (WIDGET_LOOKUP(ui->subsShadowEnabled).toBool())
        emit option("sub-back-color", QString("#%1").arg(WIDGET_LOOKUP(ui->subsShadowColorValue).toString()));
    else
        emit option("sub-back-color", "#00000000");

    if (WIDGET_LOOKUP(ui->subsBackgroundBoxEnabled).toBool()) {
        emit option("sub-border-style", "background-box");
        emit option("sub-shadow-offset", 0);
        emit option("sub-back-color", WIDGET_LOOKUP(ui->subsBackgroundBoxTranslucid).toBool() ?
                                        "#99000000" : "#FF000000");
    }
    else
        emit option("sub-border-style", "outline-and-shadow");

    emit subsPreferDefaultForced(WIDGET_LOOKUP(ui->subtitlesPreferDefaultForced_v3).toBool());
    emit subsPreferExternal(WIDGET_LOOKUP(ui->subtitlesPreferExternal).toBool());
    emit subsIgnoreEmbeded(WIDGET_LOOKUP(ui->subtitlesIgnoreEmbedded).toBool());
    bool subsAutoload = WIDGET_LOOKUP(ui->subtitlesAutoloadExternal).toBool();
    emit option("sub-filter-sdh", WIDGET_LOOKUP(ui->subtitlesRemoveSdh).toBool());
    emit option("sub-auto", subsAutoload ? WIDGET_TO_TEXT(ui->subtitlesAutoloadMatch) : QString("no"));
    emit option("sub-file-paths", WIDGET_PLACEHOLD_LOOKUP(ui->subtitlesAutoloadPath).split(';'));

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

    emit fastSeek(WIDGET_LOOKUP(ui->tweaksFastSeek).toBool());
    emit option("hr-seek-framedrop", WIDGET_LOOKUP(ui->tweaksSeekFramedrop).toBool());
    emit fallbackToFolder(WIDGET_LOOKUP(ui->tweaksOpenNextFile).toBool());
    emit mpvMouseEvents(WIDGET_LOOKUP(ui->tweaksMpvMouseEvents).toBool());
    emit mpvKeyEvents(WIDGET_LOOKUP(ui->tweaksMpvKeyEvents).toBool());
    emit timeShorten(WIDGET_LOOKUP(ui->tweaksTimeShort).toBool());
    emit videoPreview(WIDGET_LOOKUP(ui->tweaksVideoPreview).toBool());
    emit timeTooltip(WIDGET_LOOKUP(ui->tweaksTimeTooltip).toBool(),
                     WIDGET_LOOKUP(ui->tweaksTimeTooltipLocation).toInt() == 0);
    emit osdTimerOnSeek(WIDGET_LOOKUP(ui->tweaksOsdTimerOnSeek).toBool());
    emit option("osd-font", WIDGET_LOOKUP(ui->tweaksOsdFontChkBox).toBool() ? WIDGET_LOOKUP(ui->tweaksOsdFont).toString() : "");
    emit option("osd-font-size", WIDGET_LOOKUP(ui->tweaksOsdFontChkBox).toBool() ? WIDGET_LOOKUP(ui->tweaksOsdSize).toInt() : 55);
    emit option("brightness", WIDGET_LOOKUP(ui->miscBrightness).toInt());
    emit option("contrast", WIDGET_LOOKUP(ui->miscContrast).toInt());
    emit option("gamma", WIDGET_LOOKUP(ui->miscGamma).toInt());
    emit option("hue", WIDGET_LOOKUP(ui->miscHue).toInt());
    emit option("saturation", WIDGET_LOOKUP(ui->miscSaturation).toInt());
    setCustomMpvOptions();
}

void SettingsWindow::sendAcceptedSettings()
{
    emit settingsData(acceptedSettings.toVMap());
    emit keyMapData(acceptedKeyMap);
}

bool SettingsWindow::settingsOrKeyMapChanged()
{
    SettingMap currentSettings = generateSettingMap(this);
    QVariantMap currentKeyMap = actionEditor->toVMap();

    return (currentSettings.toVMap() != acceptedSettings.toVMap()) ||
           (currentKeyMap != acceptedKeyMap);
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
    ui->playerOpenSame->setVisible(yes);
    ui->playerOpenNew->setVisible(yes);
    ui->playerAppendToQuickPlaylist->setVisible(yes);
    ui->playerOpenBox->setEnabled(yes);
    ui->playerKeepHistory->setVisible(yes);
    ui->playerKeepHistoryOnlyForVideos->setVisible(yes);
    ui->playerRememberFilePosition->setVisible(yes);
    ui->playerRememberQuickPlaylist->setVisible(yes);
    ui->playerRememberLastPlaylist->setVisible(yes);
    ui->playerRememberWindowGeometry->setVisible(yes);
    ui->playerRememberPanels->setVisible(yes);
    ui->playerRememberPanScanZoom->setVisible(false /*yes*/);
    ui->playerHistoryBox->setEnabled(yes);
    ui->webTcpIpBox->setEnabled(yes);
    ui->webLocalFilesBox->setEnabled(yes);
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

void SettingsWindow::setHidePanels(bool hidden)
{
    ui->fullscreenHidePanels->setChecked(hidden);
    WIDGET_LOOKUP(ui->fullscreenHidePanels).setValue(hidden);
    emit settingsData(acceptedSettings.toVMap());
}

void SettingsWindow::setAudioFilter(QString filter, QString options, bool add)
{
    setFilter(audioFiltersList, filter, options, add);
    emit audioFilters(audioFiltersList);
}

void SettingsWindow::setVideoFilter(QString filter, QString options, bool add)
{
    setFilter(videoFiltersList, filter, options, add);
    emit videoFilters(videoFiltersList);
}

void SettingsWindow::setFilter(QList<QPair<QString, QString>> &filtersList, QString filter, QString options, bool add)
{
    auto it = std::find_if(filtersList.begin(),
                                     filtersList.end(),
                                     [&](const QPair<QString, QString> &pair) {
        return pair.first == filter;
    });

    if (it != filtersList.end()) { // Filter found
        int i = std::distance(filtersList.begin(), it);
        if (add)
            filtersList.replace(i, QPair<QString, QString>(filter, options));
        else
            filtersList.erase(it);
    }
    else
        filtersList.append(QPair<QString, QString>(filter, options));
}

void SettingsWindow::setCustomMpvOptions()
{
    if (WIDGET_LOOKUP(ui->tweaksMpvOptionsChkBox).toBool()) {
        QList<MpvOption> mpvOptions = parseMpvOptions(WIDGET_LOOKUP(ui->tweaksMpvOptionsText).toString());
        for (const auto &mpvOption : mpvOptions) {
            emit optionUncached(mpvOption.name, mpvOption.value);
        }
    }
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

void SettingsWindow::colorPick_changed(const QLineEdit *colorValue, QPushButton *colorPick)
{
    colorPick->setStyleSheet(QString("background: #%1").arg(colorValue->text()));
}

QList<MpvOption> SettingsWindow::parseMpvOptions(const QString &optionsInline) const
{
    QList<MpvOption> mpvOptions;
    const QStringList options = optionsInline.split(' ', Qt::SkipEmptyParts);
    for (const QString &option : options) {
        int equalPos = option.indexOf('=');
        if (equalPos > 0) {
            QString name = option.left(equalPos).trimmed();
            QString value = option.mid(equalPos + 1).trimmed();
            mpvOptions.append(MpvOption{name, value});
        } else {
            mpvOptions.append(MpvOption{option.trimmed(), QString()});
        }
    }
    return mpvOptions;
}

void SettingsWindow::on_pageTree_itemSelectionChanged()
{
    QModelIndex modelIndex = ui->pageTree->currentIndex();
    if (!modelIndex.isValid())
        return;

    static int parentIndex[] = { 0, 6, 13, 14, 17, 19, 20, 21 };
    int index = 0;
    if (!modelIndex.parent().isValid())
        index = parentIndex[modelIndex.row()];
    else
        index = parentIndex[modelIndex.parent().row()] + modelIndex.row() + 1;
    ui->pageStack->setCurrentIndex(index);
    ui->pageLabel->setText(QString("<big><b>%1</b></big>").
                           arg(modelIndex.data().toString()));

    if (ui->pageStack->currentWidget() == ui->keysPage)
        ui->keysSearchField->setFocus();
}

void SettingsWindow::on_buttonBox_clicked(QAbstractButton *button)
{
    QDialogButtonBox::ButtonRole buttonRole;
    buttonRole = ui->buttonBox->buttonRole(button);
    if (buttonRole != QDialogButtonBox::RejectRole) {
        updateAcceptedSettings();
        sendAcceptedSettings();
        actionEditor->updateActions();
    }
    if (buttonRole == QDialogButtonBox::ApplyRole) {
        sendSignals();
        ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
    }
    else
        close();
}

void SettingsWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    sendSignals();
    setCustomMpvOptions();
    ui->keysSearchField->clear();
    ui->videoPreset->setCurrentIndex(0);
}

void SettingsWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
        this->close();
    else
        QWidget::keyPressEvent(event);
}

void SettingsWindow::on_playerOpenNew_toggled(bool checked)
{
    ui->playerAppendToQuickPlaylist->setEnabled(!checked);
    if (checked)
        ui->playerAppendToQuickPlaylist->setChecked(false);
}

void SettingsWindow::on_playerAppendToQuickPlaylist_toggled(bool checked)
{
    if (checked)
        ui->playerRememberQuickPlaylist->setChecked(false);
}

void SettingsWindow::on_playerKeepHistory_toggled(bool checked)
{
    bool playerKeepHistoryEnabled = checked;
    ui->playerRememberFilePosition->setChecked(playerKeepHistoryEnabled);
    ui->playerRememberQuickPlaylist->setChecked(playerKeepHistoryEnabled);
    ui->playerKeepHistoryOnlyForVideos->setEnabled(playerKeepHistoryEnabled);
    ui->playerRememberFilePosition->setEnabled(playerKeepHistoryEnabled);
    ui->playerRememberQuickPlaylist->setEnabled(playerKeepHistoryEnabled);
}

void SettingsWindow::on_interfaceIconsTheme_currentIndexChanged(int index)
{
    bool customIcons = index == 1;
    ui->interfaceIconsCustomFolder->setEnabled(customIcons);
    ui->interfaceIconsCustomBrowse->setEnabled(customIcons);
    ui->interfaceIconsCustomLabel->setEnabled(customIcons);
    ui->interfaceIconsNotice->setEnabled(customIcons);
}

void SettingsWindow::on_interfaceWidgetCustom_toggled(bool checked)
{
    ui->interfaceWidgetCustomScrollArea->setEnabled(checked);
}

void SettingsWindow::on_interfaceIconsCustomBrowse_clicked()
{
    static QFileDialog::Options options = QFileDialog::Options();
#ifdef Q_OS_MAC
    options.setFlag(QFileDialog::DontUseNativeDialog)
#endif
    QString dir = ui->interfaceIconsCustomFolder->text();
    dir = QFileDialog::getExistingDirectory(this, "", dir, options);
    if (dir.isEmpty())
        return;

    ui->interfaceIconsCustomFolder->setText(dir);
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
    static QFileDialog::Options options = QFileDialog::Options();
#ifdef Q_OS_MAC
    options = QFileDialog::DontUseNativeDialog;
#endif
    QString file = ui->logoExternalLocation->text();
    file = QFileDialog::getOpenFileName(this, tr("Open Logo Image"), file, "", nullptr, options);
    if (file.isEmpty())
        return;

    ui->logoExternalLocation->setText(file);
    updateLogoWidget();
}

void SettingsWindow::on_logoExternal_toggled(bool checked)
{
    ui->logoInternal->setEnabled(!checked);
    ui->logoExternalLocation->setEnabled(checked);
    ui->logoExternalBrowse->setEnabled(checked);
    updateLogoWidget();
}

void SettingsWindow::on_logoInternal_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    updateLogoWidget();
}

void SettingsWindow::on_videoPreset_currentIndexChanged(int index)
{
    if (index > 0) {
        for (Setting &s : videoPresets[index-1])
            s.sendToControl();
        ui->videoPresetApplied->setText(tr("Preset applied"));
        QTimer::singleShot(3000, this, [this]() {
            ui->videoPresetApplied->clear();
        });
    }
}

void SettingsWindow::on_ccHdrCompute_currentIndexChanged(int index)
{
    // Alpha channel is needed for HDR Compute Peak
    if (index < 2)
        ui->videoUseAlpha->setChecked(true);
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
    static QFileDialog::Options options = QFileDialog::Options();
#ifdef Q_OS_MAC
    options = QFileDialog::DontUseNativeDialog;
#endif
    QStringList files = QFileDialog::getOpenFileNames(this, "", "", "", nullptr, options);
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
    for (auto item : std::as_const(items))
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
}

void SettingsWindow::on_fullscreenShowWhen_currentIndexChanged(int index)
{
    ui->fullscreenShowWhenDuration->setEnabled(index != 0);
}

void SettingsWindow::on_audioBalance_valueChanged(int value)
{
    QToolTip::showText(QCursor::pos(), QString::number(value), ui->audioBalance);
    if (value == 0)
        setAudioFilter("stereotools", "", false);
    else
        setAudioFilter("stereotools", "balance_out=" + QString::number((double) value/100), true);
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

void SettingsWindow::on_ccICCAutodetect_toggled(bool checked)
{
    ui->ccICCLocation->setEnabled(!checked);
    ui->ccICCBrowse->setEnabled(!checked);
}

void SettingsWindow::on_ccICCBrowse_clicked()
{
    static QFileDialog::Options options = QFileDialog::Options();
#ifdef Q_OS_MAC
    options = QFileDialog::DontUseNativeDialog;
#endif
    QString file = WIDGET_LOOKUP(ui->ccICCLocation).toString();
    file = QFileDialog::getOpenFileName(this, tr("Open ICC Profile"),
                                        file, tr("ICC profile files (*.icc *.icm)"),
                                        nullptr, options);
    if (file.isEmpty())
        return;

    ui->ccICCLocation->setText(file);
}

void SettingsWindow::on_playbackPlayTimes_toggled(bool checked)
{
    ui->playbackPlayAmount->setEnabled(checked);
    ui->playbackPlayTimesLabel->setEnabled(checked);
}

void SettingsWindow::on_ditherDithering_toggled(bool checked)
{
    ui->ditherDepth->setEnabled(checked);
    ui->ditherDepthLabel->setEnabled(checked);
    ui->ditherType->setEnabled(checked);
    ui->ditherTypeLabel->setEnabled(checked);
    bool fruitEnabled = checked && (ui->ditherType->currentIndex() == 0);
    ui->ditherFruitSize->setEnabled(fruitEnabled);
    ui->ditherFruitSizeLabel->setEnabled(fruitEnabled);
}

void SettingsWindow::on_ditherType_currentIndexChanged(int index)
{
    bool fruitEnabled = index == 0;
    ui->ditherFruitSize->setEnabled(fruitEnabled);
    ui->ditherFruitSizeLabel->setEnabled(fruitEnabled);
}

void SettingsWindow::on_ditherTemporal_toggled(bool checked)
{
    ui->ditherTemporalPeriod->setEnabled(checked);
    ui->ditherTemporalPeriodLabel->setEnabled(checked);
}

void SettingsWindow::on_audioSpdif_toggled(bool checked)
{
    ui->audioSpdifCodecs->setEnabled(checked);
}

void SettingsWindow::on_subsBackgroundBoxEnabled_toggled(bool checked)
{
    ui->subsBackgroundBoxTranslucid->setEnabled(checked);
    ui->subsShadowEnabled->setEnabled(!checked);
    on_subsShadowEnabled_toggled(!checked);
    if (checked)
        ui->subsShadowEnabled->setChecked(false);
}

void SettingsWindow::on_subsShadowEnabled_toggled(bool checked)
{
    ui->subsShadowColorLabel->setEnabled(checked);
    ui->subsShadowColorValue->setEnabled(checked);
    ui->subsShadowColorPick->setEnabled(checked);
    ui->subsShadowOffsetLabel->setEnabled(checked);
    ui->subsShadowOffset->setEnabled(checked);
}

void SettingsWindow::on_screenshotDirectorySet_toggled(bool checked)
{
    ui->screenshotDirectoryValue->setEnabled(checked);
    ui->screenshotDirectoryBrowse->setEnabled(checked);
}

void SettingsWindow::on_screenshotDirectoryBrowse_clicked()
{
    static QFileDialog::Options options = QFileDialog::Options();
#ifdef Q_OS_MAC
    options.setFlag(QFileDialog::DontUseNativeDialog)
#endif
    QString dir = ui->screenshotDirectoryValue->text().isEmpty() ?
                                    ui->screenshotDirectoryValue->placeholderText() :
                                    ui->screenshotDirectoryValue->text();
    dir = QFileDialog::getExistingDirectory(this, "", dir, options);
    if (dir.isEmpty())
        return;

    ui->screenshotDirectoryValue->setText(dir);
}

// REMOVEME: Disable auto zoom in Wayland mode as window centering isn't possible yet
void SettingsWindow::on_tweaksPreferWayland_toggled(bool checked)
{
    if (checked && QGuiApplication::platformName() == "wayland")
        ui->playbackAutoZoom->setChecked(false);
}

void SettingsWindow::on_tweaksTimeTooltip_toggled(bool checked)
{
    ui->tweaksTimeTooltipLocation->setEnabled(checked);
}

void SettingsWindow::on_tweaksOsdFontChkBox_toggled(bool checked)
{
    ui->tweaksOsdFont->setEnabled(checked);
    ui->tweaksOsdSize->setEnabled(checked);
}

void SettingsWindow::on_tweaksMpvOptionsChkBox_toggled(bool checked)
{
    ui->tweaksMpvOptionsText->setEnabled(checked);
}

void SettingsWindow::on_loggingEnabled_toggled(bool checked)
{
    ui->debugBox->setEnabled(checked);
    ui->logFileBox->setEnabled(checked);
    ui->logUpdateBox->setEnabled(checked);
    ui->logHistoryBox->setEnabled(checked);
}

void SettingsWindow::on_logFileCreate_toggled(bool checked)
{
    ui->logFilePathLabel->setEnabled(checked);
    ui->logFilePathValue->setEnabled(checked);
    ui->logFilePathBrowse->setEnabled(checked);
}

void SettingsWindow::on_logUpdateDelayed_toggled(bool checked)
{
    ui->logUpdateInterval->setEnabled(checked);
}

void SettingsWindow::on_logHistoryTrim_toggled(bool checked)
{
    ui->logHistoryLines->setEnabled(checked);
}

void SettingsWindow::on_miscBrightness_valueChanged(int value)
{
    ui->miscBrightnessValue->setText(QString("%1").arg(value, 4, 10, QChar(' ')));
    emit option("brightness", value);
}

void SettingsWindow::on_miscContrast_valueChanged(int value)
{
    ui->miscContrastValue->setText(QString("%1").arg(value, 4, 10, QChar(' ')));
    emit option("contrast", value);
}

void SettingsWindow::on_miscGamma_valueChanged(int value)
{
    ui->miscGammaValue->setText(QString("%1").arg(value, 4, 10, QChar(' ')));
    emit option("gamma", value);
}

void SettingsWindow::on_miscHue_valueChanged(int value)
{
    ui->miscHueValue->setText(QString("%1").arg(value, 4, 10, QChar(' ')));
    emit option("hue", value);
}

void SettingsWindow::on_miscSaturation_valueChanged(int value)
{
    ui->miscSaturationValue->setText(QString("%1").arg(value, 4, 10, QChar(' ')));
    emit option("saturation", value);
}

void SettingsWindow::on_miscResetColor_clicked()
{
    ui->miscBrightness->setValue(0);
    ui->miscContrast->setValue(0);
    ui->miscGamma->setValue(0);
    ui->miscHue->setValue(0);
    ui->miscSaturation->setValue(0);
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
    logoWidget->setLogoBackground(QString("#%1").arg(arg1));
}

void SettingsWindow::on_subtitlesAutoloadReset_clicked()
{
    ui->subtitlesAutoloadPath->clear();
}

void SettingsWindow::on_audioAutoloadPathReset_clicked()
{
    ui->audioAutoloadPath->clear();
}

void SettingsWindow::on_webPortLink_linkActivated(const QString &link)
{
    Q_UNUSED(link);
    QUrl url("http://127.0.0.1");
    url.setPort(ui->webPort->value());
    QDesktopServices::openUrl(url);
}

void SettingsWindow::on_webRootBrowse_clicked()
{
    static QFileDialog::Options options = QFileDialog::ShowDirsOnly;
#ifdef Q_OS_MAC
    options |= QFileDialog::DontUseNativeDialog;
#endif
    QString path = QFileDialog::getExistingDirectory(this, "", "", options);
    if (path.isEmpty())
        return;
    ui->webRoot->setText(path);
}

void SettingsWindow::on_logFilePathBrowse_clicked()
{
    static QFileDialog::Options options = QFileDialog::Options();
#ifdef Q_OS_MAC
    options |= QFileDialog::DontUseNativeDialog;
#endif
    QString file = ui->logFilePathValue->text().isEmpty() ? ui->logFilePathValue->placeholderText() :
                                                            ui->logFilePathValue->text();
    file = QFileDialog::getSaveFileName(this, tr("Choose Log File"), file, "*.log *.txt", nullptr, options);
    if (file.isEmpty())
        return;
    ui->logFilePathValue->setText(file);
}

