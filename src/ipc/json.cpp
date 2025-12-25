#include <QLocalSocket>
#include <QLocalServer>
#include <QMetaMethod>
#include <QJsonDocument>

#include <mpv/client.h>

#include "logger.h"
#include "mainwindow.h"
#include "manager.h"
#include "mpvwidget.h"
#include "ipc/json.h"

static constexpr char logModule[] =  "ipc";
static constexpr char serverNameJson[] = "cmdrkotori.mpc-qt";
static constexpr char serverNameMpv[] = "cmdrkotori.mpc-qt.mpv";

Q_GLOBAL_STATIC_WITH_ARGS(QSet<QString>, bannedProperties, ({
    "stream-open-filename", "file-local-options", "ab-loop-a",
    "ab-loop-b", "volume", "mute", "fullscreen"
}))


Q_GLOBAL_STATIC_WITH_ARGS(QSet<QString>, bannedOptions, ({
    "vo", "vo-defaults", "script", "script-opts",  "wid", "input-conf",
    "input-test", "input-file", "input-terminal", "no-input-terminal",
    "input-ipc-server", "input-media-keys", "input-vo-keyboard",
    "input-app-event", "osc", "no-osc", "no-osd-bar", "osd-bar",
    "no-terminal",  "terminal"
}))

Q_GLOBAL_STATIC_WITH_ARGS(QSet<QString>, bannedCommands, ({
    "openfile", "set", "add", "cycle", "multiply", "run", "quit",
    "quit-watch-later", "mouse", "keypress", "keydown", "keyup",
    "enable-section", "define-section", "script-message",
    "script-message-to", "script-binding", "vo-cmdline", "hook-add",
    "hook-ack", "set_property", "set_property_string", "enable_event",
    "suspend", "volume"
}))



JsonServer::JsonServer(const QString &socketName, QObject *parent) :
    QObject(parent)
{
    this->socketName = socketName;
}

bool JsonServer::sendPayload(const QByteArray &payload)
{
    return sendPayload(payload, socketName);
}

bool JsonServer::sendPayload(const QByteArray &payload, const QString &serverName)
{
    QLocalSocket socket;
    socket.setServerName(serverName);
    socket.connectToServer();
    if (!socket.waitForConnected(100))
        return false;
    socket.write(payload);
    return socket.waitForReadyRead(100);
}

QString JsonServer::fullServerName()
{
    if (!server)
        return QString();
    return server->fullServerName();
}

void JsonServer::listen()
{
    server = new QLocalServer(this);
    connect(server, &QLocalServer::newConnection,
            this, &JsonServer::server_newConnection);

    server->removeServer(socketName);
    server->listen(socketName);
}

void JsonServer::server_newConnection()
{
    QLocalSocket *connection = server->nextPendingConnection();
    if (connection)
        emit newConnection(connection);
}



MpcQtServer::MpcQtServer(MainWindow *mainWindow,
                         PlaybackManager *playbackManager,
                         QObject *parent)
    : JsonServer(defaultSocketName(), parent),
      playbackManager(playbackManager), mainWindow(mainWindow)
{
    setupIpcCommands();
    connect(this, &JsonServer::newConnection,
            this, &MpcQtServer::self_newConnection);
}

bool MpcQtServer::sendIdentify()
{
    return JsonServer::sendPayload(QString("{ \"command\": \"identify\" }").toUtf8(),
                                   defaultSocketName());
}

void MpcQtServer::fakePayload(const QByteArray &payload)
{
    socket_payloadReceived(payload, nullptr);
}

QString MpcQtServer::defaultSocketName()
{
    return serverNameJson;
}

void MpcQtServer::setMainWindow(MainWindow *mainWindow)
{
    this->mainWindow = mainWindow;
}

void MpcQtServer::setPlaybackManger(PlaybackManager *playbackManager)
{
    this->playbackManager = playbackManager;
}

void MpcQtServer::setupIpcCommands()
{
    for (int i = 0; i < metaObject()->methodCount(); ++i) {
        QMetaMethod method(metaObject()->method(i));
        QString name(method.name());
        if (name.startsWith("ipc_"))
            ipcCommands[name.mid(4)] = method;
    }
}

