#ifndef PLAYLISTWINDOW_H
#define PLAYLISTWINDOW_H

#include <QDockWidget>
#include <QHash>
#include <QUuid>

namespace Ui {
class PlaylistWindow;
}

class QDrawnPlaylist;

class PlaylistWindow : public QDockWidget
{
    Q_OBJECT

public:
    explicit PlaylistWindow(QWidget *parent = 0);
    ~PlaylistWindow();

    void setCurrentPlaylist(QUuid what);
    void clearPlaylist(QUuid what);
    QPair<QUuid, QUuid> addToCurrentPlaylist(QList<QUrl> what);
    QPair<QUuid, QUuid> urlToQuickPlaylist(QUrl what);
    bool isCurrentPlaylistEmpty();
    QUuid getItemAfter(QUuid list, QUuid item);
    QUrl getUrlOf(QUuid list, QUuid item);

    QVariantList tabsToVList() const;
    void tabsFromVList(const QVariantList &qvl);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

private:
    void addNewTab(QUuid playlist, QString title);

signals:
    void itemDesired(QUuid playlistUuid, QUuid itemUuid);
    void importPlaylist(QString fname);
    void exportPlaylist(QString fname, QStringList items);

public slots:
    void changePlaylistSelection(QUuid playlistUuid, QUuid itemUuid);
    void addSimplePlaylist(QStringList data);

private slots:
    void on_newTab_clicked();

    void on_closeTab_clicked();

    void on_tabWidget_tabCloseRequested(int index);

    void on_duplicateTab_clicked();

    void on_tabWidget_tabBarDoubleClicked(int index);

    void on_importList_clicked();

    void on_exportList_clicked();

    void on_tabWidget_customContextMenuRequested(const QPoint &pos);

private:
    Ui::PlaylistWindow *ui;

    QHash<QUuid, QDrawnPlaylist*> widgets;
};

#endif // PLAYLISTWINDOW_H
