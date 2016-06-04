#include <QLocalSocket>
#include <QLocalServer>
#include <QCoreApplication>
#include <QMetaMethod>
#include <QJsonDocument>

#include "mainwindow.h"
#include "manager.h"
#include "mpvwidget.h"
#include "ipc.h"

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
    socket->write(QJsonDocument::fromVariant(result).toJson());
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
    QSet<QString> banned = {
        "stream-open-filename", "file-local-options", "ab-loop-a",
        "ab-loop-b", "volume", "mute", "fullscreen" };
    QString name = map.value("name").toString();
    if (name.isEmpty() || banned.contains(name))
        return QVariant::fromValue(MpvErrorCode(-0xdedbeef));

    return mainWindow->mpvWidget()->blockingSetMpvPropertyVariant(name, map["value"]);
}

QVariant MpcQtServer::ipc_setMpvOption(const QVariantMap &map)
{
    QSet<QString> banned = {
        "vo", "vo-defaults", "script", "script-opts",  "wid", "input-conf",
        "input-test", "input-file", "input-terminal", "no-input-terminal",
        "input-ipc-server", "input-media-keys", "input-vo-keyboard",
        "input-app-event", "osc", "no-osc", "no-osd-bar", "osd-bar",
        "no-terminal",  "terminal" };
    QString name = map.value("name").toString();
    if (name.isEmpty() || banned.contains(name))
        return QVariant::fromValue(MpvErrorCode(-0xdedbeef));

    return mainWindow->mpvWidget()->blockingSetMpvOptionVariant(name, map["value"]);
}

QVariant MpcQtServer::ipc_doMpvCommand(const QVariantMap &map)
{
    QSet<QString> banned = {
        "openfile", "stop", "set", "add", "cycle", "multiply", "run", "quit",
        "quit-watch-later", "mouse", "keypress", "keydown", "keyup",
        "enable-section", "define-section", "script-message",
        "script-message-to", "script-binding", "vo-cmdline", "hook-add",
        "hook-ack", "set_property", "set_property_string", "enable_event",
        "suspend", "volume" };
    QString name = map.value("name").toString();
    if (name.isEmpty() || banned.contains(name))
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

}

void MpvServer::server_newConnection(QLocalSocket *socket)
{

}

void MpvServer::connection_disconnected()
{

}



MpvConnection::MpvConnection(QLocalSocket *socket, PlaybackManager *manager,
                             MpvWidget *mpvWidget, QObject *parent)
{

}

MpvConnection::~MpvConnection()
{

}

void MpvConnection::socket_readyRead()
{

}

void MpvConnection::socket_disconnected()
{

}

void MpvConnection::ctrl_mpvPropertyChanged(QString name, QVariant v,
                                            uint64_t userData)
{

}

void MpvConnection::ctrl_logMessage(QString message)
{

}

void MpvConnection::ctrl_unhandledMpvEvent(int eventNumber)
{

}

void MpvConnection::ctrl_videoSizeChanged(QSize size)
{

}
