#include "playlistwindow.h"
#include "ui_playlistwindow.h"
#include "qdrawnplaylist.h"
#include "playlist.h"

PlaylistWindow::PlaylistWindow(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::PlaylistWindow)
{
    ui->setupUi(this);
    addNewTab(QUuid(), tr("Quick Playlist"));
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

QPair<QUuid, QUuid> PlaylistWindow::urlToQuickPlaylist(QUrl what)
{
    auto pl = PlaylistCollection::getSingleton()->playlistOf(QUuid());
    pl->clear();
    widgets[QUuid()]->clear();
    ui->tabWidget->setCurrentWidget(widgets[QUuid()]);
    return addToCurrentPlaylist(QList<QUrl>() << what);
}

bool PlaylistWindow::isCurrentPlaylistEmpty()
{
    auto qdp = reinterpret_cast<QDrawnPlaylist *>(ui->tabWidget->currentWidget());
    auto pl = PlaylistCollection::getSingleton()->playlistOf(qdp->uuid());
    return pl->count() == 0;
}

QUuid PlaylistWindow::getItemAfter(QUuid list, QUuid item)
{
    auto pl = PlaylistCollection::getSingleton()->playlistOf(list);
    if (!pl)
        return QUuid();
    auto after = pl->itemAfter(item);
    if (!after)
        return QUuid();
    return after->uuid();
}

QUrl PlaylistWindow::getUrlOf(QUuid list, QUuid item)
{
    auto pl = PlaylistCollection::getSingleton()->playlistOf(list);
    if (!pl)
        return QUrl();
    auto i = pl->itemOf(item);
    if (!i)
        return QUrl();
    return i->url();
}

void PlaylistWindow::addNewTab(QUuid playlist, QString title)
{
    auto qdp = new QDrawnPlaylist();
    qdp->setUuid(playlist);
    connect(qdp, &QDrawnPlaylist::itemDesired, this, &PlaylistWindow::itemDesired);
    widgets.insert(playlist, qdp);
    ui->tabWidget->addTab(qdp, title);
}

void PlaylistWindow::changePlaylistSelection(QUuid playlistUuid, QUuid itemUuid)
{
    if (!widgets.contains(playlistUuid))
        return;
    auto qdp = widgets[playlistUuid];
    auto pl = PlaylistCollection::getSingleton()->playlistOf(playlistUuid);
    qdp->setCurrentRow(pl->indexOf(itemUuid));
}

void PlaylistWindow::on_newTab_clicked()
{
    auto pl = PlaylistCollection::getSingleton()->newPlaylist(tr("New Playlist"));
    addNewTab(pl->uuid(), pl->title());
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
    widgets.remove(qdp->uuid());
    ui->tabWidget->removeTab(index);
}

void PlaylistWindow::on_duplicateTab_clicked()
{
    auto origin = reinterpret_cast<QDrawnPlaylist *>(ui->tabWidget->currentWidget());
    auto remote = PlaylistCollection::getSingleton()->clonePlaylist(origin->uuid());
    addNewTab(remote->uuid(), remote->title());
}
