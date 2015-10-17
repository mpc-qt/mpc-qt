#ifndef MAIN_H
#define MAIN_H
#include "helpers.h"
#include "mainwindow.h"
#include "manager.h"
#include "storage.h"

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
    void process_payloadRecieved(const QStringList &payload);
    void importPlaylist(QString fname);
    void exportPlaylist(QString fname, QStringList items);

private:
    SingleProcess *process;
    MainWindow *mainWindow;
    PlaybackManager *playbackManager;
    Storage storage;

    bool hasPrevious_;
};

#endif // MAIN_H
