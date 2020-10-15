#ifndef LIBRARYWINDOW_H
#define LIBRARYWINDOW_H

#include <QUuid>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class LibraryWindow;
}
QT_END_NAMESPACE

class DrawnPlaylist;
class DrawnCollection;

class LibraryWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LibraryWindow(QWidget *parent = nullptr);
    ~LibraryWindow();

public slots:
    void refreshLibrary();

signals:
    void windowClosed();
    void playlistRestored(QUuid playlistUuid);

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void on_restorePlaylist_clicked();

    void on_removePlaylist_clicked();

private:
    Ui::LibraryWindow *ui;
    DrawnPlaylist *playlistWidget;
    DrawnCollection *collectionWidget;
};

#endif // LIBRARYWINDOW_H
