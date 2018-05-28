#include <QDebug>
#include "devicemanager.h"


DeviceInfo::DeviceInfo(QObject *parent)
    : QObject(parent)
{
    static int index = 0;
    uniqueId_ = index++;
}

int DeviceInfo::uniqueId()
{
    return uniqueId_;
}

DeviceManager::DeviceManager(QObject *parent) : QObject(parent)
{

}

int DeviceManager::count()
{
    return drives.count();
}

bool DeviceManager::checkDeviceValid(QSharedPointer<DeviceInfo> info)
{
    return drives.contains(info->uniqueId());
}

void DeviceManager::clearDevices()
{
    drives.clear();
}

void DeviceManager::addDevice(QSharedPointer<DeviceInfo> device)
{
    drives.insert(device->uniqueId(), device);
}

void DeviceManager::removeDevice(QSharedPointer<DeviceInfo> device)
{
    drives.remove(device->uniqueId());
}

void DeviceManager::iterateDevices(std::function<void(QSharedPointer<DeviceInfo>)> callback)
{
    for (auto &i : drives) {
        callback(i);
    }
}
