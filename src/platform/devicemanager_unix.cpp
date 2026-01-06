#include "logger.h"
#include "devicemanager_unix.h"

using dbus_ay = QList<unsigned char>;
using QVariantMapMap = QMap<QString, QVariantMap>;
using DBusManagedObjects = QMap<QDBusObjectPath, QVariantMapMap>;
using ByteArrayList = QList<QByteArray>;
Q_DECLARE_METATYPE(dbus_ay)
Q_DECLARE_METATYPE(QVariantMapMap)
Q_DECLARE_METATYPE(DBusManagedObjects)
Q_DECLARE_METATYPE(ByteArrayList)

static QString dbus_ay_toString(const dbus_ay &data);
static QString lastPart(const QString &path);

static constexpr char logModule[] =  "devman";

static const QString DBUS_SERVICE_NAME      ("org.freedesktop.UDisks2");
static const QString DBUS_IFACE_MANAGER     ("org.freedesktop.DBus.ObjectManager");
static const QString DBUS_IFACE_INTROSPECT  ("org.freedesktop.DBus.Introspectable");
static const QString DBUS_IFACE_BLOCK       ("org.freedesktop.UDisks2.Block");
static const QString DBUS_IFACE_FILESYSTEM  ("org.freedesktop.UDisks2.Filesystem");
static const QString DBUS_IFACE_DRIVE       ("org.freedesktop.UDisks2.Drive");
static const QString DBUS_IFACE_PROPERTIES  ("org.freedesktop.DBus.Properties");

static const QString DBUS_OBJECT_ROOT   ("/org/freedesktop/UDisks2");
static const QString DBUS_BLOCKS_ROOT   ("/org/freedesktop/UDisks2/block_devices");
static const QString DBUS_DRIVES_ROOT   ("/org/freedesktop/UDisks2/drives");

static const QString DBUS_CMD_GET           ("Get");
static const QString DBUS_CMD_MOUNT         ("Mount");
static const QString DBUS_CMD_UNMOUNT       ("Unmount");
static const QString DBUS_CMD_MOUNTPOINTS   ("MountPoints");

static const QString DBUS_IFACE_ADDED   ("InterfacesAdded");
static const QString DBUS_IFACE_REMOVED ("InterfacesRemoved");
static const QString DBUS_INTROSPECT    ("Introspect");
static const QString DBUS_PROP_CHANGED  ("PropertiesChanged");


DeviceManagerUnix::DeviceManagerUnix(QObject *parent)
    : DeviceManager(parent)
{
    qDBusRegisterMetaType<QVariantMapMap>();
    qDBusRegisterMetaType<DBusManagedObjects>();
    qDBusRegisterMetaType<ByteArrayList>();
    //qDBusRegisterMetaType<ByteArray>();

    // Check for presence of udisks2
    auto system = QDBusConnection::systemBus();
    serviceConnected = system.isConnected()
            && system.interface()->isServiceRegistered(DBUS_SERVICE_NAME);
    if (!serviceConnected)
        return;

    // Listen to interface changes
    system.connect(DBUS_SERVICE_NAME, DBUS_OBJECT_ROOT,
                   DBUS_IFACE_MANAGER, DBUS_IFACE_ADDED,
                   this, SLOT(dbus_interfaceAdded(QDBusObjectPath,QMap<QString,QVariant>)));
    system.connect(DBUS_SERVICE_NAME, DBUS_OBJECT_ROOT,
                   DBUS_IFACE_MANAGER, DBUS_IFACE_REMOVED,
                   this, SLOT(dbus_interfaceRemoved(QDBusObjectPath,QStringList)));

    // Populate our data fields
    foreach (QString drive, drives()) {
        addDrive(drive);
    }
    foreach (QString block, blockDevices()) {
        addBlock(block);
    }
}

DeviceManagerUnix::~DeviceManagerUnix()
{

}

bool DeviceManagerUnix::deviceAccessPossible()
{
    return serviceConnected;
}

void DeviceManagerUnix::populate()
{
    // managed automagically
}

