#include "playlistwindow.h"
#include "ui_playlistwindow.h"
#include "qdrawnplaylist.h"
#include "playlist.h"

PlaylistWindow::PlaylistWindow(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::PlaylistWindow)
{
    ui->setupUi(this);
    auto qdp = new QDrawnPlaylist();
    qdp->setUuid(QUuid());
    ui->tabWidget->addTab(qdp, tr("Quick Playlist"));
}

PlaylistWindow::~PlaylistWindow()
{
    delete ui;
}

QPair<QUuid, QUuid> PlaylistWindow::addToCurrentPlaylist(QList<QUrl> what)
{
    QPair<QUuid, QUuid> info;
    auto qdp = reinterpret_cast<QDrawnPlaylist *>(ui->tabWidget->currentWidget());
    auto pl = PlaylistCollection::getSingleton()->playlistOf(qdp->uuid());
    Item *firstItem = NULL;
    for (QUrl url : what) {
       auto item = pl->addItem(url);
       qdp->addItem(item->uuid());
       if (!firstItem)  firstItem = item;
    }
    if (firstItem) {
        info.first = pl->uuid();
        info.second = firstItem->uuid();
    }
    return info;
}

bool PlaylistWindow::isCurrentPlaylistEmpty()
{
    auto qdp = reinterpret_cast<QDrawnPlaylist *>(ui->tabWidget->currentWidget());
    auto pl = PlaylistCollection::getSingleton()->playlistOf(qdp->uuid());
    return pl->count() == 0;
}

void PlaylistWindow::on_newTab_clicked()
{
    auto qdp = new QDrawnPlaylist();
    auto pl = PlaylistCollection::getSingleton()->newPlaylist(tr("New Playlist"));
    qdp->setUuid(pl->uuid());
    ui->tabWidget->addTab(qdp, pl->title());
}

void PlaylistWindow::on_closeTab_clicked()
{
    int index = ui->tabWidget->currentIndex();
    on_tabWidget_tabCloseRequested(index);
}

void PlaylistWindow::on_tabWidget_tabCloseRequested(int index)
{
    auto qdp = reinterpret_cast<QDrawnPlaylist *>(ui->tabWidget->widget(index));
    if (!qdp || qdp->uuid().isNull())
        return;
    PlaylistCollection::getSingleton()->removePlaylist(qdp->uuid());
    ui->tabWidget->removeTab(index);
}

void PlaylistWindow::on_duplicateTab_clicked()
{
    auto origin = reinterpret_cast<QDrawnPlaylist *>(ui->tabWidget->currentWidget());
    auto remote = PlaylistCollection::getSingleton()->clonePlaylist(origin->uuid());
    auto qdp = new QDrawnPlaylist();
    qdp->setUuid(remote->uuid());
    ui->tabWidget->addTab(qdp, remote->title());
}
