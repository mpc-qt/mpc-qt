#include <QDateTime>
#include <QFileDialog>
#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include "helpers.h"

QString Helpers::toDateFormat(double time)
{
    int t = time*1000 + 0.5;
    if (t < 0)
        t = 0;
    int hr = t/3600000;
    int mn = t/60000 % 60;
    int se = t%60000 / 1000;
    int fr = t % 1000;
    return QString("%1:%2:%3.%4").arg(QString().number(hr))
            .arg(QString().number(mn),2,'0')
            .arg(QString().number(se),2,'0')
            .arg(QString().number(fr),3,'0');
}


QString Helpers::parseFormat(QString fmt, QString fileName,
                             Helpers::DisabledTrack disabled,
                             Subtitles subtitles, double timeNav,
                             double timeBegin, double timeEnd)
{
    // convenient format parsing
    struct TimeParse {
        TimeParse(double time) {
            this->time = time;
            int t = time*1000 + 0.5;
            hr = t/3600000;
            mn = t/60000 % 60;
            se = t%60000 / 1000;
            fr = t % 1000;
        }
        QString toString(QChar fmt) {
            switch (fmt.unicode()) {
            case 'p':
                return QString("%1:%2:%3")
                        .arg(QString::number(hr),2,'0')
                        .arg(QString::number(mn),2,'0')
                        .arg(QString::number(se),2,'0');
            case 'P':
                return QString("%1:%2:%3.%4")
                        .arg(QString::number(hr),2,'0')
                        .arg(QString::number(mn),2,'0')
                        .arg(QString::number(se),2,'0')
                        .arg(QString::number(fr),3,'0');
            case 'H':
                return QString("%1").arg(QString::number(hr),2,'0');
            case 'M':
                return QString("%1").arg(QString::number(mn),2,'0');
            case 'S':
                return QString("%1").arg(QString::number(se),2,'0');
            case 'T':
                return QString("%1").arg(QString::number(fr),3,'0');
            case 'h':
                return QString::number(hr);
            case 'm':
                return QString::number((int)(time)/60);
            case 's':
                return QString::number((int)time);
            case 'f':
                return QString::number(time,'f');
            }
            return fmt;
        }
        double time;
        int hr, mn, se, fr;
    };

    QString fileNameNoExt = QFileInfo(fileName).completeBaseName();
    TimeParse nav(timeNav), begin(timeBegin), end(timeEnd);
    QDateTime currentTime = QDateTime::currentDateTime();
    QString output;
    int length = fmt.length();
    int position = 0;

    // grab a {} from the format string
    auto grabBrackets = [&position, &length](QString source) {
        QString match;
        QChar c;
        enum MatchMode { Bracket, Inside, Finish };
        MatchMode mm = Bracket;
        while (mm != Finish && position < length) {
            c = source.at(position);
            if (mm == Bracket) {
                if (c != '{') {
                    position--;
                    mm = Finish;
                } else {
                    mm = Inside;
                }
                ++position;
            } else if (mm == Inside) {
                if (c != '}')
                    match.append(c);
                else
                    mm = Finish;
                ++position;
            }
        }
        return match;
    };

    // grab a {}{} pair from the format string
    auto grabPair = [&position, &length, grabBrackets](QString source) {
        QString p1 = grabBrackets(source);
        QString p2 = grabBrackets(source);
        return QStringList({p1, p2});
    };

    QStringList pairs;
    while (position < length) {
        QChar c = fmt.at(position);
        if (c == '%') {
            position++;
            if (position >= length)
                break;
            c = fmt.at(position);
            switch (c.unicode()) {
            case 'f':
                ++position;
                output += fileName;
                break;
            case 'F':
                ++position;
                output += fileNameNoExt;
                break;
            case 's':
                ++position;
                pairs = grabPair(fmt);
                if (subtitles == SubtitlesPresent)
                    output.append(pairs[0]);
                if (subtitles == SubtitlesDisabled)
                    output.append(pairs[1]);
                break;
            case 'd':
                ++position;
                pairs = grabPair(fmt);
                if (disabled == DisabledAudio)
                    output.append(pairs[0]);
                if (disabled == DisabledVideo)
                    output.append(pairs[1]);
                break;
            case 't':
                ++position;
                output.append(currentTime.toString(grabBrackets(fmt)));
                break;
            case 'a':
                if (++position < length)
                    output.append(begin.toString(fmt.at(position)));
                break;
            case 'b':
                if (++position < length)
                    output.append(end.toString(fmt.at(position)));
                break;
            case 'w':
                if (++position < length)
                    output.append(nav.toString(fmt.at(position)));
                break;
            case '%':
                output.append('%');
                break;
            default:
                output.append(c);
                // %n unimplemented (look at mpv source code?)
            }
        } else {
            output.append(c);
            position++;
        }
    }
    return output;
}

AsyncFileDialog::AsyncFileDialog(QWidget *parent)
    : QObject(parent), qfd(NULL), mode_(SingleFile)
{
    qfd = new QFileDialog(parent);
    qfd->setAttribute(Qt::WA_DeleteOnClose);
    qfd->setOption(QFileDialog::DontResolveSymlinks);
    qfd->setWindowModality(Qt::WindowModal);
    connect(qfd, &QFileDialog::urlsSelected,
            this, &AsyncFileDialog::qfd_urlsSelected);
    connect(qfd, &QFileDialog::destroyed, [=]() {
        qfd = NULL;
        this->deleteLater();
    });
}

