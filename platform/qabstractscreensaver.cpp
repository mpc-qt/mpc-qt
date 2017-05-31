#include "qabstractscreensaver.h"

QAbstractScreenSaver::QAbstractScreenSaver(QObject *parent)
    : QObject(parent)
{
}

bool QAbstractScreenSaver::inhibiting()
{
    return isInhibiting;
}
