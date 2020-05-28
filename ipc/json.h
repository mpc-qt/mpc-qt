#ifndef IPCJSON_H
#define IPCJSON_H

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
    explicit JsonServer(const QString &socketName, QObject *parent = nullptr);
    bool sendPayload(const QByteArray &payload);
    static bool sendPayload(const QByteArray &payload, const QString &serverName);
    QString fullServerName();
    void listen();

signals:
    void newConnection(QLocalSocket *socket);
    void payloadReceived(const QByteArray &payload, QLocalSocket *socket);

private slots:
    void server_newConnection();

private:
    QString socketName;
    QLocalServer *server = nullptr;
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
    static bool sendIdentify();
    void fakePayload(const QByteArray &payload);
    static QString defaultSocketName();
    void setMainWindow(MainWindow *mainWindow);
    void setPlaybackManger(PlaybackManager *playbackManager);

signals:

private:
    void setupIpcCommands();
    void socketReturn(QLocalSocket *socket, bool wasParsed,
                      QVariant value = QVariant());

private slots:
    void self_newConnection(QLocalSocket *socket);
    void socket_payloadReceived(const QByteArray &payload,
                                QLocalSocket *socket);
    void ipc_identify();
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
    void ipc_deltaExtraPlaytimes(const QVariantMap &map);
    QVariant ipc_getMpvProperty(const QVariantMap &map);
    QVariant ipc_setMpvProperty(const QVariantMap &map);
    QVariant ipc_setMpvOption(const QVariantMap &map);
    QVariant ipc_doMpvCommand(const QVariantMap &map);

private:
    PlaybackManager *playbackManager = nullptr;
    MainWindow *mainWindow = nullptr;
    QHash<QString, QMetaMethod> ipcCommands;
};


class MpvConnection;
class MpvObject;
class MpvServer : public JsonServer
{
    Q_OBJECT
public:
    explicit MpvServer(QObject *parent = nullptr);
    void setPlaybackManger(PlaybackManager *manager);
    void setMpvObject(MpvObject *object);

private slots:
    void server_newConnection(QLocalSocket *socket);

private:
    PlaybackManager *playbackManager = nullptr;
    MpvObject *mpvObject = nullptr;
};



class MpvConnection : public QObject
{
    Q_OBJECT
public:
    explicit MpvConnection(QLocalSocket *socket, PlaybackManager *manager,
                           MpvObject *mpvObject, QObject *parent = nullptr);
    ~MpvConnection();

signals:
    void disconnected(MpvConnection *self);

private:
    void socketWrite(const QVariant &v);
    void commandReturn(int errorCode, QVariant requestId, QVariant data = QVariant());
    void commandReturnVariant(const QVariant &requestId, const QVariant &data);

private slots:
    void socket_readyRead();
    void socket_disconnected();
    void ctrl_mpvPropertyChanged(QString name, const QVariant &v, uint64_t userData);
    void ctrl_logMessage(QString message);
    void ctrl_clientMessage(uint64_t id, const QStringList &args);
    void ctrl_videoSizeChanged(const QSize &size);
    void ctrl_unhandledMpvEvent(int eventNumber);

    void command_raw(const QStringList &list, const QVariant &requestId);
    void command_forbidden();
    void command_client_name(const QVariant &requestId);
    void command_get_time_us(const QVariant &requestId);
    void command_get_version(const QVariant &requestId);
    void command_get_property(const QStringList &list, const QVariant &requestId);
    void command_get_property_string(const QStringList &list, const QVariant &requestId);
    void command_set_property(const QVariantList &list, const QVariant &requestId);
    void command_set_property_string(const QStringList &list, const QVariant &requestId);
    void command_observe_property(const QVariantList &list, const QVariant &requestId);
    void command_observe_property_string(const QVariantList &list, const QVariant &requestId);
    void command_unobserve_property(const QVariantList &list, const QVariant &requestId);

private:
    QLocalSocket *socket = nullptr;
    PlaybackManager *manager = nullptr;
    MpvObject *mpvObject = nullptr;
    QMap<QString,QMetaMethod> commandParsers;
};

#endif // IPCJSON_H
