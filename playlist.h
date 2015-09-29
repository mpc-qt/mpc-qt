#ifndef PLAYLIST_H
#define PLAYLIST_H
// A set of classes suitable for creating and storing playlists.

#include <QString>
#include <QUuid>
#include <QUrl>
#include <QObject>
#include <functional>
#include <QList>
#include <QHash>
#include <QStringList>
#include <QVariantMap>

class Item {
public:
    Item(QUrl url = QUrl());

    QUuid uuid() const;
    void setUuid(QUuid uuid);
    QUrl url() const;
    void setUrl(QUrl url);

    QString toDisplayString() const;
    QString toString() const;
    void fromString(QString input);

    QVariantMap toVMap() const;
    void fromVMap(const QVariantMap &qvm);

private:
    QUuid uuid_;
    QUrl url_;
};

class Playlist : public QObject {
    Q_OBJECT
public:
    Playlist(QString title = QString());
    ~Playlist();
    Item *addItem(QUrl url = QUrl());
    Item *addItem(QUuid uuid, QUrl url);
    Item *itemAt(int row);
    Item *itemOf(QUuid uuid);
    Item *itemAfter(QUuid uuid);
    int indexOf(QUuid uuid);
    int count() const;
    void iterateItems(std::function<void(Item *)> callback);
    void moveItems(int sourceRow, int destRow, int count);
    void addItems(int where, QList<Item*> itemsToAdd);
    void removeItems(int where, int count);
    void removeItem(QUuid uuid);
    QList<Item*> takeItems(int where, int count);
    void clear();

    QString title() const;
    void setTitle(const QString title);
    QUuid uuid() const;
    void setUuid(const QUuid uuid);

    QStringList toStringList() const;
    void fromStringList(QStringList sl);

    QVariantMap toVMap() const;
    void fromVMap(const QVariantMap &qvm);

private:
    QList<Item*> items;
    QHash<QUuid, Item*> itemsByUuid;
    QString title_;
    QUuid uuid_;
};

class PlaylistCollection : public QObject {
    Q_OBJECT
private:
    PlaylistCollection();
    ~PlaylistCollection();
    static PlaylistCollection* collection;

public:
    static PlaylistCollection *getSingleton();
    static void freeSingleton();

    Playlist *nowPlaying();

    Playlist *newPlaylist(QString title = 0);
    Playlist *clonePlaylist(QUuid uuid);
    void removePlaylist(QUuid uuid);
    void removePlaylist(Playlist *p);
    Playlist *playlistAt(int col);
    Playlist *playlistOf(QUuid uuid);

    void addPlaylist(Playlist *playlist);

private:
    QList<Playlist*> playlists;
    QHash<QUuid, Playlist*> playlistsByUuid;

    Playlist *doNewPlaylist(QString title, QUuid uuid);
};


#endif // PLAYLIST_H
