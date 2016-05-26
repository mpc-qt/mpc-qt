#include <QFileInfo>
#include <QDebug>
#include "playlist.h"

Item::Item(QUrl url)
{
    setUrl(url);
    setUuid(QUuid::createUuid());
    setQueuePosition(0);
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
    return url().toDisplayString();
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
    items.append(i);
    itemsByUuid.insert(i->uuid(), i);
    return i;
}

QSharedPointer<Item> Playlist::addItem(const QUuid &uuid, const QUrl &url)
{
    QWriteLocker locker(&listLock);
    QSharedPointer<Item> i(new Item(url));
    i->setUrl(url);
    i->setUuid(uuid);
    items.append(i);
    itemsByUuid.insert(uuid, i);
    return i;
}

QSharedPointer<Item> Playlist::addItemClone(const QSharedPointer<Item> &item)
{
    QSharedPointer<Item> i = addItem(item->url());
    i->setMetadata(item->metadata());
    return i;
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

bool Playlist::isEmpty() const
{
    return items.isEmpty();
}

bool Playlist::contains(const QUuid &uuid) const
{
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
        items.insert(indexWhere + i, item);
        itemsByUuid.insert(item->uuid(), item);
    }
}

void Playlist::removeItem(const QUuid &uuid)
{
    QWriteLocker locker(&listLock);
    queueRemove(uuid);
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

void Playlist::clear()
{
    QWriteLocker locker(&listLock);
    items.clear();
    itemsByUuid.clear();
}

QUuid Playlist::queueFirst()
{
    if (queue.isEmpty())
        return QUuid();
    return queue.first();
}

QUuid Playlist::queueTakeFirst()
{
    if (queue.isEmpty())
        return QUuid();
    for (QUuid uuid : queue) {
        if (itemsByUuid.contains(uuid))
            itemsByUuid[uuid]->decQueuePosition();
    }
    return queue.takeFirst();
}

void Playlist::queueToggle(const QUuid &uuid, bool always)
{
    if (!itemsByUuid.contains(uuid))
        return;

    bool inQueue = queue.contains(uuid);
    if (!always && inQueue) {
        queueRemove(uuid);
    } else if (!inQueue) {
        itemsByUuid[uuid]->setQueuePosition(queue.length() + 1);
        queue.append(uuid);
    }
}

void Playlist::queueToggle(const QList<QUuid> &uuids)
{
    for (auto uuid : uuids)
        queueToggle(uuid);
}

void Playlist::queueAddItems(const QList<QUuid> &itemsToAdd)
{
    for (QUuid itemUuid : itemsToAdd)
        queueToggle(itemUuid, true);
}

void Playlist::queueRemove(const QUuid &uuid)
{
    itemsByUuid[uuid]->setQueuePosition(0);
    int a = queue.indexOf(uuid);
    if (a < 0)
        return;
    int b = queue.length();
    for (int i = a; i < b; i++)
        if (itemsByUuid.contains(queue[i]))
            itemsByUuid[queue[i]]->decQueuePosition();
    queue.removeAll(uuid);
}

void Playlist::queueRemoveItems(const QList<QUuid> &itemsToRemove)
{
    for (QUuid itemUuid : itemsToRemove)
        queueRemove(itemUuid);
}

void Playlist::queueClear()
{
    for (QUuid queueItem : queue)
        if (itemsByUuid.contains(queueItem))
           itemsByUuid[queueItem]->setQueuePosition(0);
    queue.clear();
}

int Playlist::queueContains(const QList<QUuid> &itemsToCheck) const
{
    int count;
    for (QUuid itemUuid : itemsToCheck)
        if (queue.contains(itemUuid))
            ++count;
    return count;
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
            i->fromVMap(v.toMap());
            this->items.append(i);
            this->itemsByUuid.insert(i->uuid(), i);
        }
    }
}

QSharedPointer<PlaylistCollection> PlaylistCollection::collection;

PlaylistCollection::PlaylistCollection()
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
