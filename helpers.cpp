#include <QFileDialog>
#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
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

