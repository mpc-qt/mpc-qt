#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QTcpSocket>
#include "helpers.h"
#include "version.h"
#include "ipc/http.h"
#include "platform/devicemanager.h"
#include "platform/unify.h"

static constexpr char logModule[] =  "http";
static constexpr char httpVersion[] = "HTTP/1.1";
static constexpr char errorDocument[] = "<html><head><title>%1</title></head>"
                                    "<body><h1>%1</h1></body></html>";
static constexpr char mimeFormUrlEncoded[] = "application/x-www-form-urlencoded";
static constexpr char mimeHtml[] = "text/html";
static constexpr char dateZero[] = "1970.01.01 00:00";

static const QMap<HttpResponse::HttpStatus, QString> statusToText = {
    { HttpResponse::Http200Ok, "200 OK" },
    { HttpResponse::Http204NoContent, "204 No Content" },
    { HttpResponse::Http301MovedPermanently, "301 Moved Permanently" },
    { HttpResponse::Http302Found, "302 Found" },
    { HttpResponse::Http303SeeOther, "303 See Other" },
    { HttpResponse::Http400BadRequest, "400 Bad Request" },
    { HttpResponse::Http401Unauthorized, "401 Unauthorized" },
    { HttpResponse::Http403Forbidden, "403 Forbidden" },
    { HttpResponse::Http404NotFound, "404 Not Found" },
    { HttpResponse::Http500InternalServerError, "500 Internal Server Error" },
    { HttpResponse::Http501NotImplemented, "501 Not Implemented" }
};

static const QMap<QString, QString> extensionToText {
    { "aac", "audio/aac" },
    { "avi", "video/x-msvideo" },
    { "bin", "application/octet-stream" },
    { "bmp", "image/bmp" },
    { "css", "text/css" },
    { "eot", "application/vnd-ms-fontobject" },
    { "gif", "image/gif" },
    { "htm", "text/html" },
    { "html", "text/html" },
    { "ico", "image/vnd.microsoft.icon" },
    { "jpeg", "image/jpeg" },
    { "jpg", "image/jpeg" },
    { "js", "text/javascript" },
    { "json", "application/json" },
    { "mp3", "audio/mpeg" },
    { "mpeg", "video/mpeg" },
    { "oga", "audio/ogg" },
    { "ogg", "audio/ogg" },
    { "ogv", "video/ogg" },
    { "otf", "font/otf" },
    { "pdf", "application/pdf" },
    { "png", "image/png" },
    { "svg", "image/svg+xml" },
    { "tif", "image/tiff" },
    { "tiff", "image/tiff" },
    { "ttf", "font/ttf" },
    { "txt", "text/plain" },
    { "wav", "audio/wav" },
    { "weba", "audio/webm" },
    { "webm", "video/webm" },
    { "webp", "image/webp" },
    { "woff", "font/woff" },
    { "woff2", "font/woff2" },
    { "xhtml", "application/xhtml+xml" },
    { "xml", "application/xml" }
};

HttpResponse::HttpResponse()
{
    statusCode = HttpResponse::Http200Ok;
    dateResponse = dateModified = QDateTime::currentDateTimeUtc();
    contentType = mimeHtml;
    fallthrough = false;
}

void HttpResponse::fillHeaders()
{
    static constexpr char dateFmt[] = "ddd, dd MMM yyyy HH:mm:ss t";
    headers["Date"] = dateResponse.toString(dateFmt);
    if (!headers.contains("Server"))
        headers["Server"] = QCoreApplication::applicationName();
    headers["Last-Modified"] = dateModified.toString(dateFmt);
    //headers["Accept-Ranges"] = "bytes";
    headers["Content-Length"] = QString::number(content.size());
    headers["Content-Type"] = contentType;
}

void HttpResponse::redirect(QString where) {
    statusCode = HttpResponse::Http302Found;
    headers["Location"] = where;
}