void MpcQtServer::socketReturn(QLocalSocket *socket,
                               bool wasParsed, QVariant value)
{
    if (!socket)
        return;

    QVariantMap result;
    if (!wasParsed) {
        result["code"] = "unknown";
        goto end;
    }
    if (value.canConvert<MpvErrorCode>()) {
        result["code"]= "error";
        value = value.value<MpvErrorCode>().errorcode();
    } else {
        result["code"] = "ok";
    }
    result["value"] = value;
    end:
    socket->write(QJsonDocument::fromVariant(result).toJson(QJsonDocument::Compact).append('\n'));
    socket->flush();
    socket->deleteLater();
}

void MpcQtServer::self_newConnection(QLocalSocket *socket)
{
    if (!mainWindow || !playbackManager) {
        socket->deleteLater();
        return;
    }

    connect(socket, &QLocalSocket::readyRead, this, [this, socket]() {
        QList<QByteArray> dataList = socket->readAll().split('\n');
        for (const QByteArray &data : std::as_const(dataList)) {
            if(data.size())
                socket_payloadReceived(data, socket);
        }
    });
}

void MpcQtServer::socket_payloadReceived(const QByteArray &payload,
                                         QLocalSocket *socket)
{
    QJsonParseError parseError;
    QVariantMap map = QJsonDocument::fromJson(payload, &parseError).toVariant().toMap();

    if (!map.contains("command"))
        socketReturn(socket, false);

    QString command = map["command"].toString();
    QVariant value;
    if (ipcCommands.contains(command)) {
        QMetaMethod method = ipcCommands[command];
        if (ipcCommands[command].returnType() == QMetaType::QVariant)
            method.invoke(this, Q_RETURN_ARG(QVariant, value),
                                Q_ARG(QVariantMap, map));
        else if (method.parameterCount())
            method.invoke(this, Q_ARG(QVariantMap,map));
        else
            method.invoke(this);
        socketReturn(socket, true, value);
    } else {
        socketReturn(socket, false);
    }
}

void MpcQtServer::ipc_identify()
{
    // do nothing!
}

void MpcQtServer::ipc_playFiles(const QVariantMap &map)
{
    QString workingDirectory = map["directory"].toString();
    QStringList filesAsText = map["files"].toStringList();
    bool important = !map.value("append", false).toBool();
    QList<QUrl> files;
    for (const QString &s : std::as_const(filesAsText)) {
        files << QUrl::fromUserInput(s, workingDirectory);
    }
    if (!files.empty()) {
        playbackManager->openSeveralFiles(files, important);
    }
}

void MpcQtServer::ipc_play(const QVariantMap &map)
{
    QUrl url = map["file"].toString();
    if (!url.isEmpty())
        playbackManager->openFile(url);
}

void MpcQtServer::ipc_pause()
{
    playbackManager->pausePlayer();
}

void MpcQtServer::ipc_unpause()
{
    playbackManager->unpausePlayer();
}

void MpcQtServer::ipc_start()
{
    playbackManager->startPlayer();
}

void MpcQtServer::ipc_stop()
{
    playbackManager->stopPlayer();
}

void MpcQtServer::ipc_next(const QVariantMap &map)
{
    if (playbackManager->playbackState() != PlaybackManager::StoppedState)
        playbackManager->playNext(false);
    else if (map.value("autostart", false).toBool())
        ipc_start();
    else
        mainWindow->playlistWindow()->activateNext();
}

void MpcQtServer::ipc_previous(const QVariantMap &map)
{
    if (playbackManager->playbackState() != PlaybackManager::StoppedState)
        playbackManager->playPrev(false);
    else if (map.value("autostart", false).toBool())
        ipc_start();
    else
        mainWindow->playlistWindow()->activatePrevious();
}

void MpcQtServer::ipc_repeat()
{
    playbackManager->repeatThisFile();
}

