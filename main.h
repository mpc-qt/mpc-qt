#ifndef MAIN_H
#define MAIN_H
#include "mainwindow.h"
#include "manager.h"

// a simple class to control program exection and own application objects
class Flow : public QObject {
    Q_OBJECT
public:
    explicit Flow(QObject *owner = 0);
    ~Flow();

    int run();

private:
    MainWindow *mainWindow;
    PlaybackManager *playbackManager;
};

#endif // MAIN_H