AsyncFileDialog::~AsyncFileDialog()
{
    if (qfd)
        delete qfd;
}

void AsyncFileDialog::setMode(AsyncFileDialog::DialogMode mode)
{
    switch (mode) {
    case FolderContents:
    case Directory:
        qfd->setFileMode(QFileDialog::Directory);
        qfd->setOption(QFileDialog::ShowDirsOnly);
        break;
    case SingleFile:
        qfd->setFileMode(QFileDialog::ExistingFile);
        break;
    case MultipleFiles:
        qfd->setFileMode(QFileDialog::ExistingFiles);
        break;
    }
    mode_ = mode;
}

void AsyncFileDialog::show()
{
    qfd->show();
}

void AsyncFileDialog::qfd_urlsSelected(QList<QUrl> urls)
{
    if (urls.isEmpty())
        return;
    if (mode_ == SingleFile) {
        emit fileOpened(urls.front());
        return;
    }
    if (mode_ == Directory) {
        emit dirOpened(urls.front());
        return;
    }
    if (mode_ == MultipleFiles) {
        emit filesOpened(urls);
        return;
    }
    if (mode_ ==FolderContents) {
        QDir dir(urls.front().toLocalFile());
        QList<QUrl> list;
        QFileInfoList f = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files);
        for(auto file : f)
            list.append(QUrl::fromLocalFile(file.absoluteFilePath()));
        emit filesOpened(list);
    }
}

SingleProcess::SingleProcess(QObject *parent) :
    QObject(parent)
{
    socketName = QCoreApplication::organizationDomain();
}

bool SingleProcess::hasPrevious(const QStringList &payload)
{
    QLocalSocket socket;
    socket.setServerName(socketName);
    socket.connectToServer();
    if (!socket.waitForConnected(100)) {
        listen();
        return false;
    }
    socket.write(payload.join('\n').toUtf8());
    return socket.waitForReadyRead(100);
}

void SingleProcess::listen()
{
    server = new QLocalServer(this);
    server->removeServer(socketName);
    server->listen(socketName);
    connect(server, &QLocalServer::newConnection,
            this, &SingleProcess::server_newConnection);
}

void SingleProcess::server_newConnection()
{
    QLocalSocket *socket = server->nextPendingConnection();
    connect(socket, &QLocalSocket::readyRead, [=]() {
        QByteArray data = socket->readAll();
        socket->write("ACK");
        socket->flush();
        socket->deleteLater();
        emit payloadReceived(QString::fromUtf8(data).split('\n'));
    });
}

LogoDrawer::LogoDrawer(QObject *parent)
    : QObject(parent), logo(NULL)
{
    setLogoUrl("");
}

LogoDrawer::~LogoDrawer()
{
    if (logo)
        delete logo;
}

void LogoDrawer::setLogoUrl(const QString &filename)
{
    logoUrl = filename.isEmpty() ? ":/images/bitmaps/blank-screen.png"
                                 : filename;
    regenerateTexture();
}

void LogoDrawer::resizeGL(int w, int h)
{
    float ratioImg = logo->width() / std::max((float)logo->height(), 1.0f);
    float ratioWin = w / std::max((float)h, 1.0f);
    int aimWidth;
    int aimHeight;

    if (logo->width() <= w && logo->height() <= h) {
        // fits inside
        aimWidth = logo->width();
        aimHeight = logo->height();
    } else if (ratioImg > ratioWin) {
        // left and right touch
        aimWidth = w;
        aimHeight = w / ratioImg;
    } else {
        // top and bottom touch
        aimWidth = h * ratioImg;
        aimHeight = h;
    }
    float fw = 2.0f/w;
    float fh = 2.0f/h;
    float iw = fw * aimWidth;
    float ih = fh * aimHeight;
    logoLocation = {-iw/2, -ih/2, iw, ih};
}

void LogoDrawer::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, logo->textureId());
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);  // eww, quads!
        glTexCoord2f(0,0);
            glVertex2f(logoLocation.left(), logoLocation.bottom());
        glTexCoord2f(1,0);
            glVertex2f(logoLocation.right(), logoLocation.bottom());
        glTexCoord2f(1,1);
            glVertex2f(logoLocation.right(), logoLocation.top());
        glTexCoord2f(0,1);
            glVertex2f(logoLocation.left(), logoLocation.top());
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

void LogoDrawer::regenerateTexture()
{
    if (logo) {
        delete logo;
    }
    logo = new QOpenGLTexture(QImage(logoUrl),
                                     QOpenGLTexture::DontGenerateMipMaps);
    logo->setMinificationFilter(QOpenGLTexture::Linear);
}

LogoWidget::LogoWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      logoDrawer(NULL)
{
}

LogoWidget::~LogoWidget()
{
    if (logoDrawer) {
        makeCurrent();
        delete logoDrawer;
        logoDrawer = NULL;
    }
}

void LogoWidget::setLogo(const QString &filename) {
    logoUrl = filename;
    if (logoDrawer) {
        makeCurrent();
        logoDrawer->setLogoUrl(filename);
        logoDrawer->resizeGL(width(), height());
        doneCurrent();
        update();
    }
}

void LogoWidget::initializeGL()
{
    if (!logoDrawer) {
        logoDrawer = new LogoDrawer(this);
        logoDrawer->setLogoUrl(logoUrl);
    }
}

void LogoWidget::paintGL()
{
    logoDrawer->paintGL();
}

void LogoWidget::resizeGL(int w, int h)
{
    logoDrawer->resizeGL(w,h);
}
