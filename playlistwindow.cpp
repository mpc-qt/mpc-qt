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
