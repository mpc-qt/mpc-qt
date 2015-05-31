#include <clocale>
#include "main.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Qt sets the locale in the QApplication constructor, but libmpv requires
    // the LC_NUMERIC category to be set to "C", so change it back.
    std::setlocale(LC_NUMERIC, "C");
    Flow f;
    return f.run();
}

Flow::Flow(QObject *owner) :
    QObject(owner)
{
    mainWindow = new MainWindow();
    playbackManager = new PlaybackManager(this);
}

Flow::~Flow()
{
    delete playbackManager;
    delete mainWindow;
}

int Flow::run()
{
    mainWindow->show();
    return qApp->exec();
}
