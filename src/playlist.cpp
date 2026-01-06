#include <QFileInfo>
#include "playlist.h"
#include <random>



static constexpr char logModule[] =  "playlist";
static constexpr char keyContents[] = "contents";
static constexpr char keyCreated[] = "created";
static constexpr char keyItems[] = "items";
static constexpr char keyMetadata[] = "metadata";
static constexpr char keyNowPlaying[] = "nowplaying";
static constexpr char keyRepeat[] = "repeat";
static constexpr char keyShuffle[] = "shuffle";
static constexpr char keyTitle[] = "title";
static constexpr char keyUrl[] = "url";
static constexpr char keyUuid[] = "uuid";
static constexpr char keyOriginalPosition[] = "originalposition";



Item::Item(QUrl url)
{
    static int globalCounter = 0;
    setUrl(url);
    setUuid(QUuid::createUuid());
    setOriginalPosition(globalCounter++);   // Preserve order on first restore
    setQueuePosition(0);
    setExtraPlayTimes(0);
    setHidden(false);
}

QUuid Item::uuid() const
{
    return itemUuid_;
}

void Item::setUuid(const QUuid &itemUuid)
{
    itemUuid_ = itemUuid;
}

QUuid Item::playlistUuid() const
{
    return playlistUuid_;
}

void Item::setPlaylistUuid(const QUuid &playlistUuid)
{
    playlistUuid_ = playlistUuid;
}

QUrl Item::url() const
{
    return url_;
}

void Item::setUrl(const QUrl &url)
{
    url_ = url;
}

const QVariantMap &Item::metadata() const
{
    return metadata_;
}

void Item::setMetadata(const QVariantMap &qvm)
{
    metadata_ = qvm;
}

int Item::originalPosition() const
{
    return originalPosition_;
}

void Item::setOriginalPosition(int i)
{
    originalPosition_ = i;
}

int Item::queuePosition() const
{
    return queuePosition_;
}

void Item::setQueuePosition(int num)
{
    queuePosition_ = num;
}

void Item::decQueuePosition()
{
    if (queuePosition_ > 0)
        queuePosition_--;
}

int Item::extraPlayTimes() const
{
    return extraPlayTimes_;
}

void Item::setExtraPlayTimes(int amount)
{
    extraPlayTimes_ = std::max(amount, 0);
}

void Item::deltaExtraPlayTimes(int delta)
{
    extraPlayTimes_ = std::max(0, extraPlayTimes_ + delta);
}

int Item::incExtraPlayTimes()
{
    return ++extraPlayTimes_ > 0 ? extraPlayTimes_ : 0;
}

int Item::decExtraPlayTimes()
{
    return extraPlayTimes_ > 0 ? --extraPlayTimes_ : 0;
}

void Item::setHidden(bool yes)
{
    hidden_ = yes;
}

bool Item::hidden()
{
    return hidden_;
}

QString Item::toDisplayString() const
{
    if (url().isLocalFile())
        return QFileInfo(url().toLocalFile()).completeBaseName();
    return url().toDisplayString(QUrl::PrettyDecoded);
}

QString Item::toString() const
{
    return url_.isLocalFile() ? url_.toLocalFile() : url_.url();
}

void Item::fromString(QString input)
{
    setUuid(QUuid::createUuid());
    setUrl(QUrl::fromUserInput(input));
}

QVariantMap Item::toVMap(bool savePosition) const
{
    QVariantMap v;
    v.insert(keyUrl, url());
    v.insert(keyUuid, uuid());
    v.insert(keyMetadata, metadata());
    if (savePosition)
        v.insert(keyOriginalPosition, originalPosition());
    return v;
}

void Item::fromVMap(const QVariantMap &qvm)
{
    url_ = qvm.contains(keyUrl) ? qvm.value(keyUrl).toUrl() : QUrl();
    itemUuid_ = qvm.contains(keyUuid) ? qvm.value(keyUuid).toUuid() : QUuid::createUuid();
    metadata_ = qvm.contains(keyMetadata) ? qvm.value(keyMetadata).toMap() : QVariantMap();
    if (qvm.contains(keyOriginalPosition))
        originalPosition_ = qvm.value(keyOriginalPosition).toInt();
}