void HttpResponse::serveFile(QString filename)
{
    filename.replace("..", ".");

    QFile f(filename);
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
        statusCode = HttpResponse::Http404NotFound;
        return;
    }

    auto extension = QFileInfo(filename).suffix();
    contentType = HttpServer::extensionToContentType(extension);
    content = f.readAll();
}

HttpServer::HttpServer(QObject *owner) : QTcpServer(owner)
{
    connect(this, &HttpServer::newConnection,
            this, &HttpServer::self_newConnection);
}

void HttpServer::clearRoutes()
{
    routeMap.clear();
}

void HttpServer::route(QString path, std::function<void(HttpRequest&, HttpResponse&)> callback)
{
    routeMap.append({path, callback});
}

QString HttpServer::extensionToContentType(QString extension)
{
    return extensionToText.value(extension, extensionToText["bin"]);
}

QString HttpServer::urlDecode(QString urlText, bool plusToSpace)
{
    using byte = unsigned char;
    static QByteArray chToHex = [](){
        QByteArray map;
        map.resize(256);
        for (int i = 0; i < 256; i++)
            map[i] = 0;
        map[(byte)'0'] = 0;
        map[(byte)'1'] = 1;
        map[(byte)'2'] = 2;
        map[(byte)'3'] = 3;
        map[(byte)'4'] = 4;
        map[(byte)'5'] = 5;
        map[(byte)'6'] = 6;
        map[(byte)'7'] = 7;
        map[(byte)'8'] = 8;
        map[(byte)'9'] = 9;
        map[(byte)'a'] = 10;
        map[(byte)'A'] = 10;
        map[(byte)'b'] = 11;
        map[(byte)'B'] = 11;
        map[(byte)'c'] = 12;
        map[(byte)'C'] = 12;
        map[(byte)'d'] = 13;
        map[(byte)'D'] = 13;
        map[(byte)'e'] = 14;
        map[(byte)'E'] = 14;
        map[(byte)'f'] = 15;
        map[(byte)'F'] = 15;
        return map;
    }();
    QByteArray ba = urlText.toUtf8();
    QByteArray dest;
    int length = ba.length();
    byte c;
    const byte *data = reinterpret_cast<const byte*>(ba.constData());
    for (int i = 0; i < length; i++) {
        if (data[i] != '%') {
            if (plusToSpace && data[i] == '+')
                dest.append(' ');
            else
                dest.append(data[i]);
            continue;
        }
        c = 0;
        if (i < length-1) {
            i++;
            c = chToHex[data[i]];
        }
        if (i < length-1) {
            i++;
            c *= 16;
            c += chToHex[data[i]];
            if (c)
                dest.append(c);
        }
    }
    return QString::fromUtf8(dest);
}

QString HttpServer::urlEncode(QString plainText)
{
    using byte = unsigned char;
    static const QByteArray isEncoded = [] {
        QByteArray map;
        map.resize(256);
        for (int i = 0; i < 32; i++)
            map[i] = 1;
        for (int i = 32; i < 128; i++)
            map[i] = 0;
        for (int i = 128; i < 256; i++)
            map[i] = 1;
        map[(byte)' '] = 1;
        map[(byte)'!'] = 1;
        map[(byte)'*'] = 1;
        map[(byte)'\''] = 1;
        map[(byte)'('] = 1;
        map[(byte)')'] = 1;
        map[(byte)';'] = 1;
        map[(byte)':'] = 1;
        map[(byte)'@'] = 1;
        map[(byte)'&'] = 1;
        map[(byte)'='] = 1;
        map[(byte)'+'] = 1;
        map[(byte)'$'] = 1;
        map[(byte)','] = 1;
        map[(byte)'/'] = 1;
        map[(byte)'\\'] = 1;
        map[(byte)'?'] = 1;
        map[(byte)'#'] = 1;
        map[(byte)'['] = 1;
        map[(byte)']'] = 1;
        return map;
    }();
    static constexpr char hexadecimal[] = "0123456789ABCDEF";
    QByteArray ba = plainText.toUtf8();
    QByteArray dest;
    const byte *data = reinterpret_cast<const byte*>(ba.constData());
    int length = ba.length();
    for (int i = 0; i < length; i++) {
        if (isEncoded[data[i]]) {
            dest.append('%');
            dest.append(hexadecimal[data[i]/16]);
            dest.append(hexadecimal[data[i]%16]);
        } else {
            dest.append(data[i]);
        }
    }
    return QString::fromUtf8(dest);
}

