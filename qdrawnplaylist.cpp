#include <QApplication>
#include <QThread>
#include <QPainter>
#include <QFontMetrics>
#include <QMenu>
#include <QKeyEvent>
#include "qdrawnplaylist.h"
#include "playlist.h"
#include "helpers.h"

PlayPainter::PlayPainter(QObject *parent) : QAbstractItemDelegate(parent) {}

void PlayPainter::paint(QPainter *painter, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const
{
    auto playWidget = qobject_cast<QDrawnPlaylist*>(parent());
    auto p = PlaylistCollection::getSingleton()->
            playlistOf(playWidget->uuid());
    if (p == NULL)
        return;
    QSharedPointer<Item> i = p->itemOf(QUuid(index.data(Qt::DisplayRole).toString()));
    if (i == NULL)
        return;

    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &option,
                                       painter);
    QRect rc = option.rect.adjusted(3,0,-3,0);

    if (i->queuePosition() > 0) {
        QString queueIndex(QString::number(i->queuePosition()));
        int queueIndexWidth = painter->fontMetrics().width(queueIndex);
        QRect rc2(rc);
        rc2.setLeft(rc.right() - queueIndexWidth);
        painter->drawText(rc2, Qt::AlignRight|Qt::AlignVCenter, queueIndex);
        rc.adjust(0, 0, -(3 + queueIndexWidth), 0);
    }

    DisplayParser *dp = playWidget->displayParser();
    QString text = i->toDisplayString();
    if (dp) {
        // TODO: detect what type of file is being played
        text = dp->parseMetadata(i->metadata(), text, Helpers::VideoFile);
    }

    QFont f = playWidget->font();
    f.setBold(i->uuid() == playWidget->nowPlayingItem());
    painter->setFont(f);
    painter->setPen(playWidget->palette().text().color());
    painter->drawText(rc, Qt::AlignLeft|Qt::AlignVCenter,
                      text);
    painter->setFont(playWidget->font());
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
    QSharedPointer<Playlist> playlist = PlaylistCollection::getSingleton()->
            playlistOf(playWidget->uuid());
    if (playlist == NULL)
        return;
    QSharedPointer<Item> item = playlist->itemOf(uuid);
    if (item == NULL)
        return;

    setText(uuid.toString());
}


QDrawnPlaylist::QDrawnPlaylist(QWidget *parent) : QListWidget(parent),
    displayParser_(NULL), worker(NULL), searcher(NULL)
{
    worker = new QThread();
    worker->start();
    searcher = new PlaylistSearcher();
    searcher->moveToThread(worker);

    setItemDelegate(new PlayPainter(this));
    connect(model(), SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
            this, SLOT(model_rowsMoved(QModelIndex,int,int,QModelIndex,int)));
    connect(this, &QDrawnPlaylist::searcher_filterPlaylist,
            searcher, &PlaylistSearcher::filterPlaylist,
            Qt::QueuedConnection);
    connect(searcher, &PlaylistSearcher::playlistFiltered,
            this, &QDrawnPlaylist::repopulateItems,
            Qt::QueuedConnection);
    connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(self_itemDoubleClicked(QListWidgetItem*)));
    connect(this, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(self_customContextMenuRequested(QPoint)));
    this->setContextMenuPolicy(Qt::CustomContextMenu);
}

QDrawnPlaylist::~QDrawnPlaylist()
{
    worker->deleteLater();
}

QUuid QDrawnPlaylist::uuid() const
{
    return uuid_;
}

QUuid QDrawnPlaylist::currentItemUuid() const
{
    PlayItem *item = reinterpret_cast<PlayItem*>(currentItem());
    if (!item)
        return QUuid();
    return item->uuid();
}

void QDrawnPlaylist::setCurrentItem(QUuid itemUuid)
{
    QList<QListWidgetItem *> items = findItems(itemUuid.toString(), Qt::MatchExactly);
    if (items.isEmpty())
        setCurrentRow(-1);
    else
        QListWidget::setCurrentItem(items.first());
}

void QDrawnPlaylist::setUuid(const QUuid &uuid)
{
    uuid_ = uuid;
    repopulateItems();
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
    QSharedPointer<Playlist> playlist = PlaylistCollection::getSingleton()->playlistOf(uuid_);
    if (playlist)
        playlist->removeItem(uuid);
    auto matchingRows = findItems(uuid.toString(), Qt::MatchExactly);
    if (matchingRows.length() > 0)
        takeItem(row(matchingRows[0]));
}

