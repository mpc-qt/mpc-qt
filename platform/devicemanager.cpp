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
    return devices.count();
}

bool DeviceManager::isDeviceValid(DeviceInfo *info)
{
    return info && devices.contains(info->uniqueId());
}

void DeviceManager::clearDevices()
{
    qDeleteAll(devices);
    devices.clear();
}

void DeviceManager::addDevice(DeviceInfo *device)
{
    devices.insert(device->uniqueId(), device);
    emit deviceAdded(device);
}

void DeviceManager::removeDevice(DeviceInfo *device)
{
    removeDeviceById(device->uniqueId());
}

void DeviceManager::removeDeviceById(int id)
{
    devices.remove(id);
}

void DeviceManager::iterateDevices(std::function<void(DeviceInfo *)> callback)
{
    for (auto i : devices) {
        callback(i);
    }
}
