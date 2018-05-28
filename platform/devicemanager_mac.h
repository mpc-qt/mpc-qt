#ifndef DEVICEMANAGER_MAC_H
#define DEVICEMANAGER_MAC_H

#include "devicemanager.h"

class DeviceManagerMac : public DeviceManager
{
    Q_OBJECT
public:
    DeviceManagerMac(QObject *parent);
    bool deviceAccessPossible();

protected:
    void populate();
};

#endif // DEVICEMANAGER_MAC_H
