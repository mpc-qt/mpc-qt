#include <QCloseEvent>
#include <QMessageBox>
#include "playlist.h"
#include "widgets/drawncollection.h"
#include "widgets/drawnplaylist.h"
#include "librarywindow.h"
#include "ui_librarywindow.h"
#include "logger.h"

static constexpr char logModule[] =  "library";

LibraryWindow::LibraryWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LibraryWindow)
{
    Logger::log(logModule, "creating ui");
    ui->setupUi(this);
    Logger::log(logModule, "finished creating ui");

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

void LibraryWindow::updateLanguage()
{
    ui->retranslateUi(this);
}

void LibraryWindow::refreshLibrary()
{
    auto backupCollection = PlaylistCollection::getBackup();
    while (backupCollection->count() > 100) {
        backupCollection->removePlaylist(backupCollection->playlistAt(0));
    }
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

void LibraryWindow::on_removeAllPlaylists_clicked()
{
    auto answer = QMessageBox::question(
        this,
        tr("Remove All"),
        tr("Remove all playlists backups?"),
        QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No),
        QMessageBox::No
    );

    if (answer == QMessageBox::Yes) {
        auto backupCollection = PlaylistCollection::getBackup();
        backupCollection->removeAllPlaylists();
        collectionWidget->clear();
    }
}

