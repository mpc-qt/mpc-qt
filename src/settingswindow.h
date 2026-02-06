#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QWidget>
#include <QTreeWidgetItem>
#include <QAbstractButton>
#include <QPushButton>
#include <QVariantMap>

#include <functional>

#include "helpers.h"
#include "widgets/actioneditor.h"
#include "widgets/logowidget.h"
#include "widgets/paletteeditor.h"
#include "widgets/screencombo.h"

class Setting {
public:
    Setting() = default;
    Setting(const Setting &s) = default;
    Setting(QString name, QWidget *widget, QVariant const &value) : name(name), widget(widget), value(value) {}
    Setting &operator =(const Setting &s);
    void sendToControl();
    void fetchFromControl();

    ~Setting() {}
    QString name;
    QWidget *widget = nullptr;
    QVariant value;

    static QMap<QString, std::function<QVariant (QObject *)> > classFetcher;
    static QMap<QString, std::function<void (QObject *, const QVariant &)> > classSetter;

//private:
    static QMap<QString,const char*> classToProperty;
};

struct SettingMap : public QHash<QString, Setting> {
public:
    static QHash<QString, QStringList> indexedValueToText;
    QVariantMap toVMap();
    void fromVMap(const QVariantMap &m);
};

struct MpvOption {
    QString name;
    QString value;
};

namespace Ui {
class SettingsWindow;
}

class SettingsWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget *parent = nullptr);
    ~SettingsWindow();
    QVariantMap settings();
    QVariantMap keyMap();
    void setWaylandOptions(bool isWayland, bool isWaylandMode);

private:
    void setupPageTree();
    void setupPlatformWidgets();
    void setupPaletteEditor();
    void setupColorPickers();
    void setupFullscreenCombo();
    void setupUnimplementedWidgets();
    void updateAcceptedSettings();
    SettingMap generateSettingMap(QWidget *root) const;
    void generateVideoPresets();
    void updateLogoWidget();
    QString selectedLogo() const;
    QString channelSwitcher();

