#ifndef DRAWNCOLLECTION_H
#define DRAWNCOLLECTION_H

#include <QAbstractItemDelegate>
#include <QListWidget>
#include "playlist.h"

class CollectionPainter : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    explicit CollectionPainter(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
};

class CollectionItem : public QListWidgetItem
{
public:
    explicit CollectionItem(QUuid playlistUuid, QListWidget *parent = nullptr);
    QUuid uuid();
private:
    QUuid playlistUuid_;
};


class DrawnCollection : public QListWidget
{
    Q_OBJECT
public:
    explicit DrawnCollection(QSharedPointer<PlaylistCollection> collection, QWidget *parent = nullptr);
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
