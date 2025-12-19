#ifndef DEVICEMANAGERWIN_H
#define DEVICEMANAGERWIN_H

#include <QTimer>
#include <QWidget>
#include <windows.h>
#include "devicemanager.h"

class DeviceManagerWin;
class DeviceInfoWin;
class DeviceListener;

class DeviceManagerWin : public DeviceManager
{
    Q_OBJECT
public:
    DeviceManagerWin(QObject *parent);
    ~DeviceManagerWin();
    bool deviceAccessPossible();

public slots:
    void rescan();

protected:
    void populate();

private:
    DeviceListener *listener;
    QList<DeviceInfo*> volumePaths(WCHAR *volumeName, WCHAR *deviceName);
};


class DeviceInfoWin : public DeviceInfo
{
    Q_OBJECT
public:
    DeviceInfoWin(QObject *parent = nullptr);
    QString toDisplayString();
    void mount();
};

class DeviceListener : public QWidget
{
    Q_OBJECT
public:
    explicit DeviceListener(QWidget *parent = nullptr);

signals:
    void rescanNeeded();

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, long *result);

private:
    QTimer rescanTimer;
};


#endif // DEVICEMANAGERWIN_H