signals:
    void settingsData(const QVariantMap &s);
    void keyMapData(const QVariantMap &s);
    void mouseWindowedMap(const MouseStateMap &map);
    void mouseFullscreenMap(const MouseStateMap &map);

    void appendToQuickPlaylist(bool yes);
    void trayIcon(bool yes);
    void showOsd(bool yes);
    void limitProportions(bool yes);
    void disableOpenDiscMenu(bool yes);
    void inhibitScreensaver(bool yes);
    void titleBarFormat(Helpers::TitlePrefix format);
    void rememberHistory(bool yes, bool onlyVideos);
    void rememberFilePosition(bool yes);
    void rememberQuickPlaylist(bool yes);
    void rememberSelectedPlaylist(bool yes);
    void rememberWindowGeometry(bool yes);
    void rememberPanels(bool yes);
    void rememberPanNScan(bool yes);

    void mprisIpc(bool enabled);
    void logoSource(const QString &s);
    void iconTheme(IconThemer::FolderMode folderMode, const QString &fallbackFolder, const QString &customFolder);
    void highContrastWidgets(bool yes);
    void applicationPalette(const QPalette s);
    void videoColor(QColor background);
    void infoStatsColors(QColor foreground, QColor background);

    void stylesheetIsFusion(bool yes);
    void stylesheetText(QString text);

    void webserverListening(bool listening);
    void webserverPort(uint16_t port);
    void webserverLocalhost(bool localhost);
    void webserverServePages(bool yes);
    void webserverRoot(QString root);
    void webserverDefaultPage(QString page);

    void balance(double pan);
    void volumeStep(int amount);
    void speedStep(double amount);
    void speedStepAdditive(bool isAdditive);
    void stepTimeNormal(int msec);
    void stepTimeLarge(int msec);
    void zoomPreset(int which, double fitFactor);
    void zoomCenter(bool yes);
    void mouseHideTimeFullscreen(int msec);
    void mouseHideTimeWindowed(int msec);
    void trackSubtitlePreference(const QString &pref);
    void trackAudioPreference(const QString &pref);
    void autoLoadAudio(bool yes);
    void autoLoadSubs(bool yes);

    void option(const QString &s, const QVariant &v);
    void optionUncached(const QString &s, const QVariant &v);

    void fullscreenScreen(QString screen);
    void fullscreenAtLaunch(bool yes);
    void fullscreenExitAtEnd(bool yes);
    void fullscreenHideControls(bool yes, int showWhen, int showWhenDuration,
                                bool setControlsInFullscreen = true);
    void hidePanels(bool yes);

    void playbackPlayTimes(int count);
    void afterPlaybackDefault(Helpers::AfterPlayback option);
    void playbackForever(bool yes);
    void playbackRewinds(bool yes);
    void playbackLoopImages(bool yes);
    void playlistFormat(const QString &fmt);

    void subtitlesDelayStep(int subtitlesDelayStep);

    // does mpv even *need* this?
    void subsPreferDefaultForced(bool yes);
    void subsPreferExternal(bool yes);
    void subsIgnoreEmbeded(bool yes);
    void susbsAutoloadPath(const QString &s);
    void subsDatabseUrl(const QUrl &url);

    void screenshotDirectory(const QString &s);
    void encodeDirectory(const QString &s);
    void screenshotTemplate(const QString &s);
    void encodeTemplate(const QString &s);

    void screenshotFormat(const QString &s);
    void encodeCodecs(const QString &videoCodec, const QString &audioCodec);
    void encodeStreams(bool noVideo, bool noAudio);
    void encodeHardsubs(bool yes);
    void encodeVideoMethod(bool useBitrate);
    void encodeVideoSize(int kilobytes);
    void encodeVideoBitrate(int kilobits);
    void encodeVideoCrf(int crf);
    void encodeVideoQMin(int qmin);
    void encodeVideoQMax(int qmax);
    void encodeAudioBitrate(int kilobits);

    void chapterMarks(bool yes);
    void fastSeek(bool yes);
    void fallbackToFolder(bool yes);
    void volumeMax(int maximum);
    void mpvMouseEvents(bool yes);
    void mpvKeyEvents(bool yes);
    void videoPreview(bool enable);
    void timeTooltip(bool yes, bool above);
    void osdTimerOnSeek(bool yes);
    void osdFont(const QString &family, const QString &size);

    // bchs should be part of a filter module page, hence the funny name
    void boschDishwasher(int brightness, int contrast, int hue, int saturation);

    void loggingEnabled(bool enabled);
    void clientDebuggingMessages(bool yes);
    void mpvLogLevel(const QString &s);
    void logFilePath(const QString &path);
    void logDelay(int msecs);
    void logHistory(int lines);

    void audioFilters(const QList<QPair<QString, QString>> &audioFiltersList);
    void videoFilters(const QList<QPair<QString, QString>> &videoFiltersList);

public slots:
    void takeActions(const QList<QAction*> actions);
    void takeSettings(QVariantMap payload);
    void takeKeyMap(const QVariantMap &payload);
    void setMouseMapDefaults(const QVariantMap &payload);
    void setAudioDevices(const QList<AudioDevice> &devices);
    void setAudioFilter(QString filter, QString options, bool add);
    void setVideoFilter(QString filter, QString options, bool add);
    void sendSignals();
    void sendAcceptedSettings();

    void setScreensaverDisablingEnabled(bool enabled);
    void setServerName(const QString &name);
    void setFreestanding(bool freestanding);

    void setZoomPreset(int which);
    void setHidePanels(bool hidden);

