#ifndef PLATFORM_ALL_H
#define PLATFORM_ALL_H

#include <QString>

namespace Platform {
    QString fixedConfigPath(QString configPath);
    QString sanitizedFilename(QString fileName);
    bool tiledDesktopsExist();
    bool tilingDesktopActive();
}

#endif // PLATFORM_ALL_H
