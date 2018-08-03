#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QMap>
#include <QSharedPointer>
#include <QTimer>
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
    virtual QString toDisplayString() = 0;
    virtual void mount() = 0;

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
    ~DeviceManager();
    virtual bool deviceAccessPossible() = 0;
    int count();
    void iterateDevices(std::function<void(DeviceInfo*)> callback);
    bool isDeviceValid(DeviceInfo *info);

signals:
    void deviceAdded(DeviceInfo *info);
    void deviceRemoved(DeviceInfo *info);
    void deviceListChanged();

public slots:

protected:
    virtual void populate() = 0;
    void clearDevices();
    void addDevice(DeviceInfo *device);
    void removeDevice(DeviceInfo *device);
    void removeDeviceById(int id);

private:
    QMap<int,DeviceInfo*> devices;
    QTimer changedTimer;
};

#endif // DEVICEMANAGER_H
