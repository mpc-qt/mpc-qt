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
    QUrl url() const;
    void setUrl(const QUrl &url);
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
    Playlist(const QString &title = QString());
    ~Playlist();
    QSharedPointer<Item> addItem(const QUrl &url = QUrl());
    QSharedPointer<Item> addItem(const QUuid &uuid, const QUrl &url);
    QSharedPointer<Item> addItemClone(const QSharedPointer<Item> &item);
    QSharedPointer<Item> itemOf(const QUuid &uuid);
    QSharedPointer<Item> itemAfter(const QUuid &uuid);
    QSharedPointer<Item> itemBefore(const QUuid &uuid);
    bool isEmpty() const;
    bool contains(const QUuid &uuid) const;
    void iterateItems(const std::function<void(QSharedPointer<Item>)> &callback);
    void addItems(const QUuid &where, const QList<QSharedPointer<Item> > &itemsToAdd);
    void removeItem(const QUuid &uuid);
    void takeItemsRaw(const QList<QSharedPointer<Item>> &itemsToRemove);
    void clear();

    QUuid queueFirst();
    QUuid queueTakeFirst();
    void queueToggle(const QUuid &uuid, bool always = false);
    void queueToggle(const QList<QUuid> &uuids);
    void queueAddItems(const QList<QUuid> &itemsToAdd);
    void queueRemove(const QUuid &uuid);
    void queueRemoveItems(const QList<QUuid> &itemsToRemove);
    void queueClear();
    int queueContains(const QList<QUuid> &itemsToCheck) const;

    QString title();
    void setTitle(const QString &title);
    QUuid uuid();
    void setUuid(const QUuid &uuid);

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

    QSharedPointer<Playlist> newPlaylist(const QString &title = QString());
    QSharedPointer<Playlist> clonePlaylist(const QUuid &uuid);
    void removePlaylist(const QUuid &uuid);
    void removePlaylist(const QSharedPointer<Playlist> &p);
    QSharedPointer<Playlist> playlistAt(int col) const;
    QSharedPointer<Playlist> playlistOf(const QUuid &uuid) const;

    void addPlaylist(const QSharedPointer<Playlist> &playlist);

private:
    QList<QSharedPointer<Playlist>> playlists;
    QHash<QUuid, QSharedPointer<Playlist>> playlistsByUuid;

    QSharedPointer<Playlist> doNewPlaylist(const QString &title,
                                           const QUuid &uuid);
};

class PlaylistSearcher : public QObject {
    Q_OBJECT
public:

    PlaylistSearcher() : QObject(), bumps_(0) {}
    void bump();
    void unbump();
    int bumps();

    static QStringList textToNeedles(QString text);
    static bool itemMatchesFilter(const QSharedPointer<Item> &item,
                                  const QStringList &needles);

signals:
    void playlistFiltered(QUuid playlist);

public slots:
    void filterPlaylist(const QUuid &playlist, QString text);
    void clearPlaylistFilter(const QUuid &playlist);

private:
    static void findNeedles(const QString &text, const QStringList &needles,
                            QSet<QString> &found);

    QReadWriteLock bumpLock;
    volatile int bumps_;
};


#endif // PLAYLIST_H
