#ifndef PLATFORM_ALL_H
#define PLATFORM_ALL_H

#include <QWidget>
#include <QString>

namespace Platform {
    extern const bool isMac;
    extern const bool isWindows;
    extern const bool isUnix;

    QString fixedConfigPath(QString configPath);
    QString sanitizedFilename(QString fileName);
    bool tiledDesktopsExist();
    bool tilingDesktopActive();
    void disableAutomaticAccel(QWidget *what);
}

#endif // PLATFORM_ALL_H