void MpcQtServer::ipc_togglePlayback()
{
    switch (playbackManager->playbackState()) {
    case PlaybackManager::StoppedState:
        ipc_start();
        break;
    case PlaybackManager::PausedState:
        playbackManager->unpausePlayer();
        break;
    default:
        playbackManager->pausePlayer();
    }
}

void MpcQtServer::ipc_deltaExtraPlaytimes(const QVariantMap &map)
{
    int delta = map.value("value", 1).toInt();
    playbackManager->deltaExtraPlaytimes(delta);
}

QVariant MpcQtServer::ipc_getMpvProperty(const QVariantMap &map)
{
    if (!map.contains("name"))
        return QVariant::fromValue(MpvErrorCode(-0xdedbeef));
    return mainWindow->mpvObject()->getMpvPropertyVariant(map["name"].toString());
}

QVariant MpcQtServer::ipc_setMpvProperty(const QVariantMap &map)
{
    QString name = map.value("name").toString();
    if (name.isEmpty() || bannedProperties->contains(name))
        return QVariant::fromValue(MpvErrorCode(-0xdedbeef));

    return mainWindow->mpvObject()->blockingSetMpvPropertyVariant(name, map["value"]);
}

QVariant MpcQtServer::ipc_setMpvOption(const QVariantMap &map)
{
    QString name = map.value("name").toString();
    if (name.isEmpty() || bannedOptions->contains(name))
        return QVariant::fromValue(MpvErrorCode(-0xdedbeef));

    return mainWindow->mpvObject()->blockingSetMpvOptionVariant(name, map["value"]);
}

QVariant MpcQtServer::ipc_doMpvCommand(const QVariantMap &map)
{
    QString name = map.value("name").toString();
    if (name.isEmpty() || bannedCommands->contains(name))
        return QVariant::fromValue(MpvErrorCode(-0xdedbeef));

    QVariantList command = { name };
    QVariant options = map.value("options");
    if (options.isNull())
        goto end;
    else if (options.canConvert<QVariantList>())
        command.append(options.toList());
    else
        command.append(options);
    end:
    return mainWindow->mpvObject()->blockingMpvCommand(QVariant(command));
}


MpvServer::MpvServer(QObject *parent)
    : JsonServer(serverNameMpv, parent)
{
    connect(this, &MpvServer::newConnection,
            this, &MpvServer::server_newConnection);
}

void MpvServer::setPlaybackManger(PlaybackManager *manager)
{
    playbackManager = manager;
}

void MpvServer::setMpvObject(MpvObject *object)
{
    mpvObject = object;
}

void MpvServer::server_newConnection(QLocalSocket *socket)
{
    if (!playbackManager || !mpvObject) {
        socket->deleteLater();
        return;
    }

    Logger::log(logModule, "new mpv connection");
    new MpvConnection(socket, playbackManager, mpvObject, this);
}

MpvConnection::MpvConnection(QLocalSocket *socket, PlaybackManager *manager,
                             MpvObject *mpvObject, QObject *parent)
    : QObject(parent), socket(socket), manager(manager), mpvObject(mpvObject)
{
    MpvController* ctrl = mpvObject->controller();
    connect(ctrl, &MpvController::mpvPropertyChanged,
            this, &MpvConnection::ctrl_mpvPropertyChanged);
    connect(ctrl, &MpvController::clientMessage,
            this, &MpvConnection::ctrl_clientMessage);
    connect(ctrl, &MpvController::videoSizeChanged,
            this, &MpvConnection::ctrl_videoSizeChanged);
    connect(ctrl, &MpvController::unhandledMpvEvent,
            this, &MpvConnection::ctrl_unhandledMpvEvent);

    int methodCount = metaObject()->methodCount();
    for (int i = 0; i < methodCount; i++) {
        auto method = metaObject()->method(i);
        if (method.name().indexOf("command_") == 0)
            commandParsers.insert(method.name().mid(8), method);
    }
    commandParsers.remove("raw");

    connect(socket, &QLocalSocket::readyRead,
            this, &MpvConnection::socket_readyRead);
    connect(socket, &QLocalSocket::disconnected,
            this, &MpvConnection::socket_disconnected);
    if (socket->bytesAvailable())
        socket_readyRead();

}

