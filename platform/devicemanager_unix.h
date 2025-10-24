#ifndef DEVICEMANAGER_UNIX_H
#define DEVICEMANAGER_UNIX_H

#include <QtCore>
#include <QtDBus>
#include <QDBusObjectPath>
#include "devicemanager.h"

class UDisks2Block;
class UDisks2Drive;
class UDisks2Filesystem;

class DeviceManagerUnix : public DeviceManager
{
    Q_OBJECT
public:
    explicit DeviceManagerUnix(QObject *parent = nullptr);
    ~DeviceManagerUnix();
    bool deviceAccessPossible() override;

    QStringList blockDevices();
    UDisks2Block *blockDevice(const QString &node);

    QStringList drives();
    UDisks2Drive *drive(const QString &node);

signals:
    void deviceInformationChanged(QString node, QVariantMap info);
    void driveAdded(const QString& node);
    void driveRemoved(const QString& node);
    void driveChanged(const QString& node);
    void blockDeviceAdded(const QString& node);
    void blockDeviceRemoved(const QString &node);
    void blockDeviceChanged(const QString &node);
    void filesystemAdded(const QString& node);
    void filesystemRemoved(const QString &node);
    void filesystemChanged(const QString &node);

protected:
    void populate() override;

private:
    void addDrive(const QString &node);
    void addBlock(const QString &node);
    void removeDrive(const QString &node);
    void removeBlock(const QString &node);

private slots:
    void dbus_interfaceAdded(const QDBusObjectPath &path, const QMap<QString, QVariant> &interfaces);
    void dbus_interfaceRemoved(const QDBusObjectPath &path, const QStringList &interfaces);

private:
    QMap<QString,UDisks2Drive*> drives_;
    QMap<QString,UDisks2Block*> blocks_;
    QHash<QString,int> deviceIdByBlockNode;
    bool serviceConnected;
};



class UDisks2Block : public DeviceInfo {
    Q_OBJECT
public:
    explicit UDisks2Block(const QString &node, QObject *parent = nullptr);
    QString toDisplayString() override;
    void mount() override;

    //QString name;       // deviceName
    QString dev;
    QString id;
    //QString drive;      // internal
    qulonglong size;
    bool readonly;
    QString usage;
    QString type;
    //QString label;      // volumeLabel
    QString toString();

    void update();
    void updateFilesystem();
    void addFilesystem();
    void removeFilesystem();
    UDisks2Filesystem *fileSystem();

signals:
    void filesystemAdded(const QString& node);
    void filesystemRemoved(const QString &node);
    void filesystemChanged(const QString &node);
    void changed(const QString &node);

private slots:
    void self_propertiesChanged(const QString &interface, const QVariantMap &changedProp, const QStringList &invalidatedProp);

private:
    QDBusInterface *dbus;
    UDisks2Filesystem* fs;
};

class UDisks2Filesystem : public QObject {
    Q_OBJECT
public:
    explicit UDisks2Filesystem(const QString &node, QObject *parent = nullptr);
    const QStringList &mountPoints() const;
    void mount();
    void unmount();
    void update();
    bool isValid();

    QString name;

private:
    QDBusInterface *dbus;
    QDBusInterface *dbusProp;
    QStringList mountPoints_;
};

class UDisks2Drive : public QObject {
    Q_OBJECT
public:
    explicit UDisks2Drive(const QString &node, QObject *parent = nullptr);

    QString name;
    qulonglong size;
    QString vendor;
    QString model;
    QString serial;
    QString id;
    QString media;
    bool optical;
    bool removable;
    bool available;
    QString toString();

    void update();

signals:
    void changed(const QString &node);

private slots:
    void self_propertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &invalidated);

private:
    QDBusInterface *dbus;
};

#endif // DEVICEMANAGER_UNIX_H
