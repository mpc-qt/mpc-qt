#ifndef MPVSERVER_H
#define MPVSERVER_H

#include <QObject>
#include <QVariant>
#include <QSharedPointer>
#include <QHash>
#include <QMetaMethod>
#include <QSize>

class QLocalServer;
class QLocalSocket;
class JsonServer : public QObject
{
    Q_OBJECT
public:
    explicit JsonServer(const QString &socketName, QObject *parent = 0);
    bool sendPayload(const QByteArray &payload);
    QString fullServerName();

private:
    void listen();

signals:
    void newConnection(QLocalSocket *socket);
    void payloadReceived(const QByteArray &payload, QLocalSocket *socket);

private slots:
    void server_newConnection();

private:
    QString socketName;
    QLocalServer *server;
};


class MainWindow;
class PlaybackManager;
class MpcQtServer : public JsonServer
{
    Q_OBJECT
public:
    explicit MpcQtServer(MainWindow *mainWindow,
                         PlaybackManager *playbackManager,
                         QObject *parent);
    void fakePayload(const QByteArray &payload);

signals:

private:
    void setupIpcCommands();
    void socketReturn(QLocalSocket *socket, bool wasParsed,
                      QVariant value = QVariant());

private slots:
    void self_newConnection(QLocalSocket *socket);
    void socket_payloadReceived(const QByteArray &payload,
                                QLocalSocket *socket);
    void ipc_playFiles(const QVariantMap &map);
    void ipc_play(const QVariantMap &map);
    void ipc_pause();
    void ipc_unpause();
    void ipc_start();
    void ipc_stop();
    void ipc_next(const QVariantMap &map);
    void ipc_previous(const QVariantMap &map);
    void ipc_repeat();
    void ipc_togglePlayback();
    QVariant ipc_getMpvProperty(const QVariantMap &map);
    QVariant ipc_setMpvProperty(const QVariantMap &map);
    QVariant ipc_setMpvOption(const QVariantMap &map);
    QVariant ipc_doMpvCommand(const QVariantMap &map);

private:
    PlaybackManager *playbackManager;
    MainWindow *mainWindow;
    QHash<QString, QMetaMethod> ipcCommands;
};


class MpvConnection;
class MpvWidget;
class MpvServer : public JsonServer
{
    Q_OBJECT
public:
    explicit MpvServer(PlaybackManager *playbackManager, MpvWidget *mpvWidget,
                       QObject *parent = 0);

private slots:
    void server_newConnection(QLocalSocket *socket);
    void connection_disconnected();

private:
    PlaybackManager *playbackManager;
    MpvWidget *mpvWidget;
    QList<QSharedPointer<MpvConnection>> connections;
};



class MpvConnection : public QObject
{
    Q_OBJECT
public:
    explicit MpvConnection(QLocalSocket *socket, PlaybackManager *manager,
                           MpvWidget *mpvWidget, QObject *parent = 0);
    ~MpvConnection();

signals:
    void disconnected();

private slots:
    void socket_readyRead();
    void socket_disconnected();
    void ctrl_mpvPropertyChanged(QString name, QVariant v, uint64_t userData);
    void ctrl_logMessage(QString message);
    void ctrl_unhandledMpvEvent(int eventNumber);
    void ctrl_videoSizeChanged(QSize size);

private:
    QLocalSocket *socket;
    PlaybackManager *manager;
    MpvWidget *mpvWidget;
};

#endif // MPVSERVER_H