void QDrawnPlaylist::removeAll()
{
    QSharedPointer<Playlist> p = PlaylistCollection::getSingleton()->playlistOf(uuid());
    if (!p)
        return;
    p->clear();
    clear();
}

QPair<QUuid,QUuid> QDrawnPlaylist::importUrl(QUrl url)
{
    QPair<QUuid,QUuid> info;
    QSharedPointer<Playlist> playlist = PlaylistCollection::getSingleton()->playlistOf(uuid_);
    if (!playlist)  return info;
    auto item = playlist->addItem(url);
    info.first = uuid_;
    info.second = item->uuid();
    if (currentFilterText.isEmpty() ||
            PlaylistSearcher::itemMatchesFilter(item, currentFilterList))
        addItem(item->uuid());
    return info;
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

QVariantMap QDrawnPlaylist::toVMap() const
{
    QSharedPointer<Playlist> playlist = PlaylistCollection::getSingleton()->playlistOf(uuid_);
    if (!playlist)
        return QVariantMap();
    QVariantMap qvm;
    qvm.insert("nowplaying", nowPlayingItem_);
    qvm.insert("contents", playlist->toVMap());
    return qvm;
}

void QDrawnPlaylist::fromVMap(const QVariantMap &qvm)
{
    QVariantMap contents = qvm.value("contents").toMap();
    QSharedPointer<Playlist> p(new Playlist);
    p->fromVMap(contents);
    PlaylistCollection::getSingleton()->addPlaylist(p);
    setUuid(p->uuid());
    nowPlayingItem_ = qvm.value("nowplaying").toUuid();
    setCurrentItem(nowPlayingItem_);
}

void QDrawnPlaylist::setDisplayParser(DisplayParser *parser)
{
    displayParser_ = parser;
}

DisplayParser *QDrawnPlaylist::displayParser()
{
    return displayParser_;
}

void QDrawnPlaylist::setFilter(QString needles)
{
    if (currentFilterText == needles)
        return;

    currentFilterText = needles;
    currentFilterList = PlaylistSearcher::textToNeedles(needles);
    searcher->bump();
    emit searcher_filterPlaylist(uuid_, needles);
}

bool QDrawnPlaylist::event(QEvent *e)
{
    if (!hasFocus())
        goto end;
    if (e->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(e);
        if (ke->key() == Qt::Key_PageUp || ke->key() == Qt::Key_PageDown) {
            e->accept();
            return true;
        }
    }
    end:
    return QListWidget::event(e);
}

void QDrawnPlaylist::repopulateItems()
{
    clear();

    auto playlist = PlaylistCollection::getSingleton()->playlistOf(uuid_);
    if (playlist == NULL)
        return;

    auto itemAdder = [&](QSharedPointer<Item> item) {
        if (!item->hidden())
            addItem(item->uuid());
    };
    playlist->iterateItems(itemAdder);

}

void QDrawnPlaylist::model_rowsMoved(const QModelIndex &parent,
                                     int start, int end,
                                     const QModelIndex &destination, int row)
{
    // FIXME: Moving by indicies is now broken.
    Q_UNUSED(parent);
    Q_UNUSED(destination);
    QSharedPointer<Playlist> p = PlaylistCollection::getSingleton()->playlistOf(uuid());
    if (p.isNull())
        return;
    p->moveItems(start, row, 1 + end - start);
}

void QDrawnPlaylist::self_itemDoubleClicked(QListWidgetItem *item)
{
    itemDesired(uuid(), reinterpret_cast<PlayItem*>(item)->uuid());
}

void QDrawnPlaylist::self_customContextMenuRequested(const QPoint &p)
{
    QMenu *m = new QMenu(this);
    QAction *a = new QAction(m);
    a->setText(tr("Remove"));
    connect(a, &QAction::triggered, [=]() {
        int index = currentRow();
        if (index < 0)
            return;
        QSharedPointer<Playlist> p = PlaylistCollection::getSingleton()->playlistOf(uuid());
        if (!p)
            return;
        QSharedPointer<Item> item = p->itemAt(index);
        if (!item)
            return;
        removeItem(item->uuid());
    });
    m->addAction(a);
    a = new QAction(m);
    a->setText(tr("Remove All"));
    connect(a, &QAction::triggered, this, &QDrawnPlaylist::removeAll);
    m->addAction(a);
    connect(m, &QMenu::aboutToHide, [=]() {
        m->deleteLater();
    });
    m->exec(mapToGlobal(p));
}
