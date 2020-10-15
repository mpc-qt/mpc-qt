#ifndef DRAWNCOLLECTION_H
#define DRAWNCOLLECTION_H

#include <QAbstractItemDelegate>
#include <QListWidget>
#include "playlist.h"

class CollectionPainter : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    CollectionPainter(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const;
};

class CollectionItem : public QListWidgetItem
{
public:
    CollectionItem(QUuid uuid, QListWidget *parent = nullptr);
    QUuid uuid();
private:
    QUuid uuid_;
};


class DrawnCollection : public QListWidget
{
    Q_OBJECT
public:
    DrawnCollection(QSharedPointer<PlaylistCollection> collection, QWidget *parent = nullptr);
    QSharedPointer<PlaylistCollection> collection();

    void addPlaylist(QUuid playlistUuid);
    QUuid currentPlaylistUuid();

signals:
    void playlistSelected(QUuid);

public slots:
    void repopulatePlaylists();

private slots:
    void self_currentRowChanged(int currentRow);

private:
    QSharedPointer<PlaylistCollection> collection_;
};

#endif // DRAWNCOLLECTION_H
