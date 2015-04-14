#include "hostwindow.h"
#include "ui_hostwindow.h"

HostWindow::HostWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HostWindow)
{
    ui->setupUi(this);
    ui->info_stats->setVisible(false);
}

HostWindow::~HostWindow()
{
    delete ui;
}
