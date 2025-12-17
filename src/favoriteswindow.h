#ifndef FAVORITESWINDOW_H
#define FAVORITESWINDOW_H

#include <QAbstractButton>
#include <QListWidget>
#include <QStyledItemDelegate>
#include <QWidget>
#include "helpers.h"

namespace Ui {
class FavoritesWindow;
}

class FavoritesList;
class FavoritesModel;
class FavoritesDelegate;

class FavoritesWindow : public QWidget
{
    Q_OBJECT

public:
    explicit FavoritesWindow(QWidget *parent = nullptr);
    ~FavoritesWindow();

signals:
    void favoriteTracks(QList<TrackInfo> files, QList<TrackInfo> streams);
    void favoriteTracksCancel();

public slots:
    void setFiles(const QList<TrackInfo> &tracks);
    void setStreams(const QList<TrackInfo> &tracks);
    void addTrack(const TrackInfo &track);
    void updateFavoriteTracks();

    void on_buttonBox_clicked(QAbstractButton *button);

private slots:
    void on_remove_clicked();

private:
    Ui::FavoritesWindow *ui;
    FavoritesList *fileList;
    FavoritesList *streamList;
};

class FavoritesList : public QListWidget
{
    Q_OBJECT

public:
    explicit FavoritesList(QWidget *parent = nullptr);
    ~FavoritesList();
    TrackInfo getTrack(int index);
    void setTracks(const QList<TrackInfo> &tracks);
    void addTrack(const TrackInfo &track);
    QList<TrackInfo> tracks();
};

class FavoritesItem : public QListWidgetItem
{
public:
    explicit FavoritesItem(QListWidget *owner, const TrackInfo &t);
    ~FavoritesItem();
    TrackInfo &track() { return track_; }
private:
    TrackInfo track_;
};


class FavoritesDelegate : public QAbstractItemDelegate
{
    Q_OBJECT

public:
    explicit FavoritesDelegate(QWidget *parent = nullptr);
    ~FavoritesDelegate();
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    FavoritesList *owner;
};

#endif // FAVORITESWINDOW_H
