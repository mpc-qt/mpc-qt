#include <QLocalSocket>
#include <QLocalServer>
#include <QCoreApplication>
#include <QMetaMethod>
#include <QJsonDocument>

#include <mpv/client.h>

#include "mainwindow.h"
#include "manager.h"
#include "mpvwidget.h"
#include "ipc.h"



static QSet<QString> bannedProperties {
    "stream-open-filename", "file-local-options", "ab-loop-a",
    "ab-loop-b", "volume", "mute", "fullscreen"
};

static QSet<QString> bannedOptions {
    "vo", "vo-defaults", "script", "script-opts",  "wid", "input-conf",
    "input-test", "input-file", "input-terminal", "no-input-terminal",
    "input-ipc-server", "input-media-keys", "input-vo-keyboard",
    "input-app-event", "osc", "no-osc", "no-osd-bar", "osd-bar",
    "no-terminal",  "terminal"
};

static QSet<QString> bannedCommands {
    "openfile", "set", "add", "cycle", "multiply", "run", "quit",
    "quit-watch-later", "mouse", "keypress", "keydown", "keyup",
    "enable-section", "define-section", "script-message",
    "script-message-to", "script-binding", "vo-cmdline", "hook-add",
    "hook-ack", "set_property", "set_property_string", "enable_event",
    "suspend", "volume"
};