QStringList DeviceManagerUnix::blockDevices()
{
    QDBusInterface ud2(DBUS_SERVICE_NAME,
                       DBUS_BLOCKS_ROOT,
                       DBUS_IFACE_INTROSPECT,
                       QDBusConnection::systemBus());
    QDBusReply<QString> reply = ud2.call(DBUS_INTROSPECT);
    if (!reply.isValid())
        return QStringList();
    QXmlStreamReader xml(reply.value());
    QStringList response;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.tokenType() == QXmlStreamReader::StartElement && xml.name().toString() == "node") {
            QString name = xml.attributes().value("name").toString();
            if (!name.isEmpty())
                response << name;
        }
    }
    return response;
}

UDisks2Block *DeviceManagerUnix::blockDevice(const QString &node)
{
    return blocks_.contains(node) ? blocks_[node] : nullptr;
}

QStringList DeviceManagerUnix::drives()
{
    QDBusInterface ud2(DBUS_SERVICE_NAME,
                       DBUS_DRIVES_ROOT,
                       DBUS_IFACE_INTROSPECT,
                       QDBusConnection::systemBus());
    if (!ud2.isValid())
        return QStringList();
    QDBusReply<QString> reply = ud2.call(DBUS_INTROSPECT);
    if (!reply.isValid())
        return QStringList();
    QXmlStreamReader xml(reply.value());
    QStringList response;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.tokenType() == QXmlStreamReader::StartElement && xml.name().toString() == "node") {
            QString name = xml.attributes().value("name").toString();
            if (!name.isEmpty())
                response << name;
        }
    }
    return response;
}

UDisks2Drive *DeviceManagerUnix::drive(const QString &node)
{
    return drives_.contains(node) ? drives_[node] : nullptr;
}

void DeviceManagerUnix::addDrive(const QString &node)
{
    if (drives_.contains(node))
        drives_.value(node)->update();
    else {
        auto drive = new UDisks2Drive(node, this);
        connect(drive, &UDisks2Drive::changed,
                this, &DeviceManagerUnix::driveChanged);
        drives_.insert(node, drive);
        Logger::logs(logModule, {"adding drive", node});
        emit driveAdded(node);
    }
}

void DeviceManagerUnix::addBlock(const QString &node)
{
    if (blocks_.contains(node))
        blocks_.value(node)->update();
    else {
        auto block = new UDisks2Block(node, this);
        connect(block, &UDisks2Block::filesystemAdded,
                this, &DeviceManagerUnix::filesystemAdded);
        connect(block, &UDisks2Block::filesystemRemoved,
                this, &DeviceManagerUnix::filesystemRemoved);
        connect(block, &UDisks2Block::filesystemChanged,
                this, &DeviceManagerUnix::filesystemChanged);
        connect(block, &UDisks2Block::changed,
                this, &DeviceManagerUnix::blockDeviceChanged);
        blocks_.insert(node, block);
        deviceIdByBlockNode.insert(node, block->uniqueId());
        addDevice(block);
        emit blockDeviceAdded(node);
    }
}

void DeviceManagerUnix::removeDrive(const QString &node)
{
    if (drives_.contains(node))
        delete drives_.take(node);
}

void DeviceManagerUnix::removeBlock(const QString &node)
{
    if (deviceIdByBlockNode.contains(node)) {
        int id = deviceIdByBlockNode.value(node);
        removeDeviceById(id);
        deviceIdByBlockNode.remove(node);
    }
    if (blocks_.contains(node))
        delete blocks_.take(node);
}

void DeviceManagerUnix::dbus_interfaceAdded(const QDBusObjectPath &path, const QMap<QString, QVariant> &interfaces)
{
    // path: o [path]
    // nodes: a{sa{sv} [dict of strings dict of string variants]
    QString node = lastPart(path.path());
    if (path.path().startsWith(DBUS_BLOCKS_ROOT)) {
        if (interfaces.contains(DBUS_IFACE_BLOCK))
            addBlock(node);
        if (interfaces.contains(DBUS_IFACE_FILESYSTEM)
                && blocks_.contains(node))
            blocks_[node]->addFilesystem();
    } else if (path.path().startsWith(DBUS_DRIVES_ROOT)) {
        if (interfaces.contains(DBUS_IFACE_DRIVE))
            addDrive(node);
    }
}

