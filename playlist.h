#ifndef PLAYLIST_H
#define PLAYLIST_H
// A set of classes suitable for creating and storing playlists.

#include <QDateTime>
#include <QString>
#include <QUuid>
#include <QUrl>
#include <QObject>
#include <functional>
#include <QSharedPointer>
#include <QList>
#include <QSet>
#include <QHash>
#include <QStringList>
#include <QVariantMap>
#include <QReadWriteLock>

struct PlaylistItem {
    QUuid list;
    QUuid item;
};

class Item {
public:
    explicit Item(QUrl url = QUrl());

    QUuid uuid() const;
    void setUuid(const QUuid &itemUuid);
    QUuid playlistUuid() const;
    void setPlaylistUuid(const QUuid &playlistUuid);
    QUrl url() const;
    void setUrl(const QUrl &url);
    const QVariantMap &metadata() const;
    void setMetadata(const QVariantMap &qvm);

    int originalPosition() const;
    void setOriginalPosition(int i);

    int queuePosition() const;
    void setQueuePosition(int num);
    void decQueuePosition();

    int extraPlayTimes() const;
    void setExtraPlayTimes(int amount);
    void deltaExtraPlayTimes(int delta);
    int incExtraPlayTimes();
    int decExtraPlayTimes();

    void setHidden(bool yes);
    bool hidden();

    QString toDisplayString() const;
    QString toString() const;
    void fromString(QString input);

    QVariantMap toVMap(bool savePosition) const;
    void fromVMap(const QVariantMap &qvm);

private:
    QUuid itemUuid_;
    QUuid playlistUuid_;
    QUrl url_;
    QVariantMap metadata_;
    int originalPosition_;
    int queuePosition_ = 0;
    int extraPlayTimes_ = 0;
    bool hidden_ = false;
};

class ItemCollection : public QObject {
    Q_OBJECT
private:
    ItemCollection();
    static QSharedPointer<ItemCollection> collection;

public:
    ~ItemCollection();
    static QSharedPointer<ItemCollection> getSingleton();

    QSharedPointer<Item> addItem(const QUrl url = QUrl());
    QSharedPointer<Item> addItem(const QUuid &itemUuid, const QUrl &url);
    QSharedPointer<Item> getItem(const QUuid &itemUuid);
    void removeItem(const QUuid &itemUuid);
    void storeItem(const QSharedPointer<Item> &item);

private:
    QHash<QUuid, QSharedPointer<Item>> items;
};



class Playlist : public QObject {
    Q_OBJECT
public:
    explicit Playlist(const QString &title = QString());
    ~Playlist();
    QSharedPointer<Item> addItem(const QUrl &url = QUrl());
    QSharedPointer<Item> addItem(const QUuid &itemUuid, const QUrl &url);
    QSharedPointer<Item> addItemClone(const QSharedPointer<Item> &item);
    void addItemRaw(const QSharedPointer<Item> &item);

    QSharedPointer<Item> itemAt(int index);
    QSharedPointer<Item> getItem(const QUuid &itemUuid);
    QSharedPointer<Item> itemAfter(const QUuid &itemUuid);
    QSharedPointer<Item> itemBefore(const QUuid &itemUuid);
    QSharedPointer<Item> itemFirst();
    QSharedPointer<Item> itemLast();
    int count();
    bool isEmpty();
    bool contains(const QUuid &itemUuid);
    void iterateItems(const std::function<void(QSharedPointer<Item>)> &callback);
    virtual void addItems(const QUuid &where, const QList<QSharedPointer<Item> > &itemsToAdd);
    virtual void removeItem(const QUuid &itemUuid);
    void takeItemsRaw(const QList<QSharedPointer<Item>> &itemsToRemove);
    QList<QUuid> replaceItem(const QUuid &where, const QList<QUrl> &urls);
    virtual void clear();

    QDateTime created();
    void setCreated(const QDateTime &dt);
    QString title();
    void setTitle(const QString &title);
    bool repeat();
    void setRepeat(bool repeat);
    bool shuffle();
    void setShuffle(bool shuffle);
    void shuffleItems();
    void unshuffleItems();
    QUuid uuid();
    void setUuid(const QUuid &playlistUuid);
    QUuid nowPlaying();
    void setNowPlaying(const QUuid &itemUuid);

