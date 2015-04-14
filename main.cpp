#include "hostwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    HostWindow w;
    w.show();

    return a.exec();
}
