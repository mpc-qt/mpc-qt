#include <QDebug>
#include <QString>
#include <windows.h>
#include "devicemanager_win.h"

static QList<QSharedPointer<DeviceInfo>> volumePaths(WCHAR *volumeName, WCHAR *deviceName);

DeviceManagerWin::DeviceManagerWin(QObject *parent)
    : DeviceManager(parent)
{
    populate();
}

bool DeviceManagerWin::deviceAccessPossible()
{
    return false;
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

static QList<QSharedPointer<DeviceInfo>> volumePaths(WCHAR *volumeName, WCHAR *deviceName)
{
    QList<QSharedPointer<DeviceInfo>> list;
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
            DeviceInfo *p = new DeviceInfo;
            p->deviceType = (DeviceInfo::DeviceType)make;
            p->deviceName =  QString::fromWCharArray(deviceName);
            p->internal = QString::fromWCharArray(volumeName);
            p->volumeLabel = QString::fromWCharArray(volumeNameBuffer);
            p->mountedPath = path;
            list << QSharedPointer<DeviceInfo>(p);
        }
    }
    return list;
}