void HttpServer::sendSocket(QTcpSocket *sock, HttpResponse &res)
{
    QString statusText = statusToText.value(res.statusCode);
    if (res.statusCode >= 300 && res.content.isEmpty()) {
        res.content = QString(errorDocument).arg(statusText).toUtf8();
    }

    res.fillHeaders();
    res.statusLine = QString("%1 %2").arg(httpVersion, statusText);

    QByteArray output;
    output.append(res.statusLine.toUtf8());
    output.append("\r\n");
    QMapIterator<QString,QString> i(res.headers);
    while (i.hasNext()) {
        i.next();
        output.append(QString("%1: %2\r\n").arg(i.key(), i.value()).toUtf8());
    }
    output.append("\r\n");
    output.append(res.content);
    sock->write(output);
    sock->close();
}

void HttpServer::self_newConnection()
{
    while (auto sock = nextPendingConnection()) {
        connect(sock, &QTcpSocket::readyRead,
                this, [this, sock]() {
            socket_readyRead(sock);
        });
    }
}

void HttpServer::socket_readyRead(QTcpSocket *sock)
{
    QByteArray raw = sock->readAll();
    QStringList header = QString::fromUtf8(raw).split("\r\n");
    if (header.size() < 1) {
        HttpResponse res;
        res.statusCode = HttpResponse::Http400BadRequest;
        sendSocket(sock, res);
        return;
    }
    QStringList firstLine = header.value(0).split(' ');

    HttpRequest req;
    req.method = firstLine.value(0);
    if (req.method != "GET" && req.method != "POST") {
        HttpResponse res;
        res.statusCode = HttpResponse::Http501NotImplemented;
        sendSocket(sock, res);
        return;
    }
    req.url = firstLine.value(1);
    header.pop_front();

    auto splitLine = [](QString line, QMap<QString,QString> &vars) {
        QStringList pairs = line.split('&');
        for (auto &p : pairs) {
            auto p2 = p.split('=');
            vars.insert(p2.value(0), urlDecode(p2.value(1), true));
        }
    };

    if (req.url.contains("?")) {
        QStringList segments = req.url.split('?');
        req.url = segments.value(0);
        segments.pop_front();
        QString line = segments.join('&');
        splitLine(line, req.getVars);
    }

    bool seenHeader = false;
    for (int i = 0; i < header.count(); i++) {
        QString line = header[i];
        if (!seenHeader && line.isEmpty()) {
            // seen the last of the header
            seenHeader = true;
        } else if (!seenHeader && line.contains(": ")) {
            // header line
            QStringList pair = line.split(": ");
            req.headers.insert(pair.value(0), pair.value(1));
        } else if (seenHeader && !line.isEmpty() && req.method == "POST"
                   && req.headers.value("Content-Type") == mimeFormUrlEncoded) {
            splitLine(line, req.postVars);
        }
    }

    HttpResponse res;
    bool foundRoute = false;
    for (auto &route : routeMap) {
        if (req.url.contains(route.first)) {
            route.second(req,res);
            if (!res.fallthrough) {
                foundRoute = true;
                break;
            }
            res.fallthrough = false;
        }
    }
    if (!foundRoute)
        res.statusCode = HttpResponse::Http404NotFound;
    sendSocket(sock, res);
}



MpcHcServer::MpcHcServer(QObject *owner)
    : QObject(owner)
{
    setupWmCommands();
    setupHttp();
}

