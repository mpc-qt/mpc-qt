#include "qabstractscreensaver.h"

QAbstractScreenSaver::QAbstractScreenSaver(QObject *parent)
    : QObject(parent), isInhibiting(false)
{
}

bool QAbstractScreenSaver::inhibiting()
{
    return isInhibiting;
}
