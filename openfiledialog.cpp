#include "openfiledialog.h"
#include "ui_openfiledialog.h"
#include <QFileDialog>

OpenFileDialog::OpenFileDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OpenFileDialog)
{
    ui->setupUi(this);
}

OpenFileDialog::~OpenFileDialog()
{
    delete ui;
}

QString OpenFileDialog::file()
{
    return ui->fileField->text();
}

QString OpenFileDialog::subs()
{
    return ui->subsField->text();
}

void OpenFileDialog::on_fileBrowse_clicked()
{
    QUrl url = QFileDialog::getOpenFileUrl(this, "Select File");
    if (url.isValid())
        ui->fileField->setText(url.toDisplayString());
}

void OpenFileDialog::on_subsBrowse_clicked()
{
    QUrl url = QFileDialog::getOpenFileUrl(this, "Select File");
    if (url.isValid())
        ui->subsField->setText(url.toDisplayString());
}