void MpcHcServer::setDefaultPage(QString file)
{
    defaultPage = file;
}

void MpcHcServer::setEnabled(bool yes)
{
    enabled = yes;
    relisten();
}

void MpcHcServer::setLocalHostOnly(bool yes)
{
    localHostOnly = yes;
    relisten();
}

void MpcHcServer::setTcpPort(uint16_t port)
{
    tcpPort = port;
    relisten();
}

void MpcHcServer::setServeFiles(bool yes)
{
    serveFiles = yes;
}

void MpcHcServer::setWebRoot(QString path)
{
    webRoot = path;
}

void MpcHcServer::setFileSize(int64_t size)
{
    fileSize = size;
}

void MpcHcServer::setFileTime(double time, double duration)
{
    fileTime = time;
    fileDuration = duration;
}

void MpcHcServer::setNowPlaying(QUrl nowPlaying)
{
    this->nowPlaying = nowPlaying;
}

void MpcHcServer::setMediaTitle(QString title)
{
    mediaTitle = title;
}

void MpcHcServer::setPlaybackRate(double rate)
{
    playbackRate = rate;
}

void MpcHcServer::setPlaybackState(PlaybackManager::PlaybackState state)
{
    playbackState = state;
}

void MpcHcServer::setVolume(int64_t volume)
{
    this->volume = volume;
}

void MpcHcServer::setVolumeMuted(bool muted)
{
    volumeMuted = muted;
}

void MpcHcServer::relisten()
{
    http.close();
    if (enabled)
        http.listen(localHostOnly ? QHostAddress::LocalHost : QHostAddress::Any, tcpPort);
}

