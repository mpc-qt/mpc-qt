#ifndef OPENFILEDIALOG_H
#define OPENFILEDIALOG_H

#include <QDialog>

namespace Ui {
class OpenFileDialog;
}

class OpenFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OpenFileDialog(QWidget *parent = nullptr);
    ~OpenFileDialog();

    QString file();
    QString subs();

private slots:
    void on_fileBrowse_clicked();

    void on_subsBrowse_clicked();

private:
    Ui::OpenFileDialog *ui = nullptr;
};

#endif // OPENFILEDIALOG_H
