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
    QString parseFormat(QString fmt, QString fileName, DisabledTrack disabled,
                        Subtitles subtitles, double timeNav, double timeBegin,
                        double timeEnd);
}

class AsyncFileDialog : public QObject {
    Q_OBJECT

public:
    enum DialogMode { SingleFile, MultipleFiles, FolderContents, Directory };

    explicit AsyncFileDialog(QWidget *parent = NULL);
    ~AsyncFileDialog();
    void setMode(DialogMode mode);
    void setSave(bool yes);
    void show();

signals:
    void filesOpened(QList<QUrl> files);
    void fileOpened(QUrl file);
    void dirOpened(QUrl dir);

private slots:
    void qfd_urlsSelected(QList<QUrl> urls);

private:
    QFileDialog *qfd;
    DialogMode mode_;
};

class SingleProcess : public QObject
{
    Q_OBJECT
public:
    explicit SingleProcess(QObject *parent = 0);
    bool hasPrevious(const QStringList &payload);

private:
    void listen();

signals:
    void payloadReceived(const QStringList &payload);

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

#endif // HELPERS_H
