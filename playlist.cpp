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
    return QString("%1\t%2").arg(uuid().toString(), QString(url().toEncoded()));
}

void Item::fromString(QString input)
{
    QStringList sl = input.split("\t");
    setUuid(QUuid(sl[0]));
    setUrl(QUrl::fromPercentEncoding(sl[1].toUtf8()));
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
    sl << QString("%1\t%2").arg(uuid().toString(), title());
    for (Item *i : items)
        sl << i->toString();
    return sl;
}

void Playlist::fromStringList(QStringList sl)
{
    auto split = [](QString input, QUuid *id, QString *output) {
        QStringList s = input.split("\t");
        *id = QUuid(s[0]);
        *output = s[1];
    };

    auto iter = sl.cbegin();
    QUuid tempUuid;
    QString tempTitle;
    split(*iter, &tempUuid, &tempTitle);
    setUuid(tempUuid);
    setTitle(tempTitle);
    for (++iter; iter != sl.cend(); ++iter) {
        QUuid itemUuid;
        QString itemText;
        split(*iter, &itemUuid, &itemText);
        addItem(itemUuid, itemText);
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

Playlist *PlaylistCollection::doNewPlaylist(QString title, QUuid uuid)
{
    Playlist *p = new Playlist(title);
    p->setUuid(uuid);
    playlists.append(p);
    playlistsByUuid.insert(p->uuid(), p);
    return p;
}
