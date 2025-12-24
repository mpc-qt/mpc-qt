#include "screensaver.h"

ScreenSaver::ScreenSaver(QObject *parent)
    : QObject(parent)
{
}

bool ScreenSaver::inhibiting()
{
    return isInhibiting;
}
