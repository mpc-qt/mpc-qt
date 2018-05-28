#include "devicemanager_mac.h"

DeviceManagerMac::DeviceManagerMac(QObject *parent)
    : DeviceManager(parent)
{

}

bool DeviceManagerMac::deviceAccessPossible()
{
    return false;
}

void DeviceManagerMac::populate()
{

}