void DeviceManagerUnix::dbus_interfaceRemoved(const QDBusObjectPath &path, const QStringList &interfaces)
{
    // path: o [path]
    // interfaces: as [list of strings]
    QString node = lastPart(path.path());
    if (path.path().startsWith(DBUS_BLOCKS_ROOT)) {
        if (interfaces.contains(DBUS_IFACE_BLOCK)) {
            removeBlock(node);
            emit blockDeviceRemoved(node);
        }
        if (interfaces.contains(DBUS_IFACE_FILESYSTEM)
                && blocks_.contains(node)) {
            blocks_[node]->removeFilesystem();
        }
    } else if (path.path().startsWith(DBUS_DRIVES_ROOT)) {
        removeDrive(node);
        emit driveRemoved(node);
    }
}



UDisks2Block::UDisks2Block(const QString &node, QObject *parent) :
    DeviceInfo(parent), fs(nullptr)
{
    deviceName = node;
    QDBusConnection system = QDBusConnection::systemBus();
    dbus = new QDBusInterface(DBUS_SERVICE_NAME,
                              DBUS_BLOCKS_ROOT + "/" + node,
                              DBUS_IFACE_BLOCK,
                              system, parent);
    system.connect(dbus->service(), dbus->path(),
                   DBUS_IFACE_PROPERTIES, DBUS_PROP_CHANGED,
                   this, SLOT(self_propertiesChanged(QString,QVariantMap,QStringList)));
    update();
    if (!type.isEmpty())
        addFilesystem();
}

QString UDisks2Block::toDisplayString()
{
    QStringList l;
    l << deviceName;
    if (!volumeLabel.isEmpty())
        l << "[" + volumeLabel + "]";
    l << ": " + internal;
    return l.join(' ');
}

void UDisks2Block::mount()
{
    if (!fs)
        return;
    if (mountedPath.isEmpty()) {
        fs->mount();
        if (!fs->mountPoints().isEmpty())
            mountedPath = fs->mountPoints().first();
    }
    Logger::logs(logModule, {"device mount", mountedPath});
}

QString UDisks2Block::toString()
{
    return QString("name: %1\ndev: %2\nid: %3\ndrive: %4\nsize: %5\n"
                   "readonly: %6\nusage: %7\ntype: %8\nlabel: %9")
            .arg(deviceName, dev, id, internal, QString::number(size),
                 QString::number(int(readonly)), usage, type, volumeLabel);
}

void UDisks2Block::update()
{
    dev = dbus->property("Device").toString();
    id = dbus->property("Id").toString();
    internal = lastPart(dbus->property("Drive").value<QDBusObjectPath>().path());
    size = dbus->property("Size").toULongLong();
    readonly = dbus->property("ReadOnly").toBool();
    usage = dbus->property("IdUsage").toString();
    type = dbus->property("IdType").toString();
    volumeLabel = dbus->property("IdLabel").toString();

    auto devices = qobject_cast<DeviceManagerUnix*>(this->parent());
    if (devices) {
        auto drive = devices->drive(internal);
        deviceType = NoDrive;
        if (drive) {
            if (drive->media.contains("optical") || drive->optical)
                deviceType = OpticalDrive;
            else if (usage.isEmpty())
                deviceType = FixedDrive; // sometimes the hard disk itself
            else if (drive->media.contains("thumb") || drive->removable)
                deviceType = RemovableDrive;
            else
                deviceType = OtherDevice;

        }
    }
}

void UDisks2Block::updateFilesystem()
{
    if (fs) {
        fs->update();
        if (!fs->mountPoints().isEmpty())
            mountedPath = fs->mountPoints().at(0);
        else
            mountedPath.clear();
    }

}

void UDisks2Block::addFilesystem()
{
    if (fs && !fs->isValid()) {
        delete fs;
        fs = nullptr;
    }
    if (!fs)
        fs = new UDisks2Filesystem(deviceName);
    if (!fs->mountPoints().isEmpty())
        mountedPath = fs->mountPoints().at(0);
    else
        mountedPath.clear();
    emit filesystemAdded(deviceName);
}

void UDisks2Block::removeFilesystem()
{
    emit filesystemRemoved(deviceName);
    if (fs) delete fs;
    fs = nullptr;
    mountedPath.clear();
}

UDisks2Filesystem *UDisks2Block::fileSystem()
{
    return fs;
}