QSharedPointer<ItemCollection> ItemCollection::collection;

ItemCollection::ItemCollection() : QObject(nullptr)
{

}

ItemCollection::~ItemCollection()
{

}

QSharedPointer<ItemCollection> ItemCollection::getSingleton()
{
    if (collection.isNull())
        collection.reset(new ItemCollection());
    return collection;
}

QSharedPointer<Item> ItemCollection::addItem(const QUrl url)
{
    auto item = QSharedPointer<Item>::create(url);
    items.insert(item->uuid(), item);
    return item;
}

QSharedPointer<Item> ItemCollection::addItem(const QUuid &itemUuid, const QUrl &url)
{
    QSharedPointer<Item> item(new Item(url));
    item->setUuid(itemUuid);
    items.insert(item->uuid(), item);
    return item;
}

QSharedPointer<Item> ItemCollection::getItem(const QUuid &itemUuid)
{
    return items.value(itemUuid, QSharedPointer<Item>());
}

void ItemCollection::removeItem(const QUuid &itemUuid)
{
    items.remove(itemUuid);
}

void ItemCollection::storeItem(const QSharedPointer<Item> &item)
{
    items.insert(item->uuid(), item);
}



Playlist::Playlist(const QString &title)
{
    setUuid(QUuid::createUuid());
    setTitle(title);
    setCreated(QDateTime::currentDateTimeUtc());
}

Playlist::~Playlist()
{
    QWriteLocker locker(&listLock);
    items.clear();
}

QSharedPointer<Item> Playlist::addItem(const QUrl &url)
{
    QWriteLocker locker(&listLock);
    QSharedPointer<Item> i(ItemCollection::getSingleton()->addItem(url));
    i->setPlaylistUuid(playlistUuid_);
    items.append(i);
    itemsByUuid.insert(i->uuid(), i);
    return i;
}

QSharedPointer<Item> Playlist::addItem(const QUuid &itemUuid, const QUrl &url)
{
    QWriteLocker locker(&listLock);
    QSharedPointer<Item> i(ItemCollection::getSingleton()->addItem(itemUuid, url));
    i->setPlaylistUuid(playlistUuid_);
    i->setUrl(url);
    i->setUuid(itemUuid);
    items.append(i);
    itemsByUuid.insert(itemUuid, i);
    return i;
}

QSharedPointer<Item> Playlist::addItemClone(const QSharedPointer<Item> &item)
{
    QSharedPointer<Item> i = addItem(item->url());
    i->setPlaylistUuid(playlistUuid_);
    i->setMetadata(item->metadata());
    return i;
}

void Playlist::addItemRaw(const QSharedPointer<Item> &item)
{
    QWriteLocker locker(&listLock);
    items.append(item);
    itemsByUuid.insert(item->uuid(), item);
}

QSharedPointer<Item> Playlist::itemAt(int index)
{
    QReadLocker locker(&listLock);
    if (index < 0 || index >= items.count())
        return QSharedPointer<Item>();
    return items.value(index);
}

QSharedPointer<Item> Playlist::getItem(const QUuid &itemUuid)
{
    QReadLocker locker(&listLock);
    return itemsByUuid.value(itemUuid, QSharedPointer<Item>());
}

QSharedPointer<Item> Playlist::itemAfter(const QUuid &itemUuid)
{
    QReadLocker locker(&listLock);
    if (!itemsByUuid.contains(itemUuid))
        return QSharedPointer<Item>();
    int index = items.indexOf(itemsByUuid[itemUuid]);
    if (index < 0 || index + 1 >= items.length())
        return QSharedPointer<Item>();
    return items[index + 1];
}

QSharedPointer<Item> Playlist::itemBefore(const QUuid &itemUuid)
{
    QReadLocker locker(&listLock);
    if (!itemsByUuid.contains(itemUuid))
        return QSharedPointer<Item>();
    int index = items.indexOf(itemsByUuid[itemUuid]);
    if (index <= 0)
        return QSharedPointer<Item>();
    return items[index - 1];
}

