#include <QFileInfo>
#include "playlist.h"

Item::Item(QUrl url)
{
    setUrl(url);
    setUuid(QUuid::createUuid());
}

QUuid Item::uuid()
{
    return uuid_;
}

void Item::setUuid(QUuid uuid)
{
    uuid_ = uuid;
}

QUrl Item::url()
{
    return url_;
}

void Item::setUrl(QUrl url)
{
    url_ = url;
}

QString Item::toDisplayString()
{
    if (url().isLocalFile())
        return QFileInfo(url().toLocalFile()).completeBaseName();
    return url().toDisplayString();
}

QString Item::toString()
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
    uuid = QUuid::createUuid();
    this->title = title;
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

int Playlist::count()
{
    return items.count();
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

QStringList Playlist::toStringList()
{
    QStringList sl;
    sl << QString("%1\t%2").arg(uuid.toString(), title);
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
    split(*iter, &uuid, &title);
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
    doNewPlaylist(tr("Now playing"), QUuid());
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
    removePlaylist(p->uuid);
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
    p->uuid = uuid;
    playlists.append(p);
    playlistsByUuid.insert(p->uuid, p);
    return p;
}