MpvConnection::~MpvConnection()
{

}

void MpvConnection::socketWrite(const QVariant &v)
{
    socket->write(QJsonDocument::fromVariant(v).toJson(QJsonDocument::Compact).append('\n'));
    socket->flush();
}

void MpvConnection::commandReturn(int errorCode, QVariant requestId, QVariant data)
{
    QVariantMap map {
        { "error", mpv_error_string(errorCode) },
        { "data", data }
    };
    if (requestId.isValid())
        map.insert("request_id", requestId);
    socketWrite(map);
}

void MpvConnection::commandReturnVariant(const QVariant &requestId, const QVariant &data)
{
    if (data.canConvert<MpvErrorCode>())
        commandReturn(data.value<MpvErrorCode>().errorcode(), requestId);
    else
        commandReturn(MPV_ERROR_SUCCESS, requestId, data);
}

void MpvConnection::socket_readyRead()
{
    QList<QByteArray> dataList = socket->readAll().split('\n');
    for(QByteArray &data : dataList) {
        if(!data.size())
            continue;

        QVariantMap rawCommand = QJsonDocument::fromJson(data).toVariant().toMap();
        QVariant requestId = rawCommand["request_id"];

        QStringList list = rawCommand["command"].toStringList();
        if (list.isEmpty()) {
            commandReturn(MPV_ERROR_UNSUPPORTED, requestId);
            return;
        }

        QString command = list.at(0);
        if (commandParsers.contains(command)) {
            QMetaMethod m = commandParsers[command];
            switch (m.parameterCount()) {
            case 2:
                if (Q_UNLIKELY(m.parameterType(0) == QMetaType::QVariantList))
                    m.invoke(this, Q_ARG(QVariantList, rawCommand["command"].toList()),
                            Q_ARG(QVariant, requestId));
                else
                    m.invoke(this, Q_ARG(QStringList, list),
                             Q_ARG(QVariant, requestId));
                break;
            case 1:
                m.invoke(this, Q_ARG(QVariant, requestId));
                break;
            case 0:
                m.invoke(this);
                break;
            }
        }
        else if (bannedCommands->contains(command))
            command_forbidden();
        else
            command_raw(list, requestId);
    }
}

void MpvConnection::socket_disconnected()
{
    deleteLater();
}

void MpvConnection::ctrl_mpvPropertyChanged(QString name, const QVariant &v,
                                            uint64_t userData)
{
    if (!userData)
        return;

    QVariantMap map {
        { "event", mpv_event_name(MPV_EVENT_PROPERTY_CHANGE) },
        { "name", name },
        { "id", static_cast<unsigned long long>(userData) }
    };
    if (v.canConvert<MpvErrorCode>()) {
        map.insert("error", mpv_error_string(v.value<MpvErrorCode>().errorcode()));
        map.insert("data", QVariant());
    } else {
        map.insert("data", v);
    }
    socketWrite(map);
}

void MpvConnection::ctrl_logMessage(QString message)
{
    // no connection -- if you're parsing log messages for anything useful,
    // you're insane.  Doesn't mean I may not get to it later though.
    Q_UNUSED(message)
}

void MpvConnection::ctrl_clientMessage(uint64_t id, const QStringList &args)
{
    QVariantMap map {
        { "event", mpv_event_name(MPV_EVENT_CLIENT_MESSAGE) },
        { "id", static_cast<unsigned long long>(id) },
        { "args", args }
    };
    socketWrite(map);
}

void MpvConnection::ctrl_videoSizeChanged(const QSize &size)
{
    Q_UNUSED(size)
    QVariantMap map {
        { "event", mpv_event_name(MPV_EVENT_VIDEO_RECONFIG) },
    };
    socketWrite(map);
}

void MpvConnection::ctrl_unhandledMpvEvent(int eventNumber)
{
    QVariantMap map {
        { "event", mpv_event_name(static_cast<mpv_event_id>(eventNumber)) }
    };
    socketWrite(map);
}

void MpvConnection::command_raw(const QStringList &list, const QVariant &requestId)
{
    QVariant v = mpvObject->controller()->command(list);
    commandReturn(v.value<MpvErrorCode>().errorcode(), requestId);
}

