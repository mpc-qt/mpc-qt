#ifndef QDRAWNPLAYLIST_H
#define QDRAWNPLAYLIST_H

#include <QListWidget>
#include <QUuid>
#include <functional>
#include "playlist.h"

class DisplayParser;
class QThread;
class PlaylistSearcher;

class PlayPainter : public QAbstractItemDelegate {
    Q_OBJECT
public:
    PlayPainter(QObject *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const;
};


class PlayItem : public QListWidgetItem {
public:
    PlayItem(const QUuid &uuid = QUuid(), const QUuid &playlistUuid = QUuid(), QListWidget *parent = 0);
    ~PlayItem();

    QUuid playlistUuid();
    QUuid uuid();

private:
    QUuid playlistUuid_;
    QUuid uuid_;
};


// QDrawnPlaylist abuses a ListWidget's items to store the uuid of playlist
// items, which is decoded and looked up during drawing.  The alternative is a
// subclass of QAbstractItemModel with about 1.5x the code.
class QDrawnPlaylist : public QListWidget {
    Q_OBJECT
public:
    QDrawnPlaylist(QWidget *parent = 0);
    ~QDrawnPlaylist();
    virtual QSharedPointer<Playlist> playlist() const;
    QUuid uuid() const;
    void setUuid(const QUuid &uuid);
    QUuid currentItemUuid() const;
    QList<QUuid> currentItemUuids() const;
    void traverseSelected(std::function<void(QUuid)> callback);
    void setCurrentItem(QUuid itemUuid);
    void scrollToItem(QUuid itemUuid);
    virtual void addItem(QUuid uuid);
    void addItems(const QList<QUuid> &items);
    void addItemsAfter(QUuid item, const QList<QUuid> &items);
    void removeItem(QUuid uuid);
    void removeItems(const QList<int> &indicies);
    void removeAll();

    QPair<QUuid,QUuid> importUrl(QUrl url);
    void currentToQueue();

    QUuid nowPlayingItem();
    void setNowPlayingItem(QUuid uuid);

    QVariantMap toVMap() const;
    void fromVMap(const QVariantMap &qvm);

    void setDisplayParser(DisplayParser *parser);
    DisplayParser *displayParser();

    void setFilter(QString needles);

protected:
    bool event(QEvent *e);

private:
    QUuid uuid_;
    QHash <QUuid, PlayItem*> itemsByUuid;
    QUuid lastSelectedItem;
    QUuid nowPlayingItem_;
    DisplayParser *displayParser_ = nullptr;
    QThread *worker = nullptr;
    PlaylistSearcher *searcher;
    QString currentFilterText;
    QStringList currentFilterList;

signals:
    // for lack of a better term that doesn't conflict with what we already
    // have, when an item is made hot by double clicking.
    void itemDesired(QUuid playlistUuid, QUuid itemUuid);
    void searcher_filterPlaylist(QSharedPointer<Playlist>, QString text);
    void menuOpenItem(QUuid playlistUuid, QUuid itemUuid);

    void contextMenuRequested(QPoint p, QUuid playlistUuid, QUuid itemUuid);

private slots:
    void repopulateItems();

    void model_rowsMoved(const QModelIndex & parent, int start, int end,
                         const QModelIndex & destination, int row);
    void self_currentItemChanged(QListWidgetItem *current,
                                 QListWidgetItem *previous);
    void self_itemDoubleClicked(QListWidgetItem *item);
    void self_customContextMenuRequested(const QPoint &p);
};


class QDrawnQueue : public QDrawnPlaylist {
    Q_OBJECT
public:
    virtual QSharedPointer<Playlist> playlist() const;
    void addItem(QUuid uuid);
};

class PlaylistSelectionPrivate;
class PlaylistSelection {
public:
    PlaylistSelection();
    PlaylistSelection(PlaylistSelection &other);
    ~PlaylistSelection();
    void fromItem(QUuid playlistUuid, QUuid itemUuid);
    void fromQueue(QDrawnPlaylist *list);
    void fromSelected(QDrawnPlaylist *list);

    void appendToPlaylist(QDrawnPlaylist *list);
    void appendAndQuickQueue(QDrawnPlaylist *list);

private:
    PlaylistSelectionPrivate *d = nullptr;
};
#endif // QDRAWNPLAYLIST_H
