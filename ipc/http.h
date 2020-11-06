#ifndef IPCHTTP_H
#define IPCHTTP_H

#include <functional>
#include <QDateTime>
#include <QMap>
#include <QTcpServer>

class HttpRequest {
public:
    QString method;
    QString url;
    QMap<QString,QString> headers;
    QMap<QString,QString> getVars;
    QMap<QString,QString> postVars;
};

class HttpResponse {
public:
    HttpResponse();

    enum HttpStatus {
        Http200Ok = 200,
        Http204NoContent = 204,
        Http301MovedPermanently = 301,
        Http400BadRequest = 400,
        Http401Unauthorized = 401,
        Http403Forbidden = 403,
        Http404NotFound = 404,
        Http500InternalServerError = 500,
        Http501NotImplemented = 501,
    };

    HttpStatus statusCode;
    QDateTime dateResponse;
    QDateTime dateModified;
    QString contentType;
    QByteArray content;

    QString statusLine;
    QMap<QString,QString> headers;

    bool fallthrough;

    void fillHeaders();
    void serveFile(QString filename);
};


class HttpServer : public QTcpServer
{
    Q_OBJECT
public:
    HttpServer(QObject *owner);
    void clearRoutes();
    void route(QString path, std::function<void(HttpRequest&, HttpResponse&)> callback);
    static QString extensionToContentType(QString extension);
    static QString urlDecode(QString urlText, bool plusToSpace = false);
    static QString urlEncode(QString plainText);

private:
    void sendSocket(QTcpSocket *sock, HttpResponse &res);

private slots:
    void self_newConnection();
    void socket_readyRead(QTcpSocket *sock);

private:
    QList<QPair<QString, std::function<void(HttpRequest&, HttpResponse&)>>> routeMap;
};

#endif // IPCHTTP_H
