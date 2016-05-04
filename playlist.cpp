#include <QFileInfo>
#include "playlist.h"

Item::Item(QUrl url)
{
    setUrl(url);
    setUuid(QUuid::createUuid());
    setQueuePosition(0);
}

QUuid Item::uuid() const
{
    return uuid_;
}

void Item::setUuid(QUuid uuid)
{
    uuid_ = uuid;
}

QUrl Item::url() const
{
    return url_;
}

void Item::setUrl(QUrl url)
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

Playlist::Playlist(QString title)
{
    setUuid(QUuid::createUuid());
    setTitle(title);
}

Playlist::~Playlist()
{
    QWriteLocker locker(&listLock);
    items.clear();
}

QSharedPointer<Item> Playlist::addItem(QUrl url)
{
    QWriteLocker locker(&listLock);
    QSharedPointer<Item> i(new Item(url));
    items.append(i);
    itemsByUuid.insert(i->uuid(), i);
    return i;
}

QSharedPointer<Item> Playlist::addItem(QUuid uuid, QUrl url)
{
    QWriteLocker locker(&listLock);
    QSharedPointer<Item> i(new Item(url));
    i->setUrl(url);
    i->setUuid(uuid);
    items.append(i);
    itemsByUuid.insert(uuid, i);
    return i;
}

QSharedPointer<Item> Playlist::itemAt(int row)
{
    QReadLocker locker(&listLock);
    return (row < items.count()) ? items.at(row) : QSharedPointer<Item>();
}

QSharedPointer<Item> Playlist::itemOf(QUuid uuid)
{
    QReadLocker locker(&listLock);
    return itemsByUuid.value(uuid, QSharedPointer<Item>());
}

QSharedPointer<Item> Playlist::itemAfter(QUuid uuid)
{
    QReadLocker locker(&listLock);
    if (!itemsByUuid.contains(uuid))
        return QSharedPointer<Item>();
    int index = items.indexOf(itemsByUuid[uuid]);
    if (index < 0 || index + 1 >= items.length())
        return QSharedPointer<Item>();
    return items[index + 1];
}

QSharedPointer<Item> Playlist::itemBefore(QUuid uuid)
{
    QReadLocker locker(&listLock);
    if (!itemsByUuid.contains(uuid))
        return QSharedPointer<Item>();
    int index = items.indexOf(itemsByUuid[uuid]);
    if (index <= 0)
        return QSharedPointer<Item>();
    return items[index - 1];
}

int Playlist::indexOf(QUuid uuid)
{
    QReadLocker locker(&listLock);
    if (!itemsByUuid.contains(uuid))
        return -1;
    return items.indexOf(itemsByUuid[uuid]);
}

int Playlist::count()
{
    QReadLocker locker(&listLock);
    return items.count();
}

void Playlist::iterateItems(std::function<void(QSharedPointer<Item>)> callback)
{
    QReadLocker locker(&listLock);
    for (auto item : items)
        callback(item);
}

void Playlist::moveItems(int sourceRow, int destRow, int count)
{
    QWriteLocker locker(&listLock);
    QList<QSharedPointer<Item>> taken;
    for (int i = 0; i < count; i++) {
        taken.append(items.takeAt(sourceRow));
    }
    for (int i = 0; i < count; i++) {
        items.insert(destRow + i, taken.at(i));
    }
}

void Playlist::addItems(int where, QList<QSharedPointer<Item>> itemsToAdd)
{
    QWriteLocker locker(&listLock);
    for (int i = 0; i < itemsToAdd.count(); ++i) {
        QSharedPointer<Item> item = itemsToAdd.at(i);
        items.insert(where + i, item);
        itemsByUuid.insert(item->uuid(), item);
    }
}

void Playlist::removeItems(int where, int count)
{
    QWriteLocker locker(&listLock);
    for (int i = 0; i < count; i++) {
        queueRemove(items.at(where)->uuid());
        itemsByUuid.remove(items.at(where)->uuid());
        items.removeAt(where);
    }
}

void Playlist::removeItem(QUuid uuid)
{
    QWriteLocker locker(&listLock);
    queueRemove(uuid);
    items.removeAll(itemsByUuid.take(uuid));
}

QList<QSharedPointer<Item>> Playlist::takeItems(int where, int count)
{
    QWriteLocker locker(&listLock);
    QList<QSharedPointer<Item>> taken;
    for (int i = 0; i < count; i++) {
        QSharedPointer<Item> item = items.takeAt(where);
        taken.append(item);
        queueRemove(item->uuid());
        itemsByUuid.remove(item->uuid());
    }
    return taken;
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

void Playlist::queueToggle(QUuid uuid)
{
    if (!itemsByUuid.contains(uuid))
        return;

    if (queue.contains(uuid)) {
        queueRemove(uuid);
    } else {
        itemsByUuid[uuid]->setQueuePosition(queue.length() + 1);
        queue.append(uuid);
    }
}

void Playlist::queueRemove(QUuid uuid)
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

QString Playlist::title()
{
    QReadLocker locker(&listLock);
    return title_;
}

void Playlist::setTitle(const QString title)
{
    QWriteLocker locker(&listLock);
    title_ = title;
}

QUuid Playlist::uuid()
{
    QReadLocker locker(&listLock);
    return uuid_;
}

void Playlist::setUuid(const QUuid uuid)
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

QSharedPointer<Playlist> PlaylistCollection::nowPlaying()
{
    return playlistOf(QUuid());
}

QSharedPointer<Playlist> PlaylistCollection::newPlaylist(QString title)
{
    return doNewPlaylist(title, QUuid::createUuid());
}

QSharedPointer<Playlist> PlaylistCollection::clonePlaylist(QUuid uuid)
{
    if (!playlistsByUuid.contains(uuid))
        return QSharedPointer<Playlist>();
    auto origin = playlistsByUuid[uuid];
    auto remote = newPlaylist(origin->title());
    auto cloner = [remote](QSharedPointer<Item> i) {
        remote->addItem(i->url());
    };
    origin->iterateItems(cloner);
    return remote;
}

void PlaylistCollection::removePlaylist(QUuid uuid)
{
    if (!playlistsByUuid.contains(uuid))
        return;
    QSharedPointer<Playlist> p = playlistsByUuid.value(uuid);
    playlists.removeAll(p);
    playlistsByUuid.remove(uuid);
}

void PlaylistCollection::removePlaylist(QSharedPointer<Playlist> p)
{
    if (p == NULL)
        return;
    removePlaylist(p->uuid());
}

QSharedPointer<Playlist> PlaylistCollection::playlistAt(int col)
{
    return (col < playlists.count()) ? playlists.at(col) : QSharedPointer<Playlist>();
}

QSharedPointer<Playlist> PlaylistCollection::playlistOf(QUuid uuid)
{
    return playlistsByUuid.value(uuid);
}

void PlaylistCollection::addPlaylist(QSharedPointer<Playlist> playlist)
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

QSharedPointer<Playlist> PlaylistCollection::doNewPlaylist(QString title, QUuid uuid)
{
    QSharedPointer<Playlist> p(new Playlist(title));
    p->setUuid(uuid);
    playlists.append(p);
    playlistsByUuid.insert(p->uuid(), p);
    return p;
}
