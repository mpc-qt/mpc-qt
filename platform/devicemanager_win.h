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


class DeviceInfoWin : public DeviceInfo
{
    Q_OBJECT
public:
    DeviceInfoWin(QObject *parent = nullptr);
    QString toDisplayString();
};


#endif // DEVICEMANAGERWIN_H
