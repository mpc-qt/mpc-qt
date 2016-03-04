#ifndef MAIN_H
#define MAIN_H
#include "helpers.h"
#include "mainwindow.h"
#include "manager.h"
#include "storage.h"
#include "settingswindow.h"

// a simple class to control program exection and own application objects
class Flow : public QObject {
    Q_OBJECT
public:
    explicit Flow(QObject *owner = 0);
    ~Flow();

    int run();
    bool hasPrevious();

private:
    QStringList makePayload() const;

private slots:
    void mainwindow_applicationShouldQuit();
    void mainwindow_takeImage();
    void mainwindow_takeImageAutomatically();
    void mainwindow_optionsOpenRequested();
    void process_payloadRecieved(const QStringList &payload);
    void settingswindow_settingsData(const QVariantMap &settings);
    void settingswindow_screenshotDirectory(const QString &where);
    void settingswindow_encodeDirectory(const QString &where);
    void settingswindow_screenshotTemplate(const QString &fmt);
    void settingswindow_encodeTemplate(const QString &fmt);
    void importPlaylist(QString fname);
    void exportPlaylist(QString fname, QStringList items);

private:
    SingleProcess *process;
    MainWindow *mainWindow;
    PlaybackManager *playbackManager;
    SettingsWindow *settingsWindow;
    Storage storage;
    QVariantMap settings;

    QString screenshotDirectory;
    QString encodeDirectory;
    QString screenshotTemplate;
    QString encodeTemplate;
    bool hasPrevious_;
};

#endif // MAIN_H
