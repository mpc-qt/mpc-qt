#include "devicemanager_unix.h"

DeviceManagerUnix::DeviceManagerUnix(QObject *parent)
    : DeviceManager(parent)
{

}

bool DeviceManagerUnix::deviceAccessPossible()
{
    return false;
}

void DeviceManagerUnix::populate()
{

}
