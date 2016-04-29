#include <QDateTime>
#include <QFileDialog>
#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QPainter>
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


static QString grabBrackets(QString source, int &position, int &length) {
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

    // grab a {}{} pair from the format string
    auto grabPair = [&position, &length](QString source) {
        QString p1 = grabBrackets(source, position, length);
        QString p2 = grabBrackets(source, position, length);
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
                output.append(currentTime.toString(grabBrackets(fmt, position, length)));
                break;
            case 'a':
                if (++position < length)
                    output.append(begin.toString(fmt.at(position)));
                ++position;
                break;
            case 'b':
                if (++position < length)
                    output.append(end.toString(fmt.at(position)));
                ++position;
                break;
            case 'w':
                if (++position < length)
                    output.append(nav.toString(fmt.at(position)));
                ++position;
                break;
            case '%':
                output.append('%');
                ++position;
                break;
            default:
                output.append(c);
                ++position;
                // %n unimplemented (look at mpv source code?)
            }
        } else {
            output.append(c);
            position++;
        }
    }
    return output;
}



JsonServer::JsonServer(QObject *parent) :
    QObject(parent)
{
    socketName = QCoreApplication::organizationDomain();
}

bool JsonServer::sendPayload(const QByteArray &payload)
{
    QLocalSocket socket;
    socket.setServerName(socketName);
    socket.connectToServer();
    if (!socket.waitForConnected(100)) {
        listen();
        return false;
    }
    socket.write(payload);
    return socket.waitForReadyRead(100);
}

void JsonServer::listen()
{
    server = new QLocalServer(this);
    server->removeServer(socketName);
    server->listen(socketName);
    connect(server, &QLocalServer::newConnection,
            this, &JsonServer::server_newConnection);
}

void JsonServer::server_newConnection()
{
    QLocalSocket *socket = server->nextPendingConnection();
    connect(socket, &QLocalSocket::readyRead, [=]() {
        QByteArray data = socket->readAll();
        socket->write("ACK");
        socket->flush();
        socket->deleteLater();
        emit payloadReceived(data);
    });
}

LogoDrawer::LogoDrawer(QObject *parent)
    : QObject(parent), logo(NULL)
{
    setLogoUrl("");
}

LogoDrawer::~LogoDrawer()
{

}

void LogoDrawer::setLogoUrl(const QString &filename)
{
    logoUrl = filename.isEmpty() ? ":/images/bitmaps/blank-screen.png"
                                 : filename;
    regenerateTexture();
}

