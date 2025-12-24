#include <QApplication>
#include <QPainter>
#include "drawncollection.h"

CollectionPainter::CollectionPainter(QObject *parent) : QAbstractItemDelegate(parent)
{

}

void CollectionPainter::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    DrawnCollection *collectionWidget = qobject_cast<DrawnCollection*>(parent());
    auto collection = collectionWidget->collection();
    auto playlist = collection->getPlaylist(QUuid(index.data(Qt::DisplayRole).toString()));


    QStyleOptionViewItem o2 = option;
    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &o2,
                                       painter);
    QRect rc = option.rect.adjusted(3,0,-3,0);

    QString text = playlist->title();
    QFont f = collectionWidget->font();
    painter->setFont(f);
    painter->setPen(collectionWidget->palette().text().color());
    painter->drawText(rc, Qt::AlignLeft|Qt::AlignVCenter, text);
    painter->setFont(collectionWidget->font());
}

QSize CollectionPainter::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QApplication::style()->sizeFromContents(QStyle::CT_ItemViewItem,
                                                   &option,
                                                   option.rect.size());
}



CollectionItem::CollectionItem(QUuid playlistUuid, QListWidget *parent)
    : QListWidgetItem(playlistUuid.toString(), parent)
{
    playlistUuid_ = playlistUuid;
}

QUuid CollectionItem::uuid()
{
    return playlistUuid_;
}



DrawnCollection::DrawnCollection(QSharedPointer<PlaylistCollection> collection, QWidget *parent) : QListWidget(parent)
{
     collection_ = collection;
     setItemDelegate(new CollectionPainter(this));

     connect(this, &DrawnCollection::currentRowChanged,
             this, &DrawnCollection::self_currentRowChanged);
}

QSharedPointer<PlaylistCollection> DrawnCollection::collection()
{
    return collection_;
}

void DrawnCollection::addPlaylist(QUuid playlistUuid)
{
    CollectionItem *playItem = new CollectionItem(playlistUuid, this);
    addItem(playItem);
}

QUuid DrawnCollection::currentPlaylistUuid()
{
    auto ci = static_cast<CollectionItem*>(currentItem());
    if (!ci)
        ci = static_cast<CollectionItem*>(item(0));
    return ci ? ci->uuid() : QUuid();
}


void DrawnCollection::repopulatePlaylists()
{
    clear();
    auto itemAdder = [&](QSharedPointer<Playlist> playlist) {
        addPlaylist(playlist->uuid());
    };
    collection_->iteratePlaylists(itemAdder);
}

void DrawnCollection::self_currentRowChanged(int currentRow)
{
    QUuid playlistUuid;
    if (currentRow != -1)
        playlistUuid = static_cast<CollectionItem*>(this->item(currentRow))->uuid();
    emit playlistSelected(playlistUuid);
}

