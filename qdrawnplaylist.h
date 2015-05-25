#ifndef QDRAWNPLAYLIST_H
#define QDRAWNPLAYLIST_H

#include <QListWidget>
#include <QUuid>

class PlayPainter : public QAbstractItemDelegate {
public:
    PlayPainter(QObject *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const;
};


class PlayItem : public QListWidgetItem {
public:
    PlayItem(QListWidget *parent = 0, int type = QListWidgetItem::UserType);
    ~PlayItem();

    QUuid uuid();
    void setUuid(QUuid uuid);
private:
    QUuid uuid_;
};


// QDrawnPlaylist abuses a ListWidget's items to store the uuid of playlist
// items, which is decoded and looked up during drawing.  The alternative is a
// subclass of QAbstractItemModel with about 1.5x the code.
class QDrawnPlaylist : public QListWidget {
    Q_OBJECT
public:
    QDrawnPlaylist(QWidget *parent = 0);
    QUuid uuid() const;
    void setUuid(const QUuid &uuid);
    void addItem(QUuid uuid);
    void removeItem(QUuid uuid);

    QUuid nowPlayingItem();
    void setNowPlayingItem(QUuid uuid);

private:
    QUuid uuid_;
    QHash <QUuid, PlayItem*> itemsByUuid;
    QUuid nowPlayingItem_;

signals:
    // for lack of a better term that doesn't conflict with what we already
    // have, when an item is made hot by double clicking.
    void itemDesired(QUuid playlistUuid, QUuid itemUuid);

private slots:
    void model_rowsMoved(const QModelIndex & parent, int start, int end,
                         const QModelIndex & destination, int row);
    void self_itemDoubleClicked(QListWidgetItem *item);
};


#endif // QDRAWNPLAYLIST_H
