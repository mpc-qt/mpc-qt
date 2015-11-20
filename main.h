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
    void mainwindow_optionsOpenRequested();
    void process_payloadRecieved(const QStringList &payload);
    void settingswindow_settingsData(const Settings &s);
    void importPlaylist(QString fname);
    void exportPlaylist(QString fname, QStringList items);

private:
    SingleProcess *process;
    MainWindow *mainWindow;
    PlaybackManager *playbackManager;
    SettingsWindow *settingsWindow;
    Storage storage;
    Settings s;

    bool hasPrevious_;
};

#endif // MAIN_H