    QStringList toStringList();
    void fromStringList(QStringList sl);

    QVariantMap toVMap();
    void fromVMap(const QVariantMap &qvm);

private:
    QList<QSharedPointer<Item>> items;
    QHash<QUuid, QSharedPointer<Item>> itemsByUuid;
    //QList<QUuid> queue;
    QDateTime created_;
    QString title_;
    bool repeat_ = false;
    bool shuffle_ = false;
    QUuid playlistUuid_;
    QUuid nowPlaying_;

    QReadWriteLock listLock;

    friend class QueuePlaylist;
};

class QueuePlaylist : public Playlist {
    Q_OBJECT
public:
    explicit QueuePlaylist(const QString &title = QString());

    PlaylistItem first();
    PlaylistItem takeFirst();
    int toggle(const QUuid &playlistUuid, const QUuid &itemUuid, bool always = false);
    void toggle(const QUuid &playlistUuid, const QList<QUuid> &uuids, QList<QUuid> &added, QList<int> &removed);
    void toggleFromPlaylist(const QUuid &playlistUuid, QList<QUuid> &added, QList<int> &removedIndices);
    void appendItems(const QUuid &playlistUuid, const QList<QUuid> &itemsToAdd);
    void addItems(const QUuid &where, const QList<QSharedPointer<Item> > &itemsToAdd) override;
    void removeItem(const QUuid &itemUuid) override;
    void removeItems(const QList<QUuid> &itemsToRemove);
    void clear() override;
    int contains(const QList<QUuid> &itemsToCheck);

private:
    int toggle_(const QUuid &playlistUuid, const QUuid &itemUuid, bool always = false);
    int contains_(const QList<QUuid> &itemsToCheck) const;
    void removeItem_(const QUuid &itemUuid);
    QList<int> removeItems_(const QList<QUuid> &itemsToRemove);
};

class PlaylistCollection : public QObject {
    Q_OBJECT
private:
    PlaylistCollection();
    static QSharedPointer<PlaylistCollection> collection;
    static QSharedPointer<PlaylistCollection> backup;
    static QSharedPointer<QueuePlaylist> queue;

public:
    ~PlaylistCollection();
    static QSharedPointer<PlaylistCollection> getSingleton();
    static QSharedPointer<PlaylistCollection> getBackup();
    static QSharedPointer<QueuePlaylist> queuePlaylist();

    void iteratePlaylists(const std::function<void(QSharedPointer<Playlist>)> &callback);
    QSharedPointer<Playlist> newPlaylist(const QString &title = QString());
    QSharedPointer<Playlist> clonePlaylist(const QUuid &playlistUuid);
    QSharedPointer<Playlist> takePlaylist(const QUuid &playlistUuid);
    void removePlaylist(const QUuid &playlistUuid);
    void removePlaylist(const QSharedPointer<Playlist> &p);
    QSharedPointer<Playlist> playlistAt(int col) const;
    QSharedPointer<Playlist> getPlaylist(const QUuid &playlistUuid) const;

    void addPlaylist(const QSharedPointer<Playlist> &playlist);
    void fromVList(const QVariantList &data);
    QVariantList toVList();

private:
    QList<QSharedPointer<Playlist>> playlists;
    QHash<QUuid, QSharedPointer<Playlist>> playlistsByUuid;

    QSharedPointer<Playlist> doNewPlaylist(const QString &title,
                                           const QUuid &playlistUuid);
};

class PlaylistSearcher : public QObject {
    Q_OBJECT
public:

    PlaylistSearcher() : QObject() {}
    void bump();
    void unbump();
    int bumps();

    static QStringList textToNeedles(QString text);
    static bool itemMatchesFilter(const QSharedPointer<Item> &item,
                                  const QStringList &needles);

signals:
    void playlistFiltered(QUuid playlist);

public slots:
    void filterPlaylist(QSharedPointer<Playlist> list, QString text);
    void clearPlaylistFilter(QSharedPointer<Playlist> const &list);

private:
    static void findNeedles(const QString &text, const QStringList &needles,
                            QSet<QString> &found);

    QReadWriteLock bumpLock;
    int bumps_ = 0;
};


#endif // PLAYLIST_H
