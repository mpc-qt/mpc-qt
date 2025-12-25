#include <QString>
#include "logger.h"
#include "devicemanager_win.h"

static constexpr char logModule[] =  "devman";

DeviceManagerWin::DeviceManagerWin(QObject *parent)
    : DeviceManager(parent)
{
    populate();
    listener = new DeviceListener;
    listener->winId();
    connect(listener, &DeviceListener::rescanNeeded,
            this, &DeviceManagerWin::rescan);
}

DeviceManagerWin::~DeviceManagerWin()
{
    delete listener;
}

bool DeviceManagerWin::deviceAccessPossible()
{
    return true;
}

void DeviceManagerWin::rescan()
{
    populate();
}

void DeviceManagerWin::populate()
{
    WCHAR deviceName[MAX_PATH] = L"";
    WCHAR volumeName[MAX_PATH] = L"";
    HANDLE findHandle;
    DWORD charCount;
    size_t slashIndex;

    clearDevices();
    findHandle = FindFirstVolumeW(volumeName, ARRAYSIZE(volumeName));
    if (findHandle == INVALID_HANDLE_VALUE) {
        return;
    }
    do {
        if (wcsncmp(volumeName, L"\\\\?\\", 4))
            break;
        slashIndex = wcslen(volumeName) - 1;
        volumeName[slashIndex] = 0;
        charCount = QueryDosDeviceW(volumeName+4, deviceName, ARRAYSIZE(deviceName));
        volumeName[slashIndex] = L'\\';
        if (charCount == 0)
            break;
        for (auto i : volumePaths(volumeName, deviceName))
            addDevice(i);
    } while (FindNextVolumeW(findHandle, volumeName, ARRAYSIZE(volumeName)));
    FindVolumeClose(findHandle);
}

QList<DeviceInfo*> DeviceManagerWin::volumePaths(WCHAR *volumeName, WCHAR *deviceName)
{
    QList<DeviceInfo*> list;
    DWORD charCount = MAX_PATH+1;
    QScopedArrayPointer<WCHAR> names;
    WCHAR *nameIndex = nullptr;

    do {
        names.reset(new WCHAR[charCount]());
        if (GetVolumePathNamesForVolumeNameW(volumeName, names.data(), charCount, &charCount))
            break;
    } while (GetLastError() == ERROR_MORE_DATA);
    for (nameIndex = names.data(); *nameIndex; nameIndex += wcslen(nameIndex) + 1) {
        QString path = QString::fromWCharArray(nameIndex);
        if (path.endsWith(":\\")) {
            UINT make = GetDriveTypeW(nameIndex);
            WCHAR volumeNameBuffer[MAX_PATH+1] = {0};
            DWORD volumeSerialNumber;
            DWORD maxComponentLength;
            DWORD filesystemFlags;
            WCHAR filesystemName[MAX_PATH+1] = {0};
            GetVolumeInformationW(nameIndex, volumeNameBuffer,
                                  MAX_PATH+1,
                                  &volumeSerialNumber,
                                  &maxComponentLength,
                                  &filesystemFlags,
                                  filesystemName,
                                  MAX_PATH+1);
            DeviceInfoWin *p = new DeviceInfoWin(this);
            p->deviceType = (DeviceInfo::DeviceType)make;
            p->deviceName =  QString::fromWCharArray(deviceName);
            p->internal = QString::fromWCharArray(volumeName);
            p->volumeLabel = QString::fromWCharArray(volumeNameBuffer);
            p->mountedPath = path;
            list << p;
        }
    }
    return list;
}

DeviceInfoWin::DeviceInfoWin(QObject *parent)
    : DeviceInfo(parent)
{

}

QString DeviceInfoWin::toDisplayString()
{
    return QString("%1 [%2]").arg(mountedPath, volumeLabel);
}

void DeviceInfoWin::mount()
{
    // Do nothing
}

DeviceListener::DeviceListener(QWidget *parent) : QWidget(parent)
{
    rescanTimer.setSingleShot(true);
    rescanTimer.setInterval(0);
    connect(&rescanTimer, &QTimer::timeout,
            this, &DeviceListener::rescanNeeded);
}

bool DeviceListener::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);
    MSG *m = static_cast<MSG*>(message);
    if (m->message == WM_DEVICECHANGE) {
        Logger::log(logModule, "got device change notification");
        rescanTimer.start();
    }
    return false;
}
