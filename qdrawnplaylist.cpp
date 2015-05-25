#include <QApplication>
#include <QPainter>
#include "qdrawnplaylist.h"
#include "playlist.h"

PlayPainter::PlayPainter(QObject *parent) : QAbstractItemDelegate(parent) {}

void PlayPainter::paint(QPainter *painter, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const
{
    auto playWidget = qobject_cast<QDrawnPlaylist*>(parent());
    Playlist *p = PlaylistCollection::getSingleton()->
                      playlistOf(playWidget->uuid());
    if (p == NULL)
        return;
    Item *i = p->itemOf(QUuid(index.data(Qt::DisplayRole).toString()));
    if (i == NULL)
        return;

    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &option,
                                       painter);
    QRect rc = option.rect.adjusted(3,0,-3,0);
    if (i->uuid() == playWidget->nowPlayingItem()) {
         QFont f = playWidget->font();
         f.setBold(true);
         painter->setFont(f);
         painter->setPen(playWidget->palette().link().color());
         painter->drawText(rc, Qt::AlignLeft|Qt::AlignVCenter,
                           i->toDisplayString());
         painter->setFont(playWidget->font());
    } else {
         painter->setPen(playWidget->palette().text().color());
         QApplication::style()->drawItemText(painter, rc,
                                             Qt::AlignLeft|Qt::AlignVCenter,
                                             playWidget->palette(), true,
                                             i->toDisplayString());
    }
}

QSize PlayPainter::sizeHint(const QStyleOptionViewItem &option,
                            const QModelIndex &index) const
{
    Q_UNUSED(index);
    return QApplication::style()->sizeFromContents(QStyle::CT_ItemViewItem,
                                                   &option,
                                                   option.rect.size());
}

PlayItem::PlayItem(QListWidget *parent, int type)
    : QListWidgetItem(parent, type)
{

}

PlayItem::~PlayItem()
{

}

QUuid PlayItem::uuid()
{
    return uuid_;
}

void PlayItem::setUuid(QUuid uuid)
{
    uuid_ = uuid;

    auto playWidget = reinterpret_cast<QDrawnPlaylist *>(listWidget());
    if (playWidget == NULL)
        return;
    Playlist *playlist = PlaylistCollection::getSingleton()->
                             playlistOf(playWidget->uuid());
    if (playlist == NULL)
        return;
    Item *item = playlist->itemOf(uuid);
    if (item == NULL)
        return;

    setText(uuid.toString());
}


QDrawnPlaylist::QDrawnPlaylist(QWidget *parent) : QListWidget(parent)
{
    setItemDelegate(new PlayPainter(this));
    connect(model(), SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
            this, SLOT(model_rowsMoved(QModelIndex,int,int,QModelIndex,int)));
    connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(self_itemDoubleClicked(QListWidgetItem*)));
}

QUuid QDrawnPlaylist::uuid() const
{
    return uuid_;
}

void QDrawnPlaylist::setUuid(const QUuid &uuid)
{
    clear();

    uuid_ = uuid;
    Playlist *playlist = PlaylistCollection::getSingleton()->playlistOf(uuid);
    if (playlist == NULL)
        return;

    auto itemAdder = [&](Item *item) {
        addItem(item->uuid());
    };
    playlist->iterateItems(itemAdder);
}

void QDrawnPlaylist::addItem(QUuid uuid)
{
    PlayItem *playItem = new PlayItem(this);
    playItem->setUuid(uuid);
    itemsByUuid.insert(uuid, playItem);
    QListWidget::addItem(playItem);
}

void QDrawnPlaylist::removeItem(QUuid uuid)
{
    Playlist *playlist = PlaylistCollection::getSingleton()->playlistOf(uuid);
    if (playlist)
        playlist->removeItem(uuid);
    auto matchingRows = findItems(uuid.toString(), Qt::MatchExactly);
    if (matchingRows.length() > 0)
        takeItem(row(matchingRows[0]));
}

QUuid QDrawnPlaylist::nowPlayingItem()
{
    return nowPlayingItem_;
}

void QDrawnPlaylist::setNowPlayingItem(QUuid uuid)
{
    bool refreshNeeded = itemsByUuid.contains(nowPlayingItem()) ||
            itemsByUuid.contains(uuid);
    nowPlayingItem_ = uuid;
    if (refreshNeeded)
        viewport()->repaint();
}

void QDrawnPlaylist::model_rowsMoved(const QModelIndex &parent,
                                     int start, int end,
                                     const QModelIndex &destination, int row)
{
    Q_UNUSED(parent);
    Q_UNUSED(destination);
    Playlist *p = PlaylistCollection::getSingleton()->playlistOf(uuid());
    if (p == NULL)
        return;
    p->moveItems(start, row, 1 + end - start);
}

void QDrawnPlaylist::self_itemDoubleClicked(QListWidgetItem *item)
{
    itemDesired(uuid(), reinterpret_cast<PlayItem*>(item)->uuid());
}
