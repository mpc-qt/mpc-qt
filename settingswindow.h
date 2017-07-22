#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QWidget>
#include <QTreeWidgetItem>
#include <QAbstractButton>
#include <QVariantMap>

#include <functional>

#include "helpers.h"

class QActionEditor;
class LogoWidget;
class QPushButton;

class Setting {
public:
    Setting() {}
    Setting(const Setting &s) : name(s.name), widget(s.widget), value(s.value) {}
    Setting(QString name, QWidget *widget, QVariant value) : name(name), widget(widget), value(value) {}

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

namespace Ui {
class SettingsWindow;
}

class SettingsWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget *parent = 0);
    ~SettingsWindow();

private:
    void setupColorPickers();
    void setupUnimplementedWidgets();
    void updateAcceptedSettings();
    SettingMap generateSettingMap(QWidget *root);
    void generateVideoPresets();
    void updateLogoWidget();
    QString selectedLogo();
    QString channelSwitcher();

signals:
    void settingsData(const QVariantMap &s);
    void keyMapData(const QVariantMap &s);
    void mouseWindowedMap(const MouseStateMap &map);
    void mouseFullscreenMap(const MouseStateMap &map);

    void trayIcon(bool yes);
    void showOsd(bool yes);
    void limitProportions(bool yes);
    void disableOpenDiscMenu(bool yes);
    void inhibitScreensaver(bool yes);
    void titleBarFormat(Helpers::TitlePrefix format);
    void titleUseMediaTitle(bool yes);
    void rememberHistory(bool yes);
    void rememberSelectedPlaylist(bool yes);
    void rememberWindowGeometry(bool yes);
    void rememberPanNScan(bool yes);

    void logoSource(const QString &s);
    void volume(int level);
    void balance(double pan);
    void volumeStep(int amount);
    void speedStep(double amount);
    void zoomPreset(int which, double fitFactor);
    void zoomCenter(bool yes);
    void mouseHideTimeFullscreen(int msec);
    void mouseHideTimeWindowed(int msec);
    void trackVideoPreference(const QString &pref);
    void trackAudioPreference(const QString &pref);
    void autoLoadAudio(bool yes);
    void autoLoadSubs(bool yes);

    void option(const QString &s, const QVariant &v);
    void fullscreenGeometry(const QRect &f);
    void fullscreenAtLaunch(bool yes);
    void fullscreenExitAtEnd(bool yes);

    void hideMethod(Helpers::ControlHiding method);
    void hideTime(int milliseconds);
    void hidePanels(bool yes);
    //void xrandrMap(const XrandrMap &map);
    void xrandrDelay(int seconds);
    void xrandrResetDefault(bool yes);
    void xrandrResetOnExit(bool yes);

    void playbackPlayTimes(int count);
    void playbackForever(bool yes);
    void playbackRewinds(bool yes);
    void playbackLoopImages(bool yes);
    void playlistFormat(const QString &fmt);

    // does mpv even *need* this?
    void subsPreferDefault(bool yes);
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
    void fallbackToFolder(bool yes);
    void timeTooltip(bool yes, bool above);
    void osdFont(const QString &family, const QString &size);

    // bchs should be part of a filter module page, hence the funny name
    void boschDishwasher(int brightness, int contrast, int hue, int saturation);
    void clientDebuggingMessages(bool yes);
    void mpvLogLevel(const QString &s);

    void videoFilter(const QString &s);
    void audioFilter(const QString &s);

public slots:
    void takeActions(const QList<QAction*> actions);
    void takeSettings(QVariantMap payload);
    void takeKeyMap(const QVariantMap &payload);
    void setMouseMapDefaults(const QVariantMap &payload);
    void setAudioDevices(const QList<AudioDevice> &devices);
    void sendSignals();

    void setScreensaverDisablingEnabled(bool enabled);
    void setServerName(const QString &name);

    void setVolume(int level);
    void setZoomPreset(int which);

private slots:
    void colorPick_clicked(QLineEdit *colorValue);
    void colorPick_changed(QLineEdit *colorValue, QPushButton *colorPick);

    void on_pageTree_itemSelectionChanged();

    void on_buttonBox_clicked(QAbstractButton *button);

    void on_ccHdrMapper_currentIndexChanged(int index);

    void on_videoDumbMode_toggled(bool checked);

    void on_logoExternalBrowse_clicked();

    void on_logoUseInternal_clicked();

    void on_logoExternal_clicked();

    void on_logoInternal_currentIndexChanged(int index);

    void on_videoPresetLoad_clicked();

    void on_screenshotFormat_currentIndexChanged(int index);

    void on_jpgQuality_valueChanged(int value);

    void on_jpgSmooth_valueChanged(int value);

    void on_pngCompression_valueChanged(int value);

    void on_pngFilter_valueChanged(int value);

    void on_keysReset_clicked();

    void on_shadersAddFile_clicked();

    void on_shadersRemoveFile_clicked();

    void on_shadersAddToShaders_clicked();

    void on_shadersActiveRemove_clicked();

    void on_shadersWikiAdd_clicked();

    void on_shadersWikiSync_clicked();

    void on_fullscreenHideControls_toggled(bool checked);

    void on_playbackAutoZoom_toggled(bool checked);

    void on_playbackMouseHideFullscreen_toggled(bool checked);

    void on_playbackMouseHideWindowed_toggled(bool checked);

private:
    Ui::SettingsWindow *ui = nullptr;
    QActionEditor *actionEditor = nullptr;
    LogoWidget *logoWidget = nullptr;
    SettingMap acceptedSettings;
    SettingMap defaultSettings;
    QList<SettingMap> videoPresets;
    QVariantMap acceptedKeyMap;
    QVariantMap defaultKeyMap;
    QList<AudioDevice> audioDevices;
};

#endif // SETTINGSWINDOW_H