JsonServer::JsonServer(const QString &socketName, QObject *parent) :
    QObject(parent)
{
    this->socketName = socketName;
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

QString JsonServer::fullServerName()
{
    if (!server)
        return QString();
    return server->fullServerName();
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
    emit newConnection(server->nextPendingConnection());
}



MpcQtServer::MpcQtServer(MainWindow *mainWindow,
                         PlaybackManager *playbackManager,
                         QObject *parent)
    : JsonServer(QCoreApplication::organizationDomain(), parent),
      playbackManager(playbackManager), mainWindow(mainWindow)
{
    setupIpcCommands();
    connect(this, &JsonServer::newConnection,
            this, &MpcQtServer::self_newConnection);
}

void MpcQtServer::fakePayload(const QByteArray &payload)
{
    socket_payloadReceived(payload, NULL);
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
    QString code;
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
    connect(socket, &QLocalSocket::readyRead, [=]() {
        QByteArray data = socket->readAll();
        socket_payloadReceived(data, socket);
    });
}

void MpcQtServer::socket_payloadReceived(const QByteArray &payload,
                                         QLocalSocket *socket)
{
    QJsonParseError parseError;
    QVariantMap map = QJsonDocument::fromJson(payload, &parseError).toVariant().toMap();

    if (!map.contains("command"))
        return;
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

void MpcQtServer::ipc_playFiles(const QVariantMap &map)
{
    QString workingDirectory = map["directory"].toString();
    QStringList filesAsText = map["files"].toStringList();
    QList<QUrl> files;
    QUrl url;
    foreach (QString s, filesAsText) {
        files << QUrl::fromUserInput(s, workingDirectory);
    }
    if (!files.empty()) {
        playbackManager->openSeveralFiles(files, true);
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
    if (!mainWindow->playlistWindow()->playActiveItem())
        mainWindow->playlistWindow()->playCurrentItem();
}

void MpcQtServer::ipc_stop()
{
    playbackManager->stopPlayer();
}

void MpcQtServer::ipc_next(const QVariantMap &map)
{
    if (playbackManager->playbackState() != PlaybackManager::StoppedState)
        playbackManager->playNextFile();
    else if (map.value("autostart", false).toBool())
        ipc_start();
    else
        mainWindow->playlistWindow()->activateNext();
}

void MpcQtServer::ipc_previous(const QVariantMap &map)
{
    if (playbackManager->playbackState() != PlaybackManager::StoppedState)
        playbackManager->playPrevFile();
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

QVariant MpcQtServer::ipc_getMpvProperty(const QVariantMap &map)
{
    if (!map.contains("name"))
        return QVariant::fromValue(MpvErrorCode(-0xdedbeef));
    return mainWindow->mpvWidget()->getMpvPropertyVariant(map["name"].toString());
}

QVariant MpcQtServer::ipc_setMpvProperty(const QVariantMap &map)
{
    QString name = map.value("name").toString();
    if (name.isEmpty() || bannedProperties.contains(name))
        return QVariant::fromValue(MpvErrorCode(-0xdedbeef));

    return mainWindow->mpvWidget()->blockingSetMpvPropertyVariant(name, map["value"]);
}

QVariant MpcQtServer::ipc_setMpvOption(const QVariantMap &map)
{
    QString name = map.value("name").toString();
    if (name.isEmpty() || bannedOptions.contains(name))
        return QVariant::fromValue(MpvErrorCode(-0xdedbeef));

    return mainWindow->mpvWidget()->blockingSetMpvOptionVariant(name, map["value"]);
}

QVariant MpcQtServer::ipc_doMpvCommand(const QVariantMap &map)
{
    QString name = map.value("name").toString();
    if (name.isEmpty() || bannedCommands.contains(name))
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
    return mainWindow->mpvWidget()->blockingMpvCommand(QVariant(command));
}


MpvServer::MpvServer(PlaybackManager *playbackManager, MpvWidget *mpvWidget,
                     QObject *parent)
    : JsonServer(QCoreApplication::organizationDomain() + ".mpv", parent),
      playbackManager(playbackManager), mpvWidget(mpvWidget)
{
    connect(this, &MpvServer::newConnection,
            this, &MpvServer::server_newConnection);
    listen();
}

void MpvServer::server_newConnection(QLocalSocket *socket)
{
    qDebug() << "new connection";
    new MpvConnection(socket, playbackManager, mpvWidget, this);
}

MpvConnection::MpvConnection(QLocalSocket *socket, PlaybackManager *manager,
                             MpvWidget *mpvWidget, QObject *parent)
    : QObject(parent), socket(socket), manager(manager), mpvWidget(mpvWidget)
{
    MpvController* ctrl = mpvWidget->controller();
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
        emit socket_readyRead();

}

MpvConnection::~MpvConnection()
{
    socket->deleteLater();
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
    QByteArray data = socket->readAll();
    QVariantMap rawCommand = QJsonDocument::fromJson(data).toVariant().toMap();
    QVariant requestId = rawCommand["request_id"];
    qDebug() << QJsonDocument::fromBinaryData(data);

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
    else if (bannedCommands.contains(command))
        command_forbidden();
    else
        command_raw(list, requestId);
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
    Q_UNUSED(message);
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
    Q_UNUSED(size);
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
    QVariant v = mpvWidget->controller()->command(list);
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
    commandReturn(MPV_ERROR_SUCCESS, requestId, mpvWidget->controller()->clientName());
}

void MpvConnection::command_get_time_us(const QVariant &requestId)
{
    commandReturn(MPV_ERROR_SUCCESS, requestId,
                  static_cast<long long>(mpvWidget->controller()->timeMicroseconds()));
}

void MpvConnection::command_get_version(const QVariant &requestId)
{
    commandReturn(MPV_ERROR_SUCCESS, requestId,
                  static_cast<long long>(mpvWidget->controller()->apiVersion()));
}

void MpvConnection::command_get_property(const QStringList &list,
                                         const QVariant &requestId)
{
    if (list.count() != 2 || list.at(1).isEmpty()) {
        commandReturn(MPV_ERROR_INVALID_PARAMETER, requestId);
        return;
    }
    commandReturnVariant(requestId, mpvWidget->controller()->getPropertyVariant(list.at(1)));
}

void MpvConnection::command_get_property_string(const QStringList &list,
                                                const QVariant &requestId)
{
    if (list.count() != 2) {
        commandReturn(MPV_ERROR_INVALID_PARAMETER, requestId);
        return;
    }
    QString s = mpvWidget->controller()->getPropertyString(list.at(1));
    if (s.isEmpty())
        commandReturn(MPV_ERROR_INVALID_PARAMETER, requestId, QVariant(static_cast<char*>(NULL)));
    else
        commandReturn(MPV_ERROR_SUCCESS, requestId, s);
}

void MpvConnection::command_set_property(const QVariantList &list,
                                         const QVariant &requestId)
{
    if (list.count() != 2
            || !list.at(1).canConvert<QString>()
            || bannedProperties.contains(list.at(1).toString()))
        commandReturn(MPV_ERROR_INVALID_PARAMETER, requestId);
    else
        commandReturn(mpvWidget->controller()->setPropertyVariant(list.at(1).toString(), list.at(2)), requestId);
}

void MpvConnection::command_set_property_string(const QStringList &list,
                                                const QVariant &requestId)
{
    if (list.count() != 2)
        commandReturn(MPV_ERROR_INVALID_PARAMETER, requestId);
    else
        commandReturn(mpvWidget->controller()->setPropertyString(list.at(1), list.at(2)), requestId);
}

void MpvConnection::command_observe_property(const QVariantList &list,
                                             const QVariant &requestId)
{
    uint64_t id;
    if (list.count() != 3
            || (id = list.at(1).toInt())==0
            || !list.at(2).canConvert<QString>()) {
        commandReturn(MPV_ERROR_INVALID_PARAMETER, requestId);
        return;
    }
    MpvController::PropertyList property = {
        { list[2].toString(), id, MPV_FORMAT_NODE }
    };
    commandReturn(mpvWidget->controller()->observeProperties(property), requestId);
}

void MpvConnection::command_observe_property_string(const QVariantList &list,
                                                    const QVariant &requestId)
{
    uint64_t id;
    if (list.count() != 3
            || (id = list.at(1).toInt())==0
            || !list.at(2).canConvert<QString>())
        commandReturn(MPV_ERROR_INVALID_PARAMETER, requestId);
    else
        commandReturn(mpvWidget->controller()->observeProperties({{list[2].toString(), id, MPV_FORMAT_STRING}}), requestId);
}

void MpvConnection::command_unobserve_property(const QVariantList &list,
                                               const QVariant &requestId)
{
    uint64_t id;
    if (list.count() != 2 || (id = list.at(1).toInt())==0)
        commandReturn(MPV_ERROR_INVALID_PARAMETER, requestId);
    else
        commandReturn(mpvWidget->controller()->unobservePropertiesById(QSet<uint64_t>() << id), requestId);
}

