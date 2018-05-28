#ifndef DEVICEMANAGERWIN_H
#define DEVICEMANAGERWIN_H

#include "devicemanager.h"

class DeviceManagerWin : public DeviceManager
{
    Q_OBJECT
public:
    DeviceManagerWin(QObject *parent);
    bool deviceAccessPossible();

protected:
    void populate();
};

#endif // DEVICEMANAGERWIN_H