QSharedPointer<Item> Playlist::itemFirst()
{
    QReadLocker locker(&listLock);
    if (items.isEmpty())
        return QSharedPointer<Item>();
    return items.first();
}

QSharedPointer<Item> Playlist::itemLast()
{
    QReadLocker locker(&listLock);
    if (items.isEmpty())
        return QSharedPointer<Item>();
    return items.last();
}

int Playlist::count()
{
    QReadLocker lock(&listLock);
    return items.count();
}

bool Playlist::isEmpty()
{
    QReadLocker lock(&listLock);
    return items.isEmpty();
}

bool Playlist::contains(const QUuid &itemUuid)
{
    QReadLocker lock(&listLock);
    return itemsByUuid.contains(itemUuid);
}

void Playlist::iterateItems(const std::function<void(QSharedPointer<Item>)> &callback)
{
    QReadLocker locker(&listLock);
    for (auto const &item : items)
        callback(item);
}

void Playlist::addItems(const QUuid &where,
                        const QList<QSharedPointer<Item>> &itemsToAdd)
{
    QWriteLocker locker(&listLock);

    int indexWhere = items.indexOf(itemsByUuid[where]);
    if (indexWhere < 0)
        indexWhere = items.size();
    for (int i = 0; i < itemsToAdd.count(); ++i) {
        QSharedPointer<Item> item = itemsToAdd.at(i);
        item->setPlaylistUuid(playlistUuid_);
        items.insert(indexWhere + i, item);
        itemsByUuid.insert(item->uuid(), item);
    }
}

void Playlist::removeItem(const QUuid &itemUuid)
{
    QWriteLocker locker(&listLock);
    PlaylistCollection::queuePlaylist()->removeItem(itemUuid);
    items.removeAll(itemsByUuid.take(itemUuid));
    ItemCollection::getSingleton()->removeItem(itemUuid);
}

void Playlist::takeItemsRaw(const QList<QSharedPointer<Item>> &itemsToRemove)
{
    // "takeItemsRaw", because we don't check if it's in a queue or whatever,
    // it's just taken raw, potentially damaging everything.  Only use if you
    // may know what you're doing.
    for (const QSharedPointer<Item> &item: itemsToRemove) {
        itemsByUuid.remove(item->uuid());
        items.removeAll(item);
    }
}

QList<QUuid> Playlist::replaceItem(const QUuid &where, const QList<QUrl> &urls)
{
    QWriteLocker lock(&listLock);
    if (!itemsByUuid.contains(where))
        return QList<QUuid>();

    itemsByUuid[where]->setUrl(urls[0]);

    QList<QUuid> addedItems;
    // essentially insertAfter(where, urls[1..end]);
    int insertIndex = items.indexOf(itemsByUuid[where]);
    for (int urlIndex = 1; urlIndex < urls.count(); urlIndex++) {
        QSharedPointer<Item> i(new Item(urls[urlIndex]));
        i->setPlaylistUuid(playlistUuid_);
        items.insert(insertIndex + urlIndex, i);
        itemsByUuid.insert(i->uuid(), i);
        addedItems.append(i->uuid());
    }
    return addedItems;
}

void Playlist::clear()
{
    QWriteLocker locker(&listLock);
    PlaylistCollection::queuePlaylist()->removeItems(itemsByUuid.keys());
    items.clear();
    itemsByUuid.clear();
}

QDateTime Playlist::created()
{
    return created_;
}

void Playlist::setCreated(const QDateTime &dt)
{
    created_ = dt;
}

QString Playlist::title()
{
    QReadLocker locker(&listLock);
    return title_;
}

void Playlist::setTitle(const QString &title)
{
    QWriteLocker locker(&listLock);
    title_ = title;
}

bool Playlist::repeat()
{
    return repeat_;
}

void Playlist::setRepeat(bool repeat)
{
    repeat_ = repeat;
}

bool Playlist::shuffle()
{
    return shuffle_;
}

void Playlist::setShuffle(bool shuffling)
{
    shuffle_ = shuffling;
    if (shuffling)
        shuffleItems();
    else
        unshuffleItems();
}

