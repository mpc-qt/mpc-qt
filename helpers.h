// generic helper module for general-use static functions
#ifndef HELPERS_H
#define HELPERS_H
#include <QObject>
#include <QWidget>
#include <QList>
#include <QUrl>
#include <QOpenGLWidget>

class QFileDialog;
class QLocalServer;
class QOpenGLTexture;

namespace Helpers {
    QString toDateFormat(double time);
    enum DisabledTrack { NothingDisabled, DisabledAudio, DisabledVideo };
    enum Subtitles { NoSubtitles, SubtitlesPresent, SubtitlesDisabled, };
    enum FileType { AudioFile, VideoFile };
    QString parseFormat(QString fmt, QString fileName, DisabledTrack disabled,
                        Subtitles subtitles, double timeNav, double timeBegin,
                        double timeEnd);
}

class JsonServer : public QObject
{
    Q_OBJECT
public:
    explicit JsonServer(QObject *parent = 0);
    bool sendPayload(const QByteArray &payload);

private:
    void listen();

signals:
    void payloadReceived(const QByteArray &payload);

private slots:
    void server_newConnection();

private:
    QString socketName;
    QLocalServer *server;
};

class LogoDrawer : public QObject {
    Q_OBJECT
public:
    explicit LogoDrawer(QObject *parent = 0);
    ~LogoDrawer();
    void setLogoUrl(const QString &filename);
    void resizeGL(int w, int h);
    void paintGL();

private:
    void regenerateTexture();

private:
    QRectF logoLocation;
    QOpenGLTexture *logo;
    QString logoUrl;
};

class LogoWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit LogoWidget(QWidget *parent = 0);
    ~LogoWidget();
    void setLogo(const QString &filename);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);

private:
    LogoDrawer *logoDrawer;
    QString logoUrl;
};

class DisplayNode;
class DisplayParser {
public:
    DisplayParser();
    ~DisplayParser();

    void takeFormatString(QString fmt);
    QString parseMetadata(QVariantMap metaData, QString displayString,
                          Helpers::FileType fileType);
private:
    DisplayNode *node;
};

#endif // HELPERS_H