void MpcHcServer::setupWmCommands()
{
    // I'm probably missing some things that we can do
    wmCommands = QList<WmCommand>({
        { 969, "Quick Open File",           [&](){ emit quickOpenFile(); } },
        { 800, "Open File/URL",             [&](){ emit openFileUrl(); } },
        { 806, "Save Image",                [&](){ emit saveImage(); } },
        { 807, "Save Image (auto)",         [&](){ emit saveImageAuto(); } },
        { 808, "Save Thumbnails",           [&](){ emit saveThumbnails(); } },
        { 804, "Close",                     [&](){ emit close(); } },
        { 814, "Properties",                [&](){ emit properties(); } },
        { 816, "Exit",                      [&](){ emit exit(); } },
        { 887, "Play",                      [&](){ emit play(); } },
        { 888, "Pause",                     [&](){ emit pause(); } },
        { 890, "Stop",                      [&](){ emit stop(); } },
        { 891, "Frame-step",                [&](){ emit frameStep(); } },
        { 892, "Frame-step back",           [&](){ emit frameStepBack(); } },
        { 895, "Increase Rate",             [&](){ emit increaseRate(); } },
        { 894, "Decreate Rate",             [&](){ emit decreaseRate(); } },
        { 920, "Next File",                 [&](){ emit nextFile(false); } },
        { 919, "Previous File",             [&](){ emit previousFile(false); } },
        { 921, "Move to Recycle Bin",       [&](){ emit moveToRecycleBin(); } },
        { 975, "Quick add favorite",        [&](){ emit quickAddFavorite(); } },
        { 937, "Organize Favorites...",     [&](){ emit organizeFavorites(); } },
        { 817, "Toggle Caption&amp;Menu",   [&](){ emit toggleCaptionMenu(); } },
        { 818, "Toggle Seek Bar",           [&](){ emit toggleSeekBar(); } },
        { 819, "Toggle Controls",           [&](){ emit toggleControls(); } },
        { 820, "Toggle Information",        [&](){ emit toggleInformation(); } },
        { 821, "Toggle Statistics",         [&](){ emit toggleStatistics(); } },
        { 822, "Toggle Status",             [&](){ emit toggleStatus(); } },
        { 824, "Toggle Playlist Bar",       [&](){ emit togglePlaylistBar(); } },
        { 827, "View Minimal",              [&](){ emit viewMinimal(); } },
        { 828, "View Compact",              [&](){ emit viewCompact(); } },
        { 829, "View Normal",               [&](){ emit viewNormal(); } },
        { 830, "Fullscreen",                [&](){ emit fullscreen(); } },
        { 813, "Zoom 25%",                  [&](){ emit zoom25(); } },
        { 832, "Zoom 50%",                  [&](){ emit zoom50(); } },
        { 833, "Zoom 100%",                 [&](){ emit zoom100(); } },
        { 834, "Zoom 200%",                 [&](){ emit zoom200(); } },
        { 968, "Zoom Auto Fit",             [&](){ emit zoomAutoFit(); } },
        { 4900, "Zoom Auto Fit (Larger Only)", [&](){ emit zoomAutoFitLarger(); } },
        { 907, "Volume Increase",           [&](){ emit volumeUp(); } },
        { 908, "Volume Decrease",           [&](){ emit volumeDown(); } },
        { 909, "Volume Mute",               [&](){ emit volumeMute(); } },
        { 952, "Next Audio Track",          [&](){ emit nextAudioTrack(); } },
        { 953, "Prev Audio Track",          [&](){ emit previousAudioTrack(); } },
        { 954, "Next Subtitle Track",       [&](){ emit nextSubtitleTrack(); } },
        { 955, "Prev Subtitle Track",       [&](){ emit previousSubtitleTrack(); } },
        { 956, "On/Off Subtitles",          [&](){ emit onOffSubtitles(); } },
        { 948, "After Playback: Do nothing", [&](){ emit afterPlaybackDoNothing(); } },
        { 947, "After Playback: Play next file", [&](){ emit afterPlaybackPlayNext(); } },
        { 912, "After Playback: Exit",      [&](){ emit afterPlaybackExit(); } },
        { 913, "After Playback: Stand By",  [&](){ emit afterPlaybackStandBy(); } },
        { 914, "After Playback: Hibernate", [&](){ emit afterPlaybackHibernate(); } },
        { 915, "After Playback: Shutdown",  [&](){ emit afterPlaybackShutdown(); } },
        { 916, "After Playback: Log Off",   [&](){ emit afterPlaybackLogOff(); } },
        { 917, "After Playback: Lock",      [&](){ emit afterPlaybackLock(); } },
        { 14116, "Remove Selected Item",      [&](){ emit removeSelectedPlaylistItem(); } }
    });
    for (auto &c : wmCommands) {
        wmCommandsById.insert(c.id, c);
    }
}

