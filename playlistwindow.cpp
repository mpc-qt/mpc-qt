#include "playlistwindow.h"
#include "ui_playlistwindow.h"

PlaylistWindow::PlaylistWindow(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::PlaylistWindow)
{
    ui->setupUi(this);
}

PlaylistWindow::~PlaylistWindow()
{
    delete ui;
}
