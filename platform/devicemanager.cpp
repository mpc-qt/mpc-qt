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
    changedTimer.setSingleShot(true);
    changedTimer.setInterval(0);
    connect(&changedTimer, &QTimer::timeout,
            this, &DeviceManager::deviceListChanged);
}

DeviceManager::~DeviceManager()
{
    clearDevices();
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
    changedTimer.start();
}

void DeviceManager::removeDevice(DeviceInfo *device)
{
    removeDeviceById(device->uniqueId());
}

void DeviceManager::removeDeviceById(int id)
{
    if (!devices.contains(id))
        return;
    emit deviceRemoved(devices[id]);
    devices.remove(id);
    changedTimer.start();
}

void DeviceManager::iterateDevices(std::function<void(DeviceInfo *)> callback)
{
    for (auto i : std::as_const(devices)) {
        callback(i);
    }
}
