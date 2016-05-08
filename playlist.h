#ifndef PLAYLIST_H
#define PLAYLIST_H
// A set of classes suitable for creating and storing playlists.

#include <QString>
#include <QUuid>
#include <QUrl>
#include <QObject>
#include <functional>
#include <QList>
#include <QSharedPointer>
#include <QHash>
#include <QStringList>
#include <QVariantMap>
#include <QReadWriteLock>

class Item {
public:
    Item(QUrl url = QUrl());

    QUuid uuid() const;
    void setUuid(QUuid uuid);
    QUrl url() const;
    void setUrl(QUrl url);
    QVariantMap metadata() const;
    void setMetadata(const QVariantMap &qvm);

    int queuePosition() const;
    void setQueuePosition(int num);
    void decQueuePosition();

    void setHidden(bool yes);
    bool hidden();

    QString toDisplayString() const;
    QString toString() const;
    void fromString(QString input);

    QVariantMap toVMap() const;
    void fromVMap(const QVariantMap &qvm);


private:
    QUuid uuid_;
    QUrl url_;
    QVariantMap metadata_;
    int queuePosition_;
    bool hidden_;
};

class Playlist : public QObject {
    Q_OBJECT
public:
    Playlist(QString title = QString());
    ~Playlist();
    QSharedPointer<Item> addItem(QUrl url = QUrl());
    QSharedPointer<Item> addItem(QUuid uuid, QUrl url);
    QSharedPointer<Item> itemAt(int row);
    QSharedPointer<Item> itemOf(QUuid uuid);
    QSharedPointer<Item> itemAfter(QUuid uuid);
    QSharedPointer<Item> itemBefore(QUuid uuid);
    int indexOf(QUuid uuid);
    int count();
    void iterateItems(std::function<void(QSharedPointer<Item>)> callback);
    void moveItems(int sourceRow, int destRow, int count);
    void addItems(int where, QList<QSharedPointer<Item>> itemsToAdd);
    void removeItems(int where, int count);
    void removeItem(QUuid uuuid);
    QList<QSharedPointer<Item>> takeItems(int where, int count);
    void clear();

    QUuid queueFirst();
    QUuid queueTakeFirst();
    void queueToggle(QUuid uuid);
    void queueRemove(QUuid uuid);

    QString title();
    void setTitle(const QString title);
    QUuid uuid();
    void setUuid(const QUuid uuid);

    QStringList toStringList();
    void fromStringList(QStringList sl);

    QVariantMap toVMap();
    void fromVMap(const QVariantMap &qvm);


private:
    QList<QSharedPointer<Item>> items;
    QHash<QUuid, QSharedPointer<Item>> itemsByUuid;
    QList<QUuid> queue;
    QString title_;
    QUuid uuid_;

    QReadWriteLock listLock;
};

class PlaylistCollection : public QObject {
    Q_OBJECT
private:
    PlaylistCollection();
    static QSharedPointer<PlaylistCollection> collection;

public:
    ~PlaylistCollection();
    static QSharedPointer<PlaylistCollection> getSingleton();

    QSharedPointer<Playlist> nowPlaying();

    QSharedPointer<Playlist> newPlaylist(QString title = 0);
    QSharedPointer<Playlist> clonePlaylist(QUuid uuid);
    void removePlaylist(QUuid uuid);
    void removePlaylist(QSharedPointer<Playlist> p);
    QSharedPointer<Playlist> playlistAt(int col);
    QSharedPointer<Playlist> playlistOf(QUuid uuid);

    void addPlaylist(QSharedPointer<Playlist> playlist);

private:
    QList<QSharedPointer<Playlist>> playlists;
    QHash<QUuid, QSharedPointer<Playlist>> playlistsByUuid;

    QSharedPointer<Playlist> doNewPlaylist(QString title, QUuid uuid);
};

class PlaylistSearcher : public QObject {
    Q_OBJECT
public:

    PlaylistSearcher() : QObject(), bumps_(0) {}
    void bump();
    void unbump();
    int bumps();

signals:
    void playlistFiltered(QUuid playlist);

public slots:
    void filterPlaylist(QUuid playlist, QString text);
    void clearPlaylistFilter(QUuid playlist);

private:
    QReadWriteLock bumpLock;
    volatile int bumps_;
};


#endif // PLAYLIST_H