void Playlist::shuffleItems()
{
    QWriteLocker locker(&listLock);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(items.begin(), items.end(), gen);
    locker.unlock();
    this->setNowPlaying(items.first()->uuid());
}

void Playlist::unshuffleItems()
{
    QWriteLocker locker(&listLock);
    std::sort(items.begin(), items.end(),
        [](const QSharedPointer<Item> &a, const QSharedPointer<Item> &b) {
            return a->originalPosition() < b->originalPosition();
    });
}

QUuid Playlist::uuid()
{
    QReadLocker locker(&listLock);
    return playlistUuid_;
}

void Playlist::setUuid(const QUuid &playlistUuid)
{
    QWriteLocker locker(&listLock);
    playlistUuid_ = playlistUuid;
}

QUuid Playlist::nowPlaying()
{
    return nowPlaying_;
}

void Playlist::setNowPlaying(const QUuid &itemUuid)
{
    nowPlaying_ = itemUuid;
}

QStringList Playlist::toStringList()
{
    QReadLocker locker(&listLock);
    QStringList sl;
    for (auto &i : items)
        sl << i->toString();
    return sl;
}

void Playlist::fromStringList(QStringList sl)
{
    QWriteLocker locker(&listLock);
    items.clear();
    itemsByUuid.clear();
    for (QString &s : sl) {
        QSharedPointer<Item> item(new Item());
        item->setPlaylistUuid(playlistUuid_);
        item->fromString(s);
        items.append(item);
        itemsByUuid.insert(item->uuid(), item);
    }
}

QVariantMap Playlist::toVMap()
{
    QWriteLocker locker(&listLock);
    QVariantMap qvm;
    qvm.insert(keyCreated, created_);
    qvm.insert(keyTitle, title_);
    qvm.insert(keyRepeat, repeat_);
    qvm.insert(keyShuffle, shuffle_);
    qvm.insert(keyUuid, playlistUuid_);
    qvm.insert(keyNowPlaying, nowPlaying_);

    QVariantList qvl;
    for (auto &i : items) {
        qvl.append(i->toVMap(shuffle_));
    }
    qvm.insert(keyItems, qvl);
    return qvm;
}

void Playlist::fromVMap(const QVariantMap &qvm)
{
    QReadLocker locker(&listLock);
    if (qvm.contains(keyContents)) {
        // old format, call me back with the subnode
        nowPlaying_ = qvm.value(keyNowPlaying, nowPlaying_).toUuid();
        fromVMap(qvm[keyContents].toMap());
        return;
    }

    created_ = qvm.contains(keyCreated) ? qvm[keyCreated].toDateTime() : created_;
    title_ = qvm.contains(keyTitle) ? qvm[keyTitle].toString() : QString();
    repeat_ = qvm.contains(keyRepeat) ? qvm[keyRepeat].toBool() : false;
    shuffle_ = qvm.contains(keyShuffle) ? qvm[keyShuffle].toBool() : false;
    playlistUuid_ = qvm.contains(keyUuid) ? qvm[keyUuid].toUuid() : QUuid::createUuid();
    nowPlaying_ = qvm.contains(keyNowPlaying) ? qvm[keyNowPlaying].toUuid() : nowPlaying_;
    if (qvm.contains(keyItems)) {
        auto items = qvm[keyItems].toList();
        for (const QVariant &v : std::as_const(items)) {
            QSharedPointer<Item> i(new Item());
            i->setPlaylistUuid(playlistUuid_);
            i->fromVMap(v.toMap());
            this->items.append(i);
            this->itemsByUuid.insert(i->uuid(), i);
            ItemCollection::getSingleton()->storeItem(i);
        }
        // Reshuffle on first start with non shuffled list in shuffle mode
        if (shuffle() && !items.isEmpty() && !items[0].toMap().contains(keyOriginalPosition)) {
            locker.unlock();
            shuffleItems();
        }
    }
}



QueuePlaylist::QueuePlaylist(const QString &title)
    : Playlist(title)
{

}

PlaylistItem QueuePlaylist::first()
{
    QReadLocker lock(&listLock);
    if (items.isEmpty())
        return { QUuid(), QUuid() };
    return { items.constFirst()->playlistUuid(), items.constFirst()->uuid() };
}

