#ifndef QSCREENSAVER_H
#define QSCREENSAVER_H

#include <qglobal.h>

#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
#include "platform/qscreensaver_unix.h"
#elif defined(Q_OS_WIN)
#include "platform/qscreensaver_win.h"
#elif defined(Q_OS_MAC)
#include "platform/qscreensaver_mac.h"
#endif

#endif // QSCREENSAVER_H
