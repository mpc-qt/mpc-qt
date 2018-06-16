#ifndef PLAYLIST_H
#define PLAYLIST_H
// A set of classes suitable for creating and storing playlists.

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

class Item {
public:
    Item(QUrl url = QUrl());

    QUuid uuid() const;
    void setUuid(const QUuid &uuid);
    QUuid playlistUuid() const;
    void setPlaylistUuid(const QUuid &uuid);
    QUrl url() const;
    void setUrl(const QUrl &url);
    QVariantMap metadata() const;
    void setMetadata(const QVariantMap &qvm);

    int originalPosition();
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

    QVariantMap toVMap() const;
    void fromVMap(const QVariantMap &qvm);

private:
    QUuid uuid_;
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
    QSharedPointer<Item> itemOf(const QUuid &itemUuid);
    void removeItem(const QUuid &itemUuid);
    void storeItem(const QSharedPointer<Item> &item);

private:
    QHash<QUuid, QSharedPointer<Item>> items;
};



class Playlist : public QObject {
    Q_OBJECT
public:
    Playlist(const QString &title = QString());
    ~Playlist();
    QSharedPointer<Item> addItem(const QUrl &url = QUrl());
    QSharedPointer<Item> addItem(const QUuid &uuid, const QUrl &url);
    QSharedPointer<Item> addItemClone(const QSharedPointer<Item> &item);
    void addItemRaw(const QSharedPointer<Item> &item);

    QSharedPointer<Item> itemAt(int index);
    QSharedPointer<Item> itemOf(const QUuid &uuid);
    QSharedPointer<Item> itemAfter(const QUuid &uuid);
    QSharedPointer<Item> itemBefore(const QUuid &uuid);
    QSharedPointer<Item> itemFirst();
    QSharedPointer<Item> itemLast();
    int count();
    bool isEmpty();
    bool contains(const QUuid &uuid);
    void iterateItems(const std::function<void(QSharedPointer<Item>)> &callback);
    virtual void addItems(const QUuid &where, const QList<QSharedPointer<Item> > &itemsToAdd);
    virtual void removeItem(const QUuid &uuid);
    void takeItemsRaw(const QList<QSharedPointer<Item>> &itemsToRemove);
    QList<QUuid> replaceItem(const QUuid &where, const QList<QUrl> &urls);
    virtual void clear();

    QString title();
    void setTitle(const QString &title);
    bool shuffle();
    void setShuffle(bool shuffle);
    QUuid uuid();
    void setUuid(const QUuid &uuid);

    QStringList toStringList();
    void fromStringList(QStringList sl);

    QVariantMap toVMap();
    void fromVMap(const QVariantMap &qvm);

protected:
    QList<QSharedPointer<Item>> items;
    QHash<QUuid, QSharedPointer<Item>> itemsByUuid;
    //QList<QUuid> queue;
    QString title_;
    bool shuffle_ = false;
    QUuid uuid_;

    QReadWriteLock listLock;

    friend class QueuePlaylist;
};

class QueuePlaylist : public Playlist {
    Q_OBJECT
public:
    QueuePlaylist(const QString &title = QString());

    QPair<QUuid, QUuid> first();
    QPair<QUuid, QUuid> takeFirst();
    int toggle(const QUuid &playlistUuid, const QUuid &itemUuid, bool always = false);
    void toggle(const QUuid &playlistUuid, const QList<QUuid> &uuids, QList<QUuid> &added, QList<int> &removed);
    void toggleFromPlaylist(const QUuid &playlistUuid, QList<QUuid> &added, QList<int> &removedIndices);
    void appendItems(const QUuid &playlistUuid, const QList<QUuid> &itemsToAdd);
    void addItems(const QUuid &where, const QList<QSharedPointer<Item> > &itemsToAdd);
    void removeItem(const QUuid &uuid);
    void removeItems(const QList<QUuid> &itemsToRemove);
    void clear();
    int contains(const QList<QUuid> &itemsToCheck);

private:
    int toggle_(const QUuid &playlistUuid, const QUuid &itemUuid, bool always = false);
    int contains_(const QList<QUuid> &itemsToCheck) const;
    void removeItem_(const QUuid &uuid);
    QList<int> removeItems_(const QList<QUuid> &itemsToRemove);
};

class PlaylistCollection : public QObject {
    Q_OBJECT
private:
    PlaylistCollection();
    static QSharedPointer<PlaylistCollection> collection;

public:
    ~PlaylistCollection();
    static QSharedPointer<PlaylistCollection> getSingleton();

    QSharedPointer<Playlist> newPlaylist(const QString &title = QString());
    QSharedPointer<Playlist> clonePlaylist(const QUuid &uuid);
    void removePlaylist(const QUuid &uuid);
    void removePlaylist(const QSharedPointer<Playlist> &p);
    QSharedPointer<Playlist> playlistAt(int col) const;
    QSharedPointer<Playlist> playlistOf(const QUuid &uuid) const;
    QSharedPointer<QueuePlaylist> queuePlaylist() const;

    void addPlaylist(const QSharedPointer<Playlist> &playlist);

private:
    QList<QSharedPointer<Playlist>> playlists;
    QHash<QUuid, QSharedPointer<Playlist>> playlistsByUuid;
    QSharedPointer<QueuePlaylist> queuePlaylist_;

    QSharedPointer<Playlist> doNewPlaylist(const QString &title,
                                           const QUuid &uuid);
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
    void clearPlaylistFilter(QSharedPointer<Playlist> &list);

private:
    static void findNeedles(const QString &text, const QStringList &needles,
                            QSet<QString> &found);

    QReadWriteLock bumpLock;
    volatile int bumps_ = 0;
};


#endif // PLAYLIST_H
