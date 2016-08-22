#include <QFileInfo>
#include <cmath>
#include "playlist.h"

Item::Item(QUrl url)
{
    setUrl(url);
    setUuid(QUuid::createUuid());
    setQueuePosition(0);
    setExtraPlayTimes(0);
    setHidden(false);
}

QUuid Item::uuid() const
{
    return uuid_;
}

void Item::setUuid(const QUuid &uuid)
{
    uuid_ = uuid;
}

QUuid Item::playlistUuid() const
{
    return playlistUuid_;
}

void Item::setPlaylistUuid(const QUuid &uuid)
{
    playlistUuid_ = uuid;
}

QUrl Item::url() const
{
    return url_;
}

void Item::setUrl(const QUrl &url)
{
    url_ = url;
}

QVariantMap Item::metadata() const
{
    return metadata_;
}

void Item::setMetadata(const QVariantMap &qvm)
{
    metadata_ = qvm;
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
    return url().toDisplayString(QUrl::FullyDecoded);
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

QVariantMap Item::toVMap() const
{
    QVariantMap v;
    v.insert("url", url());
    v.insert("uuid", uuid());
    v.insert("metadata", metadata());
    return v;
}

void Item::fromVMap(const QVariantMap &qvm)
{
    url_ = qvm.contains("url") ? qvm.value("url").toUrl() : QUrl();
    uuid_ = qvm.contains("uuid") ? qvm.value("uuid").toUuid() : QUuid::createUuid();
    metadata_ = qvm.contains("metadata") ? qvm.value("metadata").toMap() : QVariantMap();
}

Playlist::Playlist(const QString &title)
{
    setUuid(QUuid::createUuid());
    setTitle(title);
}

Playlist::~Playlist()
{
    QWriteLocker locker(&listLock);
    items.clear();
}

QSharedPointer<Item> Playlist::addItem(const QUrl &url)
{
    QWriteLocker locker(&listLock);
    QSharedPointer<Item> i(new Item(url));
    i->setPlaylistUuid(uuid_);
    items.append(i);
    itemsByUuid.insert(i->uuid(), i);
    return i;
}

QSharedPointer<Item> Playlist::addItem(const QUuid &uuid, const QUrl &url)
{
    QWriteLocker locker(&listLock);
    QSharedPointer<Item> i(new Item(url));
    i->setPlaylistUuid(uuid_);
    i->setUrl(url);
    i->setUuid(uuid);
    items.append(i);
    itemsByUuid.insert(uuid, i);
    return i;
}

QSharedPointer<Item> Playlist::addItemClone(const QSharedPointer<Item> &item)
{
    QSharedPointer<Item> i = addItem(item->url());
    i->setPlaylistUuid(uuid_);
    i->setMetadata(item->metadata());
    return i;
}

void Playlist::addItemRaw(const QSharedPointer<Item> &item)
{
    QWriteLocker locker(&listLock);
    items.append(item);
    itemsByUuid.insert(item->uuid(), item);
}

QSharedPointer<Item> Playlist::itemOf(const QUuid &uuid)
{
    QReadLocker locker(&listLock);
    return itemsByUuid.value(uuid, QSharedPointer<Item>());
}

QSharedPointer<Item> Playlist::itemAfter(const QUuid &uuid)
{
    QReadLocker locker(&listLock);
    if (!itemsByUuid.contains(uuid))
        return QSharedPointer<Item>();
    int index = items.indexOf(itemsByUuid[uuid]);
    if (index < 0 || index + 1 >= items.length())
        return QSharedPointer<Item>();
    return items[index + 1];
}

QSharedPointer<Item> Playlist::itemBefore(const QUuid &uuid)
{
    QReadLocker locker(&listLock);
    if (!itemsByUuid.contains(uuid))
        return QSharedPointer<Item>();
    int index = items.indexOf(itemsByUuid[uuid]);
    if (index <= 0)
        return QSharedPointer<Item>();
    return items[index - 1];
}

bool Playlist::isEmpty()
{
    QReadLocker lock(&listLock);
    return items.isEmpty();
}

bool Playlist::contains(const QUuid &uuid)
{
    QReadLocker lock(&listLock);
    return itemsByUuid.contains(uuid);
}

void Playlist::iterateItems(const std::function<void(QSharedPointer<Item>)> &callback)
{
    QReadLocker locker(&listLock);
    for (auto item : items)
        callback(item);
}

void Playlist::addItems(const QUuid &where,
                        const QList<QSharedPointer<Item>> &itemsToAdd)
{
    QWriteLocker locker(&listLock);
    if (!itemsByUuid.contains(where))
        return;

    int indexWhere = items.indexOf(itemsByUuid[where]);
    for (int i = 0; i < itemsToAdd.count(); ++i) {
        QSharedPointer<Item> item = itemsToAdd.at(i);
        item->setPlaylistUuid(uuid_);
        items.insert(indexWhere + i, item);
        itemsByUuid.insert(item->uuid(), item);
    }
}

void Playlist::removeItem(const QUuid &uuid)
{
    QWriteLocker locker(&listLock);
    PlaylistCollection::getSingleton()->queuePlaylist()->removeItem(uuid);
    items.removeAll(itemsByUuid.take(uuid));
}

void Playlist::takeItemsRaw(const QList<QSharedPointer<Item>> &itemsToRemove)
{
    // "takeItemsRaw", because we don't check if it's in a queue or whatever,
    // it's just taken raw, potentially damaging everything.  Only use if you
    // may know what you're doing.
    for (QSharedPointer<Item> item: itemsToRemove) {
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
        i->setPlaylistUuid(uuid_);
        items.insert(insertIndex + urlIndex, i);
        itemsByUuid.insert(i->uuid(), i);
        addedItems.append(i->uuid());
    }
    return addedItems;
}

void Playlist::clear()
{
    QWriteLocker locker(&listLock);
    PlaylistCollection::getSingleton()->queuePlaylist()->removeItems(itemsByUuid.keys());
    items.clear();
    itemsByUuid.clear();
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

QUuid Playlist::uuid()
{
    QReadLocker locker(&listLock);
    return uuid_;
}

void Playlist::setUuid(const QUuid &uuid)
{
    QWriteLocker locker(&listLock);
    uuid_ = uuid;
}

QStringList Playlist::toStringList()
{
    QReadLocker locker(&listLock);
    QStringList sl;
    for (auto i : items)
        sl << i->toString();
    return sl;
}

void Playlist::fromStringList(QStringList sl)
{
    QWriteLocker locker(&listLock);
    items.clear();
    itemsByUuid.clear();
    for (QString s : sl) {
        QSharedPointer<Item> item(new Item());
        item->setPlaylistUuid(uuid_);
        item->fromString(s);
        items.append(item);
        itemsByUuid.insert(item->uuid(), item);
    }
}

QVariantMap Playlist::toVMap()
{
    QWriteLocker locker(&listLock);
    QVariantMap qvm;
    qvm.insert("title", title_);
    qvm.insert("uuid", uuid_);

    QVariantList qvl;
    for (auto i : items) {
        qvl.append(i->toVMap());
    }
    qvm.insert("items", qvl);
    return qvm;
}

void Playlist::fromVMap(const QVariantMap &qvm)
{
    QReadLocker locker(&listLock);
    title_ = qvm.contains("title") ? qvm["title"].toString() : QString();
    uuid_ = qvm.contains("uuid") ? qvm["uuid"].toUuid() : QUuid::createUuid();
    if (qvm.contains("items")) {
        auto items = qvm["items"].toList();
        for (const QVariant &v : items) {
            QSharedPointer<Item> i(new Item());
            i->setPlaylistUuid(uuid_);
            i->fromVMap(v.toMap());
            this->items.append(i);
            this->itemsByUuid.insert(i->uuid(), i);
        }
    }
}



QueuePlaylist::QueuePlaylist(const QString &title)
    : Playlist(title)
{

}

QPair<QUuid,QUuid> QueuePlaylist::first()
{
    QReadLocker lock(&listLock);
    if (items.isEmpty())
        return QPair<QUuid,QUuid>(QUuid(),QUuid());
    return { items.first()->playlistUuid(), items.first()->uuid() };
}

QPair<QUuid,QUuid> QueuePlaylist::takeFirst()
{
    QWriteLocker lock(&listLock);
    if (items.isEmpty())
        return { QUuid(), QUuid() };
    QSharedPointer<Item> item = items.takeFirst();
    itemsByUuid.remove(item->uuid());
    item->setQueuePosition(0);
    int i = 1;
    for (auto item : items)
        item->setQueuePosition(i++);
    return { item->playlistUuid(), item->uuid() };

}

void QueuePlaylist::toggle(const QUuid &playlistUuid, const QUuid &itemUuid, bool always)
{
    QWriteLocker lock(&listLock);
    toggle_(playlistUuid, itemUuid, always);
}

void QueuePlaylist::toggle(const QUuid &playlistUuid, const QList<QUuid> &uuids)
{
    QWriteLocker lock(&listLock);
    int numberPresent = contains_(uuids);
    if (numberPresent == uuids.count()) {
        removeItems_(uuids);
        return;
    }
    for (const QUuid &item : uuids)
        toggle_(playlistUuid, item, true);
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
        items[i]->setQueuePosition(i);
}

void QueuePlaylist::removeItem(const QUuid &uuid)
{
    QWriteLocker lock(&listLock);
    removeItem_(uuid);
}

void QueuePlaylist::removeItems(const QList<QUuid> &itemsToRemove)
{
    QWriteLocker lock(&listLock);
    removeItems_(itemsToRemove);
}

void QueuePlaylist::clear()
{
    QWriteLocker lock(&listLock);
    for (QSharedPointer<Item> item : items)
        item->setQueuePosition(0);
    items.clear();
    itemsByUuid.clear();
}

int QueuePlaylist::contains(const QList<QUuid> &itemsToCheck)
{
    QReadLocker lock(&listLock);
    return contains_(itemsToCheck);
}

void QueuePlaylist::toggle_(const QUuid &playlistUuid, const QUuid &itemUuid, bool always)
{
    QWriteLocker lock(&listLock);
    if (itemsByUuid.contains(itemUuid)) {
        if (!always)
            removeItem_(itemUuid);
        return;
    }
    auto pl = PlaylistCollection::getSingleton()->playlistOf(playlistUuid);
    if (!pl)
        return;
    QSharedPointer<Item> item = pl->itemOf(itemUuid);
    if (!item)
        return;
    items.append(item);
    itemsByUuid.insert(itemUuid, item);
    item->setQueuePosition(items.count());
}

int QueuePlaylist::contains_(const QList<QUuid> &itemsToCheck) const
{
    int count = 0;
    for (QUuid item : itemsToCheck)
        if (itemsByUuid.contains(item))
            count++;
    return count;
}

void QueuePlaylist::removeItem_(const QUuid &uuid)
{
    if (!itemsByUuid.contains(uuid))
        return;
    QSharedPointer<Item> item = itemsByUuid[uuid];
    int index = items.indexOf(item);
    items.removeOne(item);
    itemsByUuid.remove(uuid);
    item->setQueuePosition(0);
    int count = items.count();
    for (int i = index; i < count; i++)
        items[i]->setQueuePosition(i+1);
}

void QueuePlaylist::removeItems_(const QList<QUuid> &itemsToRemove)
{
    for (QUuid uuid : itemsToRemove) {
        if (!itemsByUuid.contains(uuid))
            continue;
        QSharedPointer<Item> item = itemsByUuid.take(uuid);
        items.removeOne(item);
        item->setQueuePosition(0);
    }
    for (int i = 0; i < items.count(); i++)
        items[i]->setQueuePosition(i+1);
}



QSharedPointer<PlaylistCollection> PlaylistCollection::collection;

PlaylistCollection::PlaylistCollection()
    : queuePlaylist_(new QueuePlaylist("Queue"))
{
    doNewPlaylist(tr("Quick playlist"), QUuid());
}

PlaylistCollection::~PlaylistCollection()
{
    playlists.clear();
    playlistsByUuid.clear();
}

QSharedPointer<PlaylistCollection> PlaylistCollection::getSingleton()
{
    if (collection.isNull())
        collection.reset(new PlaylistCollection());
    return collection;
}

QSharedPointer<Playlist> PlaylistCollection::newPlaylist(const QString &title)
{
    return doNewPlaylist(title, QUuid::createUuid());
}

QSharedPointer<Playlist> PlaylistCollection::clonePlaylist(const QUuid &uuid)
{
    if (!playlistsByUuid.contains(uuid))
        return QSharedPointer<Playlist>();
    auto origin = playlistsByUuid[uuid];
    auto remote = newPlaylist(origin->title());
    auto cloner = [remote](QSharedPointer<Item> i) {
        remote->addItemClone(i);
    };
    origin->iterateItems(cloner);
    return remote;
}

void PlaylistCollection::removePlaylist(const QUuid &uuid)
{
    if (!playlistsByUuid.contains(uuid))
        return;
    QSharedPointer<Playlist> p = playlistsByUuid.value(uuid);
    playlists.removeAll(p);
    playlistsByUuid.remove(uuid);
}

void PlaylistCollection::removePlaylist(const QSharedPointer<Playlist> &p)
{
    if (p == NULL)
        return;
    removePlaylist(p->uuid());
}

QSharedPointer<Playlist> PlaylistCollection::playlistAt(int col) const
{
    return (col < playlists.count()) ? playlists.at(col) : QSharedPointer<Playlist>();
}

QSharedPointer<Playlist> PlaylistCollection::playlistOf(const QUuid &uuid) const
{
    return playlistsByUuid.value(uuid);
}

QSharedPointer<QueuePlaylist> PlaylistCollection::queuePlaylist() const
{
    return queuePlaylist_;
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

QSharedPointer<Playlist> PlaylistCollection::doNewPlaylist(const QString &title,
                                                           const QUuid &uuid)
{
    QSharedPointer<Playlist> p(new Playlist(title));
    p->setUuid(uuid);
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

void PlaylistSearcher::filterPlaylist(const QUuid &playlist, QString text)
{
    // Limit response - only reply if last in event queue
    bool bumpLimited = bumps() > 1;
    unbump();
    if (bumpLimited)
        return;

    QStringList needles = textToNeedles(text);
    if (needles.isEmpty()) {
        clearPlaylistFilter(playlist);
        return;
    }

    auto c = PlaylistCollection::getSingleton();
    QSharedPointer<Playlist> list = c->playlistOf(playlist);
    if (list.isNull())
        return;

    auto marker = [&needles](QSharedPointer<Item> item) {
        item->setHidden(!itemMatchesFilter(item, needles));
    };
    list->iterateItems(marker);
    emit playlistFiltered(playlist);
}

void PlaylistSearcher::clearPlaylistFilter(const QUuid &playlist)
{
    auto c = PlaylistCollection::getSingleton();
    QSharedPointer<Playlist> list = c->playlistOf(playlist);
    if (list.isNull())
        return;

    auto clearer = [](QSharedPointer<Item> item) {
        item->setHidden(false);
    };
    list->iterateItems(clearer);
    emit playlistFiltered(playlist);
}

QStringList PlaylistSearcher::textToNeedles(QString text)
{
    return text.toLower().split(QString(" "), QString::SkipEmptyParts);
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
