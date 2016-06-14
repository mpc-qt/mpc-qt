#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QWidget>
#include <QTreeWidgetItem>
#include <QAbstractButton>
#include <QVariantMap>
#include "helpers.h"

class QActionEditor;
class LogoWidget;

class Setting {
public:
    Setting() : name(), widget(NULL), value() {}
    Setting(const Setting &s) : name(s.name), widget(s.widget), value(s.value) {}
    Setting(QString name, QWidget *widget, QVariant value) : name(name), widget(widget), value(value) {}

    void sendToControl();
    void fetchFromControl();

    ~Setting() {}
    QString name;
    QWidget *widget;
    QVariant value;

    static QMap<QString,QString> classToProperty;
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
    void updateAcceptedSettings();
    SettingMap generateSettingMap();
    void updateLogoWidget();
    QString selectedLogo();

signals:
    void settingsData(const QVariantMap &s);
    void keyMapData(const QVariantMap &s);
    void mouseWindowedMap(const MouseStateMap &map);
    void mouseFullscreenMap(const MouseStateMap &map);

    void trayIcon(bool yes);
    void showOsd(bool yes);
    void limitProportions(bool yes);
    void disableOpenDiscMenu(bool yes);
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
    void travkVideoPreference(const QString &pref);
    void trackAudioPreference(const QString &pref);
    void autoLoadAudio(bool yes);
    void autoLoadSubs(bool yes);

    void voCommandLine(const QString &s);
    void aoCommandLine(const QString &s);
    void fullscreenGeometry(const QRect &f);
    void fullscreenAtLaunch(bool yes);
    void fullscreenExitAtEnd(bool yes);

    void hideControls(bool yes);
    void hideMethod(Helpers::ControlHiding method);
    void hideTime(int milliseconds);
    void hidePanels(bool yes);
    //void xrandrMap(const XrandrMap &map);
    void xrandrDelay(int seconds);
    void xrandrResetDefault(bool yes);
    void xrandrResetOnExit(bool yes);

    void preShaders(const QStringList &files);
    void postShaders(const QStringList &files);
    void userShaders(const QStringList &files);

    void framedropMode(const QString &s);
    void decoderDropMode(const QString &s);
    void displaySyncMode(const QString &s);
    void audioDropSize(double d);
    void maximumAudioChange(double d);
    void maximumVideoChange(double d);

    void playbackPlayTimes(int count);
    void playbackRewinds(bool yes);
    void playlistFormat(const QString &fmt);

    void subsAreGray(bool flag);
    void subsGetFixedTiming(bool flag);
    void subsClearOnSeek(bool flag);
    void assOverride(const QString &option);

    void subsFont(const QString &family);
    void subsBold(bool yes);
    void subsItalic(bool yes);
    void subsSize(int size);
    void subsBorderSize(int size);
    void subsShadowSize(int size);
    void subsColor(const QColor &color);
    void subsBorderColor(const QColor &border);
    void subsShadowColor(const QColor &shadow);
    void subsMarginX(int x);
    void subsMarginY(int y);
    void subsWeight(int x, int y);      // -1:top/left 0:center 1:bottom/right

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
    void screenshotJpegQuality(int64_t value);
    void screenshotJpegSmooth(int64_t value);
    void screenshotJpegSourceChroma(bool yes);
    void screenshotPngCompression(int64_t value);
    void screenshotPngFilter(int64_t value);
    void screenshotPngColorspace(bool yes);
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

    void fastSeek(bool toKeyframes);
    void chapterMarks(bool yes);
    void fallbackToFolder(bool yes);
    void timeTooptip(bool yes, bool above);
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
    void sendSignals();
    void setNnedi3Unavailable();

    void setServerName(const QString &name);

    void setVolume(int level);
    void setZoomPreset(int which);

private slots:
    void on_pageTree_itemSelectionChanged();

    void on_buttonBox_clicked(QAbstractButton *button);

    void on_prescalarMethod_currentIndexChanged(int index);

    void on_audioRenderer_currentIndexChanged(int index);

    void on_videoDumbMode_toggled(bool checked);

    void on_logoExternalBrowse_clicked();

    void on_logoUseInternal_clicked();

    void on_logoExternal_clicked();

    void on_logoInternal_currentIndexChanged(int index);

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

private:
    Ui::SettingsWindow *ui;
    QActionEditor *actionEditor;
    LogoWidget *logoWidget;
    SettingMap acceptedSettings;
    SettingMap defaultSettings;
    QVariantMap acceptedKeyMap;
    QVariantMap defaultKeyMap;
    bool parseNnedi3Fields;
};

#endif // SETTINGSWINDOW_H
