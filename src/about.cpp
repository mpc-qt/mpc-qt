#include "about.h"
#include "helpers.h"
#include "platform/unify.h"
#include "version.h"
#include <QApplication>
#include <QDateTime>
#include <QMessageBox>

void About::showAbout(QWidget *parent, QString mpvVersion, QString ffmpegVersion)
{
#if defined(MPCQT_DEVELOPMENT)
#define BUILD_VERSION_STR QString("%1 (%2)").arg(devBuild, MPCQT_VERSION_STR)
#elif defined(MPCQT_VERSION_STR)
#define BUILD_VERSION_STR versionFmt.arg(MPCQT_VERSION_STR)
#else
#define BUILD_VERSION_STR devBuild
#endif
    QString devBuild = tr("Development Build");
    QString versionFmt = tr("Version %1");
    QString dateLineFmt = tr("Built on %1 at %2");
    QString dateLine;

#if defined(MPCQT_DEVELOPMENT)
    QDate buildDate = Helpers::dateFromCFormat(__DATE__);
    QTime buildTime = Helpers::timeFromCFormat(__TIME__);
    QString dateFormat = QLocale().dateFormat(QLocale::ShortFormat);
    QString timeFormat = QLocale().timeFormat(QLocale::ShortFormat);
    dateLine = "<br>" + dateLineFmt.arg(QLocale().toString(buildDate, dateFormat),
                                                QLocale().toString(buildTime, timeFormat));
#endif
    QString packageLine = packageType();
    packageLine = packageLine.isEmpty() ? "" : " (" + packageLine + ")";
    QString displayMode = tr("(Unknown)");
    if (QGuiApplication::platformName() == "wayland")
        displayMode = "Wayland";
    else {
        if (qEnvironmentVariable("XDG_SESSION_TYPE") == "wayland")
            displayMode = "XWayland";
        else
            displayMode = "X11";
    }
    QMessageBox::about(parent, tr("About Media Player Classic Qute Theater"),
      "<h2>" + tr("Media Player Classic Qute Theater") + "</h2>" +
      "<p>" +  tr("A clone of Media Player Classic written in Qt") +
      "<br>" + tr("Based on Qt %1 and %2").arg(QT_VERSION_STR, mpvVersion) +
      "<br>" + QString("ffmpeg %1").arg(ffmpegVersion) +
      (Platform::isUnix ? "<br>" + tr("Running on %1").arg(displayMode) : "") +
      "<p>" +  BUILD_VERSION_STR + (Platform::isUnix ? packageLine : "") +
      dateLine +
      "<h3>LICENSE</h3>"
      "<p>   Copyright (C) 2015"
      "<p>"
      "This program is free software; you can redistribute it and/or modify "
      "it under the terms of the GNU General Public License as published by "
      "the Free Software Foundation; either version 2 of the License, or "
      "(at your option) any later version."
      "<p>"
      "This program is distributed in the hope that it will be useful, "
      "but WITHOUT ANY WARRANTY; without even the implied warranty of "
      "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
      "GNU General Public License for more details."
      "<p>"
      "You should have received a copy of the GNU General Public License "
      "along with this program; if not, write to the Free Software "
      "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA "
      "02110-1301 USA.");
}

QString About::version()
{
    return MPCQT_VERSION_STR;
}

QString About::buildTime()
{
    return (QString) __DATE__ + " " + __TIME__;
}

QString About::packageType()
{
    QString package;
    if (qEnvironmentVariableIsSet("FLATPAK_ID"))
        package = "Flatpak";
    else if (qEnvironmentVariableIsSet("APPIMAGE"))
        package = "AppImage";
    else if (qEnvironmentVariableIsSet("SNAP"))
        package = "Snap";
    return package;
}
