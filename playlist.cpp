#include <QFileInfo>
#include "playlist.h"

Item::Item(QUrl url)
{
    setUrl(url);
    setUuid(QUuid::createUuid());
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
    return v;
}

void Item::fromVMap(const QVariantMap &qvm)
{
    url_ = qvm.contains("url") ? qvm.value("url").toUrl() : QUrl();
    uuid_ = qvm.contains("uuid") ? qvm.value("uuid").toUuid() : QUuid::createUuid();
}

Playlist::Playlist(QString title)
{
    setUuid(QUuid::createUuid());
    setTitle(title);
}

Playlist::~Playlist()
{
    for (Item *i : items) {
        delete i;
    }
}

Item *Playlist::addItem(QUrl url)
{
    Item *i = new Item(url);
    items.append(i);
    itemsByUuid.insert(i->uuid(), i);
    return i;
}

Item *Playlist::addItem(QUuid uuid, QUrl url)
{
    Item *i = new Item();
    i->setUrl(url);
    i->setUuid(uuid);
    items.append(i);
    itemsByUuid.insert(uuid, i);
    return i;
}

Item *Playlist::itemAt(int row)
{
    return (row < items.count()) ? items.at(row) : NULL;
}

Item *Playlist::itemOf(QUuid uuid)
{
    return itemsByUuid.value(uuid, NULL);
}

Item *Playlist::itemAfter(QUuid uuid)
{
    if (!itemsByUuid.contains(uuid))
        return NULL;
    int index = items.indexOf(itemsByUuid[uuid]);
    if (index < 0 || index + 1 >= items.length())
        return NULL;
    return items[index + 1];
}

int Playlist::indexOf(QUuid uuid)
{
    if (!itemsByUuid.contains(uuid))
        return -1;
    return items.indexOf(itemsByUuid[uuid]);
}

int Playlist::count() const
{
    return items.count();
}

void Playlist::iterateItems(std::function<void(Item *)> callback)
{
    for (Item *item : items)
        callback(item);
}

void Playlist::moveItems(int sourceRow, int destRow, int count)
{
    QList<Item*> taken;
    for (int i = 0; i < count; i++) {
        taken.append(items.takeAt(sourceRow));
    }
    for (int i = 0; i < count; i++) {
        items.insert(destRow + i, taken.at(i));
    }
}

void Playlist::addItems(int where, QList<Item *> itemsToAdd)
{
    for (int i = 0; i < itemsToAdd.count(); ++i) {
        Item *item = itemsToAdd.at(i);
        items.insert(where + i, item);
        itemsByUuid.insert(item->uuid(), item);
    }
}

void Playlist::removeItems(int where, int count)
{
    for (int i = 0; i < count; i++) {
        itemsByUuid.remove(items.at(where)->uuid());
        items.removeAt(where);
    }
}

void Playlist::removeItem(QUuid uuid)
{
    items.removeAll(itemsByUuid.take(uuid));
}

QList<Item *> Playlist::takeItems(int where, int count)
{
    QList<Item *> taken;
    for (int i = 0; i < count; i++) {
        Item *item = items.takeAt(where);
        taken.append(item);
        itemsByUuid.remove(item->uuid());
    }
    return taken;
}

void Playlist::clear()
{
    for (Item *i : items)
        delete i;
    items.clear();
    itemsByUuid.clear();
}

QString Playlist::title() const
{
    return title_;
}

void Playlist::setTitle(const QString title)
{
    title_ = title;
}

QUuid Playlist::uuid() const
{
    return uuid_;
}

void Playlist::setUuid(const QUuid uuid)
{
    uuid_ = uuid;
}

QStringList Playlist::toStringList() const
{
    QStringList sl;
    for (Item *i : items)
        sl << i->toString();
    return sl;
}

void Playlist::fromStringList(QStringList sl)
{
    items.clear();
    itemsByUuid.clear();
    for (QString s : sl) {
        auto item = new Item();
        item->fromString(s);
        items.append(item);
        itemsByUuid.insert(item->uuid(), item);
    }
}

QVariantMap Playlist::toVMap() const
{
    QVariantMap qvm;
    qvm.insert("title", title_);
    qvm.insert("uuid", uuid_);

    QVariantList qvl;
    for (const Item *i : items) {
        qvl.append(i->toVMap());
    }
    qvm.insert("items", qvl);
    return qvm;
}

void Playlist::fromVMap(const QVariantMap &qvm)
{
    title_ = qvm.contains("title") ? qvm["title"].toString() : QString();
    uuid_ = qvm.contains("uuid") ? qvm["uuid"].toUuid() : QUuid::createUuid();
    if (qvm.contains("items")) {
        auto items = qvm["items"].toList();
        for (const QVariant &v : items) {
            Item *i = new Item();
            i->fromVMap(v.toMap());
            this->items.append(i);
            this->itemsByUuid.insert(i->uuid(), i);
        }
    }
}

PlaylistCollection* PlaylistCollection::collection(NULL);

PlaylistCollection::PlaylistCollection()
{
    doNewPlaylist(tr("Quick playlist"), QUuid());
}

PlaylistCollection::~PlaylistCollection()
{
    for (Playlist *p : playlists)
        delete p;
}

PlaylistCollection *PlaylistCollection::getSingleton()
{
    if (collection == NULL)
        collection = new PlaylistCollection();
    return collection;
}

void PlaylistCollection::freeSingleton()
{
    delete collection;
    collection = NULL;
}

Playlist *PlaylistCollection::nowPlaying()
{
    return playlistOf(QUuid());
}

Playlist *PlaylistCollection::newPlaylist(QString title)
{
    return doNewPlaylist(title, QUuid::createUuid());
}

Playlist *PlaylistCollection::clonePlaylist(QUuid uuid)
{
    if (!playlistsByUuid.contains(uuid))
        return NULL;
    auto origin = playlistsByUuid[uuid];
    auto remote = newPlaylist(origin->title());
    auto cloner = [remote](Item* i) {
        remote->addItem(i->url());
    };
    origin->iterateItems(cloner);
    return remote;
}

void PlaylistCollection::removePlaylist(QUuid uuid)
{
    if (!playlistsByUuid.contains(uuid))
        return;
    Playlist *p = playlistsByUuid.value(uuid);
    playlists.removeAll(p);
    playlistsByUuid.remove(uuid);
    delete p;
}

void PlaylistCollection::removePlaylist(Playlist *p)
{
    if (p == NULL)
        return;
    removePlaylist(p->uuid());
}

Playlist *PlaylistCollection::playlistAt(int col)
{
    return (col < playlists.count()) ? playlists.at(col) : NULL;
}

Playlist *PlaylistCollection::playlistOf(QUuid uuid)
{
    return playlistsByUuid.value(uuid, NULL);
}

void PlaylistCollection::addPlaylist(Playlist *playlist)
{
    if (!playlist)
        return;
    if (playlistsByUuid.contains(playlist->uuid())) {
        Playlist *old = playlistsByUuid[playlist->uuid()];
        playlistsByUuid.remove(playlist->uuid());
        playlists.removeOne(old);
        delete old;
    }
    playlists.append(playlist);
    playlistsByUuid.insert(playlist->uuid(), playlist);
}

Playlist *PlaylistCollection::doNewPlaylist(QString title, QUuid uuid)
{
    Playlist *p = new Playlist(title);
    p->setUuid(uuid);
    playlists.append(p);
    playlistsByUuid.insert(p->uuid(), p);
    return p;
}
