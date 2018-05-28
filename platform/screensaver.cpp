#include "screensaver.h"

QScreenSaver::QScreenSaver(QObject *parent)
    : QObject(parent)
{
}

bool QScreenSaver::inhibiting()
{
    return isInhibiting;
}