void MpvConnection::command_forbidden()
{
    QVariantMap map {
        { "error", mpv_error_string(MPV_ERROR_UNSUPPORTED) }
    };
    socketWrite(map);
}

void MpvConnection::command_client_name(const QVariant &requestId)
{
    commandReturn(MPV_ERROR_SUCCESS, requestId, mpvObject->controller()->clientName());
}

void MpvConnection::command_get_time_us(const QVariant &requestId)
{
    commandReturn(MPV_ERROR_SUCCESS, requestId,
                  static_cast<long long>(mpvObject->controller()->timeMicroseconds()));
}

void MpvConnection::command_get_version(const QVariant &requestId)
{
    commandReturn(MPV_ERROR_SUCCESS, requestId,
                  static_cast<long long>(mpvObject->controller()->apiVersion()));
}

void MpvConnection::command_get_property(const QStringList &list,
                                         const QVariant &requestId)
{
    if (list.count() != 2 || list.at(1).isEmpty()) {
        commandReturn(MPV_ERROR_INVALID_PARAMETER, requestId);
        return;
    }
    commandReturnVariant(requestId, mpvObject->controller()->getPropertyVariant(list.at(1)));
}

void MpvConnection::command_get_property_string(const QStringList &list,
                                                const QVariant &requestId)
{
    if (list.count() != 2) {
        commandReturn(MPV_ERROR_INVALID_PARAMETER, requestId);
        return;
    }
    QString s = mpvObject->controller()->getPropertyString(list.at(1));
    if (s.isEmpty())
        commandReturn(MPV_ERROR_INVALID_PARAMETER, requestId, QVariant(static_cast<const char*>(nullptr)));
    else
        commandReturn(MPV_ERROR_SUCCESS, requestId, s);
}

void MpvConnection::command_set_property(const QVariantList &list,
                                         const QVariant &requestId)
{
    if (list.count() != 2
            || !list.at(1).canConvert<QString>()
            || bannedProperties->contains(list.at(1).toString()))
        commandReturn(MPV_ERROR_INVALID_PARAMETER, requestId);
    else
        commandReturn(mpvObject->controller()->setPropertyVariant(list.at(1).toString(), list.at(2)), requestId);
}

void MpvConnection::command_set_property_string(const QStringList &list,
                                                const QVariant &requestId)
{
    if (list.count() != 2)
        commandReturn(MPV_ERROR_INVALID_PARAMETER, requestId);
    else
        commandReturn(mpvObject->controller()->setPropertyString(list.at(1), list.at(2)), requestId);
}

void MpvConnection::command_observe_property(const QVariantList &list,
                                             const QVariant &requestId)
{
    uint64_t id;
    if (list.count() != 3
            || (id = list.at(1).toULongLong())==0
            || !list.at(2).canConvert<QString>()) {
        commandReturn(MPV_ERROR_INVALID_PARAMETER, requestId);
        return;
    }
    MpvController::PropertyList property = {
        { list[2].toString(), id, MPV_FORMAT_NODE }
    };
    commandReturn(mpvObject->controller()->observeProperties(property), requestId);
}

void MpvConnection::command_observe_property_string(const QVariantList &list,
                                                    const QVariant &requestId)
{
    uint64_t id;
    if (list.count() != 3
            || (id = list.at(1).toULongLong())==0
            || !list.at(2).canConvert<QString>())
        commandReturn(MPV_ERROR_INVALID_PARAMETER, requestId);
    else
        commandReturn(mpvObject->controller()->observeProperties({{list[2].toString(), id, MPV_FORMAT_STRING}}), requestId);
}

void MpvConnection::command_unobserve_property(const QVariantList &list,
                                               const QVariant &requestId)
{
    uint64_t id;
    if (list.count() != 2 || (id = list.at(1).toULongLong())==0)
        commandReturn(MPV_ERROR_INVALID_PARAMETER, requestId);
    else
        commandReturn(mpvObject->controller()->unobservePropertiesById(QSet<uint64_t>() << id), requestId);
}