void UDisks2Block::self_propertiesChanged(const QString &interface, const QVariantMap &changedProp, const QStringList &invalidatedProp)
{
    Q_UNUSED(changedProp)
    Q_UNUSED(invalidatedProp)
    if (interface == DBUS_IFACE_BLOCK) {
        update();
        emit changed(deviceName);
    }
    if (interface == DBUS_IFACE_FILESYSTEM) {
        updateFilesystem();
        emit filesystemChanged(deviceName);
    }
}


UDisks2Drive::UDisks2Drive(const QString &node, QObject *parent) :
    QObject(parent), name(node)
{
    QDBusConnection system = QDBusConnection::systemBus();
    dbus = new QDBusInterface(DBUS_SERVICE_NAME,
                              DBUS_DRIVES_ROOT + "/" + node,
                              DBUS_IFACE_DRIVE,
                              system, parent);
    system.connect(dbus->service(), dbus->path(),
                   DBUS_IFACE_PROPERTIES, DBUS_PROP_CHANGED,
                   this, SLOT(self_propertiesChanged(QString,QVariantMap,QStringList)));
    update();
}

QString UDisks2Drive::toString()
{
    return QString("name: %1\nsize: %2\nvendor: %3\nmodel: %4\nserial: %5\n").arg(name, QString::number(size), vendor, model, serial)
            + QString("id: %6\nmedia: %7\noptical: %8\nremovable: %9\navailable: %10").arg(id, media, QString::number(int(optical)), QString::number(int(removable)), QString::number(int(available)));
}

void UDisks2Drive::update()
{
    size = dbus->property("Size").toULongLong();
    vendor = dbus->property("Vendor").toString();
    model = dbus->property("Model").toString();
    serial = dbus->property("Serial").toString();
    id = dbus->property("Id").toString();
    media = dbus->property("Media").toString();
    optical = dbus->property("Optical").toBool();
    removable = dbus->property("MediaRemovable").toBool();
    available = dbus->property("MediaAvailable").toBool();
}

void UDisks2Drive::self_propertiesChanged(const QString &interface, const QVariantMap &changedProp, const QStringList &invalidatedProp)
{
    Q_UNUSED(interface)
    Q_UNUSED(changedProp)
    Q_UNUSED(invalidatedProp)
    update();
    emit this->changed(name);
}



UDisks2Filesystem::UDisks2Filesystem(const QString &node, QObject *parent)
    : QObject(parent)
{
    name = node;
    QDBusConnection system = QDBusConnection::systemBus();
    dbus = new QDBusInterface(DBUS_SERVICE_NAME,
                              DBUS_BLOCKS_ROOT + "/" + node,
                              DBUS_IFACE_FILESYSTEM,
                              system, parent);
    dbusProp = new QDBusInterface(DBUS_SERVICE_NAME,
                              DBUS_BLOCKS_ROOT + "/" + node,
                              DBUS_IFACE_PROPERTIES,
                              system, parent);
    update();
}

const QStringList &UDisks2Filesystem::mountPoints() const { return mountPoints_; }

void UDisks2Filesystem::mount()
{
    QDBusMessage reply = dbus->call(DBUS_CMD_MOUNT, QVariantMap());
    if (reply.type() == QDBusMessage::ErrorMessage)
        Logger::logs(logModule, {"mount failed with message", reply.errorMessage()});
    update();
}

void UDisks2Filesystem::unmount()
{   dbus->call(DBUS_CMD_UNMOUNT, QVariantMap());

}

void UDisks2Filesystem::update()
{
    mountPoints_.clear();
    QDBusMessage reply = dbusProp->call(DBUS_CMD_GET, DBUS_IFACE_FILESYSTEM, DBUS_CMD_MOUNTPOINTS);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        // No such interface, the device had no filesystem
        return;
    }
    QVariant v = reply.arguments().at(0);
    QDBusArgument arg = v.value<QDBusVariant>().variant().value<QDBusArgument>();
    arg.beginArray();
    while (!arg.atEnd()) {
        dbus_ay data;
        arg >> data;
        mountPoints_.append(dbus_ay_toString(data));
    }
}

bool UDisks2Filesystem::isValid()
{
    return dbus->isValid();
}



static QString lastPart(const QString &path) {
    return path.split('/').last();
}

static QString dbus_ay_toString(const dbus_ay &data) {
    QByteArray b;
    for (unsigned char c : data)
        b.append(c);
    return QString::fromLocal8Bit(b).remove(QChar::Null);
}
