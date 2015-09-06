#include "playlistwindow.h"
#include "ui_playlistwindow.h"
#include "qdrawnplaylist.h"

PlaylistWindow::PlaylistWindow(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::PlaylistWindow)
{
    ui->setupUi(this);
    auto qdp = new QDrawnPlaylist();
    qdp->setUuid(QUuid());
    ui->tabWidget->addTab(qdp, tr("Quick Playlist"));
}

PlaylistWindow::~PlaylistWindow()
{
    delete ui;
}
