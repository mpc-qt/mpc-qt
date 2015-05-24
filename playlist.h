#ifndef PLAYLIST_H
#define PLAYLIST_H
// A set of classes suitable for creating and storing playlists.

#include <QString>
#include <QUuid>
#include <QObject>
#include <QList>
#include <QHash>
#include <QStringList>

class Item {
public:
    Item(QString text = 0);
    QString text;

    QUuid uuid();
    void setUuid(QUuid uuid);
    QString toString();
    void fromString(QString input);

private:
    QUuid uuid_;
};

class Playlist : public QObject {
    Q_OBJECT
public:
    Playlist();
    Playlist(QString title);
    ~Playlist();
    Item *addItem(QString text = 0);
    Item *addItem(QUuid uuid, QString text);
    Item *itemAt(int row);
    Item *itemOf(QUuid uuid);
    int count();
    void moveItems(int sourceRow, int destRow, int count);
    void addItems(int where, QList<Item*> itemsToAdd);
    void removeItems(int where, int count);
    void removeItem(QUuid uuid);
    QList<Item*> takeItems(int where, int count);
    void clear();

    QStringList toStringList();
    void fromStringList(QStringList sl);

    QList<Item*> items;
    QHash<QUuid, Item*> itemsByUuid;
    QString title;
    QUuid uuid;
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
    void removePlaylist(QUuid uuid);
    void removePlaylist(Playlist *p);
    Playlist *playlistAt(int col);
    Playlist *playlistOf(QUuid uuid);


private:
    QList<Playlist*> playlists;
    QHash<QUuid, Playlist*> playlistsByUuid;

    Playlist *doNewPlaylist(QString title, QUuid uuid);
};


#endif // PLAYLIST_H
