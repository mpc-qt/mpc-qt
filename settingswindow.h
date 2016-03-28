#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QWidget>
#include <QTreeWidgetItem>
#include <QAbstractButton>
#include <QVariantMap>

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

    void logoSource(const QString &s);
    void voCommandLine(const QString &s);
    void framedropMode(const QString &s);
    void decoderDropMode(const QString &s);
    void displaySyncMode(const QString &s);
    void audioDropSize(double d);
    void maximumAudioChange(double d);
    void maximumVideoChange(double d);
    void subsAreGray(bool flag);
    void screenshotDirectory(const QString &s);
    void encodeDirectory(const QString &s);
    void screenshotTemplate(const QString &s);
    void encodeTemplate(const QString &s);
    void screenshotFormat(const QString &s);
    void clientDebuggingMessages(bool yes);
    void mpvLogLevel(const QString &s);

public slots:
    void takeSettings(QVariantMap payload);
    void sendSignals();    
    void setNnedi3Available(bool yes);

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

private:
    Ui::SettingsWindow *ui;
    LogoWidget *logoWidget;
    SettingMap acceptedSettings;
    SettingMap defaultSettings;
    bool parseNnedi3Fields;
};

#endif // SETTINGSWINDOW_H
