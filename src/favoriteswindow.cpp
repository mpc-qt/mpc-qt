#include <QLineEdit>
#include <QPainter>

#include "favoriteswindow.h"
#include "ui_favoriteswindow.h"
#include "logger.h"

static constexpr char logModule[] =  "favorites";

FavoritesWindow::FavoritesWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FavoritesWindow)
{
    Logger::log(logModule, "creating ui");
    ui->setupUi(this);
    Logger::log(logModule, "finished creating ui");
    fileList = new FavoritesList(this);
    streamList = new FavoritesList(this);
    ui->filesTab->layout()->addWidget(fileList);
    ui->streamsTab->layout()->addWidget(streamList);
}

FavoritesWindow::~FavoritesWindow()
{
    delete ui;
}

void FavoritesWindow::setFiles(const QList<TrackInfo> &tracks)
{
    fileList->setTracks(tracks);
}

void FavoritesWindow::setStreams(const QList<TrackInfo> &tracks)
{
    streamList->setTracks(tracks);
}

void FavoritesWindow::addTrack(const TrackInfo &track)
{
    if (track.url.isLocalFile())
        fileList->addTrack(track);
    else
        streamList->addTrack(track);
    updateFavoriteTracks();
}

void FavoritesWindow::updateFavoriteTracks()
{
    emit favoriteTracks(fileList->tracks(), streamList->tracks());
}

void FavoritesWindow::on_remove_clicked()
{
    FavoritesList *active = ui->tabWidget->currentIndex() == 0 ? fileList : streamList;
    for (auto &i : active->selectedItems())
        delete i;
}

void FavoritesWindow::on_buttonBox_clicked(QAbstractButton *button)
{
    QDialogButtonBox::ButtonRole buttonRole;
    buttonRole = ui->buttonBox->buttonRole(button);
    if (buttonRole == QDialogButtonBox::AcceptRole)
        updateFavoriteTracks();
    if (buttonRole == QDialogButtonBox::RejectRole)
        emit favoriteTracksCancel();
    close();
}

//---------------------------------------------------------------------------

FavoritesList::FavoritesList(QWidget *parent)
    : QListWidget(parent)
{
    setItemDelegateForColumn(0, new FavoritesDelegate(this));
    setSelectionMode(QAbstractItemView::ContiguousSelection);
    setDragDropMode(QAbstractItemView::InternalMove);
}

FavoritesList::~FavoritesList()
{

}

void FavoritesList::setTracks(const QList<TrackInfo> &tracks)
{
    clear();
    for (const auto &t : tracks)
        addTrack(t);
}

void FavoritesList::addTrack(const TrackInfo &track)
{
    auto item = new FavoritesItem(this, track);
    this->addItem(item);
}

QList<TrackInfo> FavoritesList::tracks()
{
    QList<TrackInfo> list;
    int rows = count();
    for (int i = 0; i < rows; i++)
        list.append(static_cast<FavoritesItem*>(item(i))->track());
    return list;
}

//---------------------------------------------------------------------------

FavoritesItem::FavoritesItem(QListWidget *owner, const TrackInfo &t)
    : QListWidgetItem(owner)
{
    track_ = t;
    Qt::ItemFlags f = flags();
    f.setFlag(Qt::ItemIsEditable);
    this->setFlags(f);
}

FavoritesItem::~FavoritesItem()
{

}

//---------------------------------------------------------------------------

FavoritesDelegate::FavoritesDelegate(QWidget *parent)
    : QAbstractItemDelegate(parent)
    , owner(static_cast<FavoritesList*>(parent))
{

}

FavoritesDelegate::~FavoritesDelegate()
{

}

QWidget *FavoritesDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    return new QLineEdit(parent);
}

void FavoritesDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    FavoritesItem *item = static_cast<FavoritesItem*>(owner->item(index.row()));
    static_cast<QLineEdit*>(editor)->setText(item->track().title);
}

void FavoritesDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    Q_UNUSED(model)
    FavoritesItem *item = static_cast<FavoritesItem*>(owner->item(index.row()));
    QString title = static_cast<QLineEdit*>(editor)->text();
    if (title.isEmpty())
        title = item->track().url.toString();
    item->track().title = title;
}

QSize FavoritesDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QApplication::style()->sizeFromContents(QStyle::CT_ItemViewItem,
                                                   &option,
                                                   option.rect.size());
}

void FavoritesDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    auto listWidget = qobject_cast<FavoritesList*>(parent());
    auto listItem = static_cast<FavoritesItem*>(listWidget->item(index.row()));

    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &option, painter);

    QString title = listItem->track().title;
    QString time = Helpers::toDateFormat(listItem->track().position) + " / "
                   + Helpers::toDateFormat(listItem->track().length);
    QRect rc = option.rect.adjusted(3, 0, -3, 0);
    painter->drawText(rc, Qt::AlignRight|Qt::AlignCenter, time);
    rc.adjust(0, 0, -(3 + option.fontMetrics.horizontalAdvance(time)), 0);
    painter->drawText(rc, Qt::AlignLeft|Qt::AlignVCenter, title);
}

void FavoritesDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)
    editor->setGeometry(option.rect);
}