PlaylistItem QueuePlaylist::takeFirst()
{
    QWriteLocker lock(&listLock);
    if (items.isEmpty())
        return { QUuid(), QUuid() };
    QSharedPointer<Item> item = items.takeFirst();
    itemsByUuid.remove(item->uuid());
    item->setQueuePosition(0);
    int i = 1;
    for (auto &item : items)
        item->setQueuePosition(i++);
    return { item->playlistUuid(), item->uuid() };

}

int QueuePlaylist::toggle(const QUuid &playlistUuid, const QUuid &itemUuid, bool always)
{
    QWriteLocker lock(&listLock);
    return toggle_(playlistUuid, itemUuid, always);
}

void QueuePlaylist::toggle(const QUuid &playlistUuid, const QList<QUuid> &uuids, QList<QUuid> &added, QList<int> &removed)
{
    QWriteLocker lock(&listLock);
    int numberPresent = contains_(uuids);
    if (numberPresent == uuids.count()) {
        removed.append(removeItems_(uuids));
        return;
    }
    for (const QUuid &item : uuids)
        if (toggle_(playlistUuid, item, true) > 0)
            added.append(item);
}

void QueuePlaylist::toggleFromPlaylist(const QUuid &playlistUuid, QList<QUuid> &added, QList<int> &removedIndices)
{
    QWriteLocker lock(&listLock);
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(playlistUuid);
    QReadLocker plLock(&pl->listLock);
    if (contains_(pl->itemsByUuid.keys()) == pl->itemsByUuid.count()) {
        // remove all items from playlist
        removedIndices.append(removeItems_(pl->itemsByUuid.keys()));
    } else {
        for (QSharedPointer<Item> &item : pl->items) {
            if (!itemsByUuid.contains(item->uuid())) {
                items.append(item);
                itemsByUuid.insert(item->uuid(), item);
                item->setQueuePosition(items.count());
                added.append(item->uuid());
            }
        }
    }
}

void QueuePlaylist::appendItems(const QUuid &playlistUuid, const QList<QUuid> &itemsToAdd)
{
    QWriteLocker lock(&listLock);
    for (QUuid item : itemsToAdd)
        toggle_(playlistUuid, item, true);
}

void QueuePlaylist::addItems(const QUuid &where, const QList<QSharedPointer<Item> > &itemsToAdd)
{
    QWriteLocker lock(&listLock);
    int index = items.indexOf(itemsByUuid[where]);
    if (index < 0)
        index = 0;

    int count = itemsToAdd.count();
    for (int i = 0; i < count; i++) {
        QSharedPointer<Item> item = itemsToAdd[i];
        items.insert(index + i, item);
        itemsByUuid.insert(item->uuid(), item);
    }
    count = items.count();
    for (int i = index; i < count; i++)
        items[i]->setQueuePosition(i+1);
}

void QueuePlaylist::removeItem(const QUuid &itemUuid)
{
    QWriteLocker lock(&listLock);
    removeItem_(itemUuid);
}

void QueuePlaylist::removeItems(const QList<QUuid> &itemsToRemove)
{
    QWriteLocker lock(&listLock);
    removeItems_(itemsToRemove);
}

void QueuePlaylist::clear()
{
    QWriteLocker lock(&listLock);
    for (QSharedPointer<Item> &item : items)
        item->setQueuePosition(0);
    items.clear();
    itemsByUuid.clear();
}

int QueuePlaylist::contains(const QList<QUuid> &itemsToCheck)
{
    QReadLocker lock(&listLock);
    return contains_(itemsToCheck);
}

int QueuePlaylist::toggle_(const QUuid &playlistUuid, const QUuid &itemUuid, bool always)
{
    if (itemsByUuid.contains(itemUuid)) {
        if (!always) {
            removeItem_(itemUuid);
            return -1;
        }
        return 0;
    }
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(playlistUuid);
    if (!pl)
        return 0;
    QSharedPointer<Item> item = pl->getItem(itemUuid);
    if (!item)
        return 0;
    items.append(item);
    itemsByUuid.insert(itemUuid, item);
    item->setQueuePosition(items.count());
    return 1;
}

