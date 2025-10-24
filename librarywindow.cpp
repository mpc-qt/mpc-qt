#include <QCloseEvent>
#include "playlist.h"
#include "widgets/drawncollection.h"
#include "widgets/drawnplaylist.h"
#include "librarywindow.h"
#include "ui_librarywindow.h"

LibraryWindow::LibraryWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LibraryWindow)
{
    ui->setupUi(this);

    collectionWidget = new DrawnCollection(PlaylistCollection::getBackup());
    ui->collectionLayout->addWidget(collectionWidget);

    playlistWidget = new DrawnPlaylist(PlaylistCollection::getBackup());
    ui->playlistLayout->addWidget(playlistWidget);

    connect(collectionWidget, &DrawnCollection::playlistSelected,
            playlistWidget, &DrawnPlaylist::setUuid);
}

LibraryWindow::~LibraryWindow()
{
    delete ui;
}

void LibraryWindow::refreshLibrary()
{
    collectionWidget->repopulatePlaylists();
}

void LibraryWindow::closeEvent(QCloseEvent *event)
{
    event->accept();
    emit windowClosed();
}

void LibraryWindow::on_restorePlaylist_clicked()
{
    int currentRow = collectionWidget->currentRow();
    if (currentRow == -1)
        return;
    auto collectionItem = static_cast<CollectionItem*>(collectionWidget->currentItem());
    auto playlistCollection = PlaylistCollection::getSingleton();
    auto backupCollection = PlaylistCollection::getBackup();

    auto playlistTaken = backupCollection->takePlaylist(collectionItem->uuid());
    playlistCollection->addPlaylist(playlistTaken);

    emit playlistRestored(playlistTaken->uuid());
    delete collectionWidget->takeItem(currentRow);
}

void LibraryWindow::on_removePlaylist_clicked()
{
    int currentRow = collectionWidget->currentRow();
    if (currentRow == -1)
        return;
    auto backupCollection = PlaylistCollection::getBackup();
    auto collectionItem = static_cast<CollectionItem*>(collectionWidget->currentItem());
    backupCollection->removePlaylist(collectionItem->uuid());
    delete collectionWidget->takeItem(currentRow);
}
