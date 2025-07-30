#include <QFileDialog>
#include "helpers.h"
#include "openfiledialog.h"
#include "ui_openfiledialog.h"

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
    static QString filter;
    static QFileDialog::Options options;
#ifdef Q_OS_MAC
    options = QFileDialog::DontUseNativeDialog;
#endif
    if (filter.isEmpty())
        filter = Helpers::fileOpenFilter();
    QUrl url = QFileDialog::getOpenFileUrl(this, tr("Select File"), QUrl(), filter,
                                           nullptr, options);
    if (url.isValid())
        ui->fileField->setText(url.toDisplayString());
}

void OpenFileDialog::on_subsBrowse_clicked()
{
    static QString subsFilter;
    static QFileDialog::Options options;
#ifdef Q_OS_MAC
    options = QFileDialog::DontUseNativeDialog;
#endif
    if (subsFilter.isEmpty())
        subsFilter = Helpers::subsOpenFilter();
    QUrl url = QFileDialog::getOpenFileUrl(this, tr("Select File"), QUrl(), subsFilter,
                                           nullptr, options);
    if (url.isValid())
        ui->subsField->setText(url.toDisplayString());
}