int QueuePlaylist::contains_(const QList<QUuid> &itemsToCheck) const
{
    int count = 0;
    for (QUuid item : itemsToCheck)
        if (itemsByUuid.contains(item))
            count++;
    return count;
}

void QueuePlaylist::removeItem_(const QUuid &itemUuid)
{
    if (!itemsByUuid.contains(itemUuid))
        return;
    QSharedPointer<Item> item = itemsByUuid[itemUuid];
    int index = items.indexOf(item);
    items.removeOne(item);
    itemsByUuid.remove(itemUuid);
    item->setQueuePosition(0);
    int count = items.count();
    for (int i = index; i < count; i++)
        items[i]->setQueuePosition(i+1);
}

QList<int> QueuePlaylist::removeItems_(const QList<QUuid> &itemsToRemove)
{
    QList<int> removedIndices;
    QSet<QUuid> removalSet(itemsToRemove.begin(), itemsToRemove.end());
    QMutableListIterator<QSharedPointer<Item>> i(items);
    int index = 0;
    while (i.hasNext()) {
        QSharedPointer<Item> item = i.next();
        if (removalSet.contains(item->uuid())) {
            itemsByUuid.remove(item->uuid());
            item->setQueuePosition(0);
            i.remove();
            removedIndices.append(index);
        }
        index++;
    }
    for (int i = 0; i < items.count(); i++)
        items[i]->setQueuePosition(i+1);
    return removedIndices;
}



QSharedPointer<PlaylistCollection> PlaylistCollection::collection;
QSharedPointer<PlaylistCollection> PlaylistCollection::backup;
QSharedPointer<QueuePlaylist> PlaylistCollection::queue;

PlaylistCollection::PlaylistCollection() = default;

PlaylistCollection::~PlaylistCollection()
{
    playlists.clear();
    playlistsByUuid.clear();
}

QSharedPointer<PlaylistCollection> PlaylistCollection::getSingleton()
{
    if (collection.isNull()) {
        collection.reset(new PlaylistCollection());
        collection->doNewPlaylist(tr("Quick Playlist"), QUuid());
    }
    return collection;
}

QSharedPointer<PlaylistCollection> PlaylistCollection::getBackup()
{
    if (backup.isNull())
        backup.reset(new PlaylistCollection());
    return backup;
}

QSharedPointer<QueuePlaylist> PlaylistCollection::queuePlaylist()
{
    if (queue.isNull())
        queue.reset(new QueuePlaylist("Queue"));
    return queue;
}

void PlaylistCollection::iteratePlaylists(const std::function<void (QSharedPointer<Playlist>)> &callback)
{
    for (const auto &p : std::as_const(playlists)) {
        callback(p);
    }
}

QSharedPointer<Playlist> PlaylistCollection::newPlaylist(const QString &title)
{
    return doNewPlaylist(title, QUuid::createUuid());
}

QSharedPointer<Playlist> PlaylistCollection::clonePlaylist(const QUuid &playlistUuid)
{
    if (!playlistsByUuid.contains(playlistUuid))
        return QSharedPointer<Playlist>();
    auto origin = playlistsByUuid[playlistUuid];
    auto remote = newPlaylist(origin->title());
    auto cloner = [remote,origin](QSharedPointer<Item> i) {
        auto clone = remote->addItemClone(i);
        if (origin->nowPlaying() == i->uuid()) {
            remote->setNowPlaying(clone->uuid());
        }
    };
    origin->iterateItems(cloner);
    return remote;
}

QSharedPointer<Playlist> PlaylistCollection::takePlaylist(const QUuid &playlistUuid)
{
    QSharedPointer<Playlist> playlist = getPlaylist(playlistUuid);
    removePlaylist(playlistUuid);
    return playlist;
}

void PlaylistCollection::removePlaylist(const QUuid &playlistUuid)
{
    if (!playlistsByUuid.contains(playlistUuid))
        return;
    QSharedPointer<Playlist> p = playlistsByUuid.value(playlistUuid);
    playlists.removeAll(p);
    playlistsByUuid.remove(playlistUuid);
}

