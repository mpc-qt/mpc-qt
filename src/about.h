#ifndef ABOUT_H
#define ABOUT_H

#include <QObject>

class About : public QObject
{
    Q_OBJECT

public:
    static void showAbout(QWidget *parent, QString mpvVersion, QString ffmpegVersion);
    static QString version();
    static QString buildTime();
    static QString packageType();
};

#endif // ABOUT_H
