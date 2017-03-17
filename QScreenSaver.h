#ifndef QSCREENSAVER_H
#define QSCREENSAVER_H

#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
#include "platform/unix_qscreensaver.h"
#elif defined(Q_OS_WIN)
#include "platform/win_qscreensaver.h"
#elif defined(Q_OS_MAC)
#include "platform/mac_qscreensaver.h"
#endif

#endif // QSCREENSAVER_H