void LogoDrawer::resizeGL(int w, int h)
{
    float ratioImg = logo.width() / std::max((float)logo.height(), 1.0f);
    float ratioWin = w / std::max((float)h, 1.0f);
    int aimWidth;
    int aimHeight;

    if (logo.width() <= w && logo.height() <= h) {
        // fits inside
        aimWidth = logo.width();
        aimHeight = logo.height();
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

void LogoDrawer::paintGL(QOpenGLWidget *widget)
{
    QPainter painter(widget);
    int ratio = widget->devicePixelRatio();
    QRect window(-1, -1, 2*ratio, 2*ratio);
    painter.setWindow(window);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, ratio > 1);
    painter.fillRect(window, QBrush(QColor(0,0,0)));
    painter.drawImage(logoLocation, logo);
}

void LogoDrawer::regenerateTexture()
{
    logo.load(logoUrl);
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
    logoDrawer->paintGL(this);
}

void LogoWidget::resizeGL(int w, int h)
{
    logoDrawer->resizeGL(w,h);
}


// For the sake of optimizing 0.0001% of execution time, let's create a
// tree out of the format string, so we don't need to do string operations
// all the time other than those we need to.
class DisplayNode {
public:
    enum NodeType { NullNode, PlainText, Trie, Property, DisplayName };
    DisplayNode() :
        type(NullNode), tagNode(NULL), audioNode(NULL), videoNode(NULL),
        next(NULL) {
    }
    ~DisplayNode() {
        empty();
        if (next)
            delete next;
        next = NULL;
    }

    bool isNull() {
        return type == NullNode;
    }

    DisplayNode *nextNode() { return next; }

    void setPlainText(QString text) {
        empty();
        data = text;
        type = PlainText;
    }

    void setDisplayName() {
        empty();
        type = DisplayName;
    }

    void setActualProperty(QString text) {
        empty();
        type = Property;
        data = text;
    }

    void setNodeTrie(QString propertyName, DisplayNode *tag,
                     DisplayNode *audio, DisplayNode *video) {
        empty();
        type = Trie;
        data = propertyName;
        tagNode = tag;
        audioNode = audio;
        videoNode = video;
    }

    void appendNode(DisplayNode *next) {
        if (this->next)
            delete this->next;
        this->next = next;
    }

    void empty() {
        if (tagNode)    { delete tagNode;   tagNode = NULL; }
        if (audioNode)  { delete audioNode; audioNode = NULL; }
        if (videoNode)  { delete videoNode; videoNode = NULL; }
    }

    QString output(const QVariantMap &metaData, const QString &displayString,
                   const Helpers::FileType fileType) {
        QString t;
        switch (type) {
        case NullNode:
            break;
        case PlainText:
            t += data;
            break;
        case Trie:
            if (metaData.contains(data) && tagNode) {
                t += tagNode->output(metaData, displayString, fileType);
            } else if (fileType == Helpers::AudioFile && audioNode) {
                t += audioNode->output(metaData, displayString, fileType);
            } else if (videoNode) {
                t += videoNode->output(metaData, displayString, fileType);
            }
            break;
        case Property:
            if (metaData.contains(data)) {
                t += metaData[data].toString();
            }
            break;
        case DisplayName:
            t += displayString;
        default:
            break;
        }
        if (next)
            t += next->output(metaData, displayString, fileType);
        return t;
    }

private:
    NodeType type;
    QString data;
    DisplayNode *tagNode;
    DisplayNode *audioNode;
    DisplayNode *videoNode;
    DisplayNode *next;
};


DisplayParser::DisplayParser() : node(NULL)
{

}

DisplayParser::~DisplayParser()
{
    if (node) { delete node; node = NULL; }
}

void DisplayParser::takeFormatString(QString fmt)
{
    int length = fmt.length();
    int position = 0;

    // grab a {}{}{} tuple from the format string
    auto grabTuple = [&position, &length](QString source) {
        QString p1 = grabBrackets(source, position, length);
        QString p2 = grabBrackets(source, position, length);
        QString p3 = grabBrackets(source, position, length);
        return QStringList({p1, p2, p3});
    };

    // grab the text between % and {
    auto grabProp = [&position](QString source) {
        int run = source.mid(position).indexOf(QChar('{'));
        if (run >= 0) {
            QString ret = source.mid(position, run);
            position += run;
            return ret;
        }
        return QString();
    };

    // dump whatever data may have been gathered up to this point
    auto dumpGatheredData = [](QString &gathered, DisplayNode* &current,
            bool final = false) {
        if (!gathered.isEmpty()) {
            current->setPlainText(gathered);
            if (!final) {
                current->appendNode(new DisplayNode);
                current = current->nextNode();
                gathered.clear();
            }
        }
    };

    // convert text inside {} to a node
    auto nodeInnerChars = [dumpGatheredData](QString text, QString propertyValue) {
        DisplayNode *first = new DisplayNode;
        DisplayNode *current = first;
        QString gathered;
        QChar c;
        int length = text.length();
        int position = 0;
        while (position < length) {
            c = text.at(position);
            position++;
            if (c == '#') {
                if (position < length && text.at(position)=='#') {
                    gathered += '#';
                    position++;
                } else {
                    dumpGatheredData(gathered, current);
                    current->setActualProperty(propertyValue);
                    current->appendNode(new DisplayNode);
                    current = current->nextNode();
                }
            } else if (c == '$') {
                if (position < length && text.at(position)=='$') {
                    gathered += '$';
                    position++;
                } else {
                    dumpGatheredData(gathered, current);
                    current->setDisplayName();
                    current->appendNode(new DisplayNode);
                    current = current->nextNode();
                }
            } else {
                gathered += c;
            }
        }
        dumpGatheredData(gathered, current, true);
        return first;
    };

    if (node)
        delete node;
    node = new DisplayNode;

    DisplayNode *current = node;
    QString prop;
    QStringList tuple;
    QString gathered;
    while (position < length) {
        QChar c = fmt.at(position++);
        if (c == '%') {
            if (position < length && fmt.at(position)=='%') {
                gathered += '%';
                continue;
            }
            dumpGatheredData(gathered, current);
            prop = grabProp(fmt);
            if (prop.isEmpty())
                continue;
            tuple = grabTuple(fmt);
            current->setNodeTrie(prop, nodeInnerChars(tuple[0], prop),
                                 nodeInnerChars(tuple[1], prop),
                                 nodeInnerChars(tuple[2], prop));
            current->appendNode(new DisplayNode);
            current = current->nextNode();
        } else {
            gathered += c;
        }
    }
    dumpGatheredData(gathered, current, true);
}

QString DisplayParser::parseMetadata(QVariantMap metaData,
                                     QString displayString,
                                     Helpers::FileType fileType)
{
    if (metaData.isEmpty()) {
        return displayString;
    } else {
        if (!metaData.contains("title"))
            metaData["title"] = displayString;
        return node->output(metaData, displayString, fileType);
    }
}

TrackInfo::TrackInfo(const QUrl &url, const QUuid &list, const QUuid &item)
{
    this->url = url;
    this->list = list;
    this->item = item;
}

QVariantMap TrackInfo::toVMap() const
{
    return QVariantMap({{"url", url}, {"list", list}, {"item", item}});
}

void TrackInfo::fromVMap(const QVariantMap &map)
{
    url = map.value("url").toUrl();
    list = map.value("list").toUuid();
    item = map.value("item").toUuid();
}

bool TrackInfo::operator ==(const TrackInfo &track) const
{
    return url == track.url;
}