void PlaylistCollection::removePlaylist(const QSharedPointer<Playlist> &p)
{
    if (p == nullptr)
        return;
    removePlaylist(p->uuid());
}

QSharedPointer<Playlist> PlaylistCollection::playlistAt(int col) const
{
    return (col < playlists.count()) ? playlists.at(col) : QSharedPointer<Playlist>();
}

QSharedPointer<Playlist> PlaylistCollection::getPlaylist(const QUuid &playlistUuid) const
{
    return playlistsByUuid.value(playlistUuid);
}

void PlaylistCollection::addPlaylist(const QSharedPointer<Playlist> &playlist)
{
    if (!playlist)
        return;
    if (playlistsByUuid.contains(playlist->uuid())) {
        QSharedPointer<Playlist> old = playlistsByUuid[playlist->uuid()];
        playlistsByUuid.remove(playlist->uuid());
        playlists.removeOne(old);
    }
    playlists.append(playlist);
    playlistsByUuid.insert(playlist->uuid(), playlist);
}

void PlaylistCollection::fromVList(const QVariantList &data)
{
    for (const auto &d : data) {
        QSharedPointer<Playlist> p(new Playlist);
        p->fromVMap(d.toMap());
        addPlaylist(p);
    }
}

QVariantList PlaylistCollection::toVList()
{
    QVariantList l;
    for (const auto &p : std::as_const(playlists)) {
        l.append(p->toVMap());
    }
    return l;
}

QSharedPointer<Playlist> PlaylistCollection::doNewPlaylist(const QString &title,
                                                           const QUuid &playlistUuid)
{
    QSharedPointer<Playlist> p(new Playlist(title));
    p->setUuid(playlistUuid);
    playlists.append(p);
    playlistsByUuid.insert(p->uuid(), p);
    return p;
}

void PlaylistSearcher::bump()
{
    QWriteLocker locker(&bumpLock);
    ++bumps_;
}

void PlaylistSearcher::unbump()
{
    QWriteLocker locker(&bumpLock);
    --bumps_;
}

int PlaylistSearcher::bumps()
{
    QReadLocker locker(&bumpLock);
    return bumps_;
}

bool PlaylistSearcher::itemMatchesFilter(const QSharedPointer<Item> &item,
                                         const QStringList &needles)
{
    QSet<QString> found;
    findNeedles(item->toDisplayString(), needles, found);
    if (found.count() == needles.count())
        return true;

    // OPTIMIZE: don't run the metadata through toLower every search
    for (const QVariant &v : item->metadata())
        findNeedles(v.toString(), needles, found);
    return found.count() == needles.count();
}

void PlaylistSearcher::filterPlaylist(QSharedPointer<Playlist> list, QString text)
{
    // Limit response - only reply if last in event queue
    bool bumpLimited = bumps() > 1;
    unbump();
    if (bumpLimited)
        return;

    QStringList needles = textToNeedles(text);
    if (needles.isEmpty()) {
        clearPlaylistFilter(list);
        return;
    }

    if (list.isNull())
        return;

    auto marker = [&needles](QSharedPointer<Item> item) {
        item->setHidden(!itemMatchesFilter(item, needles));
    };
    list->iterateItems(marker);
    emit playlistFiltered(list->uuid());
}

void PlaylistSearcher::clearPlaylistFilter(QSharedPointer<Playlist> const &list)
{
    if (list.isNull())
        return;

    auto clearer = [](QSharedPointer<Item> item) {
        item->setHidden(false);
    };
    list->iterateItems(clearer);
    emit playlistFiltered(list->uuid());
}

QStringList PlaylistSearcher::textToNeedles(QString text)
{
    return text.toLower().split(QString(" "), Qt::SkipEmptyParts);
}

void PlaylistSearcher::findNeedles(const QString &text,
                                   const QStringList &needles,
                                   QSet<QString> &found)
{
    QString haystack = text.toLower();
    for (const QString &needle : needles) {
        if (haystack.contains(needle))
            found.insert(needle);
    }
}
