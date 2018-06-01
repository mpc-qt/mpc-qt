#ifndef DEVICEMANAGERWIN_H
#define DEVICEMANAGERWIN_H

#include <windows.h>
#include "devicemanager.h"

class DeviceManagerWin : public DeviceManager
{
    Q_OBJECT
public:
    DeviceManagerWin(QObject *parent);
    bool deviceAccessPossible();

protected:
    void populate();

private:
    QList<DeviceInfo*> volumePaths(WCHAR *volumeName, WCHAR *deviceName);
};


class DeviceInfoWin : public DeviceInfo
{
    Q_OBJECT
public:
    DeviceInfoWin(QObject *parent = nullptr);
    QString toDisplayString();
    void mount();
};


#endif // DEVICEMANAGERWIN_H
