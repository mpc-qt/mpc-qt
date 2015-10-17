// generic helper module for general-use static functions
#ifndef HELPERS_H
#define HELPERS_H
#include <QObject>
#include <QWidget>
#include <QList>
#include <QUrl>

class QFileDialog;
class QLocalServer;

namespace Helpers {
    QString toDateFormat(double time);
}

class AsyncFileDialog : public QObject {
    Q_OBJECT

public:
    enum DialogMode { SingleFile, MultipleFiles, FolderContents, Directory };

    explicit AsyncFileDialog(QWidget *parent = NULL);
    ~AsyncFileDialog();
    void setMode(DialogMode mode);
    void show();

signals:
    void filesOpened(QList<QUrl> files);
    void fileOpened(QUrl file);
    void dirOpened(QUrl dir);

private slots:
    void qfd_urlsSelected(QList<QUrl> urls);

private:
    QFileDialog *qfd;
    DialogMode mode_;
};

class SingleProcess : public QObject
{
    Q_OBJECT
public:
    explicit SingleProcess(QObject *parent = 0);
    bool hasPrevious(const QStringList &payload);

private:
    void listen();

signals:
    void payloadReceived(const QStringList &payload);

private slots:
    void server_newConnection();

private:
    QString socketName;
    QLocalServer *server;
};

#endif // HELPERS_H
