#ifndef HELPERS_H
#define HELPERS_H
#include <QString>
#include <QFileDialog>
#include <QList>

// generic helper module for general-use static functions
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

#endif // HELPERS_H