private slots:
    void setFilter(QList<QPair<QString, QString>> &filtersList, QString filter, QString options, bool add);
    void setCustomMpvOptions();
    void colorPick_clicked(QLineEdit *colorValue);
    void colorPick_changed(const QLineEdit *colorValue, QPushButton *colorPick);
    QList<MpvOption> parseMpvOptions(const QString &optionsInline) const;

    bool settingsOrKeyMapChanged();

    void on_pageTree_itemSelectionChanged();

    void on_buttonBox_clicked(QAbstractButton *button);

    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

    void on_playerOpenNew_toggled(bool checked);

    void on_playerAppendToQuickPlaylist_toggled(bool checked);

    void on_playerKeepHistory_toggled(bool checked);

    void on_interfaceIconsTheme_currentIndexChanged(int index);

    void on_interfaceWidgetCustom_toggled(bool checked);

    void on_interfaceIconsCustomBrowse_clicked();

    void on_ccHdrMapper_currentIndexChanged(int index);

    void on_videoDumbMode_toggled(bool checked);

    void on_logoExternalBrowse_clicked();

    void on_logoExternal_toggled(bool checked);

    void on_logoInternal_currentIndexChanged(int index);

    void on_videoPreset_currentIndexChanged(int index);

    void on_ccHdrCompute_currentIndexChanged(int index);

    void on_screenshotFormat_currentIndexChanged(int index);

    void on_jpgQuality_valueChanged(int value);

    void on_jpgSmooth_valueChanged(int value);

    void on_pngCompression_valueChanged(int value);

    void on_pngFilter_valueChanged(int value);

    void on_avifCrf_valueChanged(int value);

    void on_avifSpeed_valueChanged(int value);

    void on_avifLossless_clicked(bool checked);

    void on_keysReset_clicked();

    void on_shadersAddFile_clicked();

    void on_shadersRemoveFile_clicked();

    void on_shadersAddToShaders_clicked();

    void on_shadersActiveRemove_clicked();

    void on_shadersWikiAdd_clicked();

    void on_shadersWikiSync_clicked();

    void on_fullscreenHideControls_toggled(bool checked);

    void on_fullscreenShowWhen_currentIndexChanged(int index);;

    void on_audioBalance_valueChanged(int value);

    void on_playbackAutoZoom_toggled(bool checked);

    void on_playbackMouseHideFullscreen_toggled(bool checked);

    void on_playbackMouseHideWindowed_toggled(bool checked);

    void on_ccICCAutodetect_toggled(bool checked);

    void on_ccICCBrowse_clicked();

    void on_playbackPlayTimes_toggled(bool checked);

    void on_ditherDithering_toggled(bool checked);

    void on_ditherType_currentIndexChanged(int index);

    void on_ditherTemporal_toggled(bool checked);

    void on_audioSpdif_toggled(bool checked);

    void on_subsBackgroundBoxEnabled_toggled(bool checked);

    void on_subsShadowEnabled_toggled(bool checked);

    void on_screenshotDirectorySet_toggled(bool checked);

    void on_screenshotDirectoryBrowse_clicked();

    void on_tweaksPreferWayland_toggled(bool checked);

    void on_tweaksTimeTooltip_toggled(bool checked);

    void on_tweaksOsdFontChkBox_toggled(bool checked);

    void on_tweaksMpvOptionsChkBox_toggled(bool checked);

    void on_loggingEnabled_toggled(bool checked);

    void on_logFileCreate_toggled(bool checked);

    void on_logUpdateDelayed_toggled(bool checked);

    void on_logHistoryTrim_toggled(bool checked);

    void on_miscBrightness_valueChanged(int value);

    void on_miscContrast_valueChanged(int value);

    void on_miscGamma_valueChanged(int value);

    void on_miscSaturation_valueChanged(int value);

    void on_miscResetColor_clicked();

    void on_miscHue_valueChanged(int value);

    void on_miscResetSettings_clicked();

    void on_windowVideoValue_textChanged(const QString &arg1);

    void on_subtitlesAutoloadReset_clicked();

    void on_audioAutoloadPathReset_clicked();

    void on_webPortLink_linkActivated(const QString &link);

    void on_webRootBrowse_clicked();

    void on_logFilePathBrowse_clicked();

private:
    Ui::SettingsWindow *ui = nullptr;
    ActionEditor *actionEditor = nullptr;
    LogoWidget *logoWidget = nullptr;
    PaletteEditor *paletteEditor = nullptr;
    ScreenCombo *screenCombo = nullptr;
    SettingMap acceptedSettings;
    SettingMap defaultSettings;
    QList<SettingMap> videoPresets;
    QVariantMap acceptedKeyMap;
    QVariantMap defaultKeyMap;
    QList<AudioDevice> audioDevices;
    QList<QPair<QString, QString>> audioFiltersList;
    QList<QPair<QString, QString>> videoFiltersList;
};

#endif // SETTINGSWINDOW_H
