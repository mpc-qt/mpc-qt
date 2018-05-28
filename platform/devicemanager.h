#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QMap>
#include <QSharedPointer>
#include <functional>

class DeviceInfo : public QObject
{
    Q_OBJECT

public:
    enum DeviceType { UnknownDrive, NoDrive, RemovableDrive, FixedDrive,
                      RemoteDrive, OpticalDrive, RamDrive, MemoryDevice,
                      OtherDevice = -1 };

    DeviceInfo(QObject *parent = nullptr);
    int uniqueId();

    DeviceType deviceType;
    QString deviceName;
    QString internal;
    QString volumeLabel;
    QString mountedPath;

private:
    int uniqueId_;
};


class DeviceManager : public QObject
{
    Q_OBJECT
public:
    explicit DeviceManager(QObject *parent = nullptr);
    virtual bool deviceAccessPossible() = 0;
    int count();
    void iterateDevices(std::function<void(QSharedPointer<DeviceInfo>)> callback);
    bool checkDeviceValid(QSharedPointer<DeviceInfo> info);

signals:
    void deviceAdded(QSharedPointer<DeviceInfo> info);
    void deviceRemoved(QSharedPointer<DeviceInfo> info);

public slots:

protected:
    virtual void populate() = 0;
    void clearDevices();
    void addDevice(QSharedPointer<DeviceInfo> device);
    void removeDevice(QSharedPointer<DeviceInfo> device);

private:
    QMap<int, QSharedPointer<DeviceInfo>> drives;
};

#endif // DEVICEMANAGER_H
