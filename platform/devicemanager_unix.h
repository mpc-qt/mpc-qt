#ifndef DEVICEMANAGER_UNIX_H
#define DEVICEMANAGER_UNIX_H

#include "devicemanager.h"

class DeviceManagerUnix : public DeviceManager
{
    Q_OBJECT
public:
    DeviceManagerUnix(QObject *parent);
    bool deviceAccessPossible();

protected:
    void populate();
};

#endif // DEVICEMANAGER_UNIX_H