void MpcHcServer::setupHttp()
{
    http.route("/", [](HttpRequest &req, HttpResponse &res) {
        (void)req;
        res.headers["Server"] = "MPC-HC WebServer";
        res.fallthrough = true;
    });

    http.route("/favicon.ico", [](HttpRequest &req, HttpResponse &res) {
        (void)req;
        res.serveFile(":/images/icon/mpc-qt.svg");
    });

    http.route("/browser.html", [this](HttpRequest &req, HttpResponse &res) {
        QString path;
        if (!req.getVars.contains("path")) {
            path = QFileInfo(nowPlaying.toLocalFile()).absoluteDir().path();
        } else if (req.getVars.value("path").isEmpty()) {
            path = "";
        } else {
            path = req.getVars.value("path");
            path.replace('\\', '/');
            QFileInfo info(path);
            if (info.isFile()) {
                emit fileSelected(path);
                path = info.absoluteDir().path();
            }
        }
        if (Platform::isWindows && path=="/")
            path = "C:/";

        struct DirEntry {
            QString name;
            QString type;
            QString size;
            QString date;
        };
        QList<DirEntry> entries;
        if (Platform::isWindows && path=="") {
            auto func = [&](DeviceInfo *device) -> void {
                if (device->deviceType != DeviceInfo::FixedDrive
                        && device->deviceType != DeviceInfo::OpticalDrive
                        && device->deviceType != DeviceInfo::RamDrive
                        && device->deviceType != DeviceInfo::RemoteDrive
                        && device->deviceType != DeviceInfo::RemovableDrive)
                    return;
                device->mount();
                DirEntry entry;
                entry.name = QString("<b><a href=\"browser.html?path=%1\">%1</a></b>").arg(device->mountedPath);
                entry.type = "Drive";
                entry.size = "&nbsp;";
                entry.date = dateZero;
                entries.append(entry);
            };
            Platform::deviceManager()->iterateDevices(func);
            std::sort(entries.begin(), entries.end(), [](DirEntry &a, DirEntry &b) -> bool {
                return a.name < b.name;
            });
        } else {
            QDir dir(path);
            DirEntry entry;
            if (Platform::isWindows && dir.isRoot()) {
                entry.name = QString("<b><a href=\"browser.html?path=%1\">.</a></b>").arg(path);
                entry.type = "Dir";
                entry.size = "&nbsp;";
                entry.date = dateZero;
                entries.append(entry);

                entry.name = "<b><a href=\"browser.html?path=\">..</a></b>";
                entry.type = "Dir";
                entry.size = "&nbsp;";
                entry.date = dateZero;
                entries.append(entry);
            }
            for (const auto &info : dir.entryInfoList()) {
                QString path = HttpServer::urlEncode(info.absoluteFilePath());
                if (path == "%2F..")
                    path = "%2F";
                entry.name = QString("<a href=\"browser.html?path=%1\">%2</a>").arg(path, info.fileName());
                if (info.isDir()) entry.name = QString("<b>%1</b>").arg(entry.name);
                entry.type = info.isDir() ? "Dir" : info.isFile() ? "File" : "Unknown";
                entry.size = info.isFile() ? QString("%1K").arg(info.size()/1024) : "&nbsp;";
                entry.date = info.lastModified().toString("yyyy.MM.dd hh:mm");
                entries.append(entry);
            }
        }
        QString text;
        text += QString(
"        <div>\n"
"            <table class=\"browser-table\">\n"
"                <tr>\n"
"                    <td class=\"text-center\">\n"
"                        <strong>Location: %1</strong>\n"
"                    </td>\n"
"                </tr>\n"
"            </table>\n"
"        </div>\n").arg(path);
        text +=
"        <span>&nbsp;</span>\n"
"        <div>\n"
"            <table class=\"browser-table\">\n"
"                <tr>\n"
"                    <th>Name</th>\n"
"                    <th>Type</th>\n"
"                    <th>Size</th>\n"
"                    <th>Date Modified</th>\n"
"                </tr>\n"
"                ";
        QString rowFmt = "<tr>\n"
"<td class=\"dirname\">%1</td>\n"
"<td class=\"dirtype\">%2</td>\n"
"<td class=\"dirsize\">%3</td>\n"
"<td class=\"dirdate\">%4</td>\n"
"</tr>\n";
        for (const auto &entry : entries)
            text += rowFmt.arg(entry.name, entry.type, entry.size, entry.date);
        text +=
"\n"
"            </table>\n"
"        </div>\n";
        res.serveFile(":http/browser.html");
        QString content = QString::fromUtf8(res.content);
        res.content = content.arg(text).toUtf8();
    });

    http.route("/command.html", [this](HttpRequest &req, HttpResponse &res) {
        res.redirect("/index.html");
        QString commandString = req.getVars.value("wm_command", req.postVars.value("wm_command"));
        if (commandString.isEmpty())
            return;
        int command = commandString.toInt();
        if (command > 0 && wmCommandsById.contains(command))
            wmCommandsById.value(command).func();
    });

    http.route("/info.html", [this](HttpRequest &req, HttpResponse &res) {
        (void)req;
        QString str("&laquo; %1 &bull; %2 &bull; %3/%4 &bull; %5 &raquo;");
        QString version("MPC-QT v" MPCQT_VERSION_STR);
        QString time(Helpers::toDateFormatFixed(fileTime, Helpers::ShortFormat));
        QString duration(Helpers::toDateFormatFixed(fileDuration, Helpers::ShortFormat));
        QString size(Helpers::fileSizeToStringShort(fileSize));

        res.serveFile(":/http/info.html");
        QString content = QString::fromUtf8(res.content);
        res.content = content.arg(str.arg(version, mediaTitle, time, duration, size)).toUtf8();
    });

    http.route("/variables.html", [this](HttpRequest &req, HttpResponse &res) {
        (void)req;
        QString filePath = nowPlaying.toLocalFile();
        QString filePathArg = HttpServer::urlEncode(filePath);
        QFileInfo fileInfo(filePath);
        QString file = fileInfo.fileName();
        QString fileDir = fileInfo.absoluteDir().path();
        QString fileDirArg = HttpServer::urlEncode(fileDir);
        QString state;
        QString stateString;
        switch(playbackState) {
        case PlaybackManager::StoppedState:
            state = QString::number(-1);
            stateString = "N/A";
            break;
        case PlaybackManager::PausedState:
            state = QString::number(1);
            stateString = "Paused";
            break;
        default:
            state = QString::number(2);
            stateString = "Playing";
            break;
        }
        QString position = QString::number(int64_t(fileTime*1000));
        QString positionString = Helpers::toDateFormatFixed(fileTime, Helpers::ShortFormat);
        QString duration = QString::number(int64_t(fileDuration*1000));
        QString durationString = Helpers::toDateFormatFixed(fileDuration, Helpers::ShortFormat);
        QString volumeLevel = QString::number(volume);
        QString muted = QString::number(volumeMuted ? 1 : 0);
        QString playbackRate = QString::number(this->playbackRate);
        QString size = Helpers::fileSizeToStringShort(fileSize);
        QString reloadTime = "0";
        QString version = MPCQT_VERSION_STR;

        QList<QPair<QString,QString>> variables = {
            { "file", file },
            { "filepatharg", filePathArg },
            { "filepath", filePath },
            { "filedirarg", fileDirArg },
            { "filedir", fileDir },
            { "state", state },
            { "statestring" , stateString },
            { "position", position },
            { "positionstring", positionString },
            { "duration", duration },
            { "durationstring", durationString },
            { "volumelevel", volumeLevel },
            { "muted", muted },
            { "playbackRate", playbackRate },
            { "size", size },
            { "reloadtime", reloadTime },
            { "version", version }
        };
        QString data;
        QString fmt = "       <p id=\"%1\">%2</p>\n";
        for (const auto &var : variables) {
            data.append(fmt.arg(var.first, var.second));
        }
        res.serveFile(":/http/variables.html");
        QString contents = QString::fromUtf8(res.content);
        res.content = contents.arg(data).toUtf8();
    });

    http.route("/", [this](HttpRequest &req, HttpResponse &res) {
        QString fileRoot = serveFiles ? webRoot : ":/http";
        QString fallback = serveFiles ? defaultPage : "index.html";
        if (req.url == "/")
            req.url = "/" + fallback;
        QString servedFile = fileRoot + req.url;
        res.serveFile(servedFile);
        if (res.statusCode == HttpResponse::Http404NotFound) {
            servedFile = webRoot + "/" + fallback;
            res.serveFile(servedFile);
        };
        if (servedFile == ":/http/index.html") {
            QString contents = QString::fromUtf8(res.content);
            QString actionText;
            for (auto &c : wmCommands) {
                QString optionValue = QString::number(c.id);
                actionText.append(QString("<option value=\"%1\">%2</option>\n").arg(optionValue, c.text));
            }
            res.content = contents.arg(actionText).toUtf8();
        }
    });
}
