#include "thumbnailerwindow.h"
#include "ui_thumbnailerwindow.h"

ThumbnailerWindow::ThumbnailerWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ThumbnailerWindow)
{
    ui->setupUi(this);
}

ThumbnailerWindow::~ThumbnailerWindow()
{
    delete ui;
}
