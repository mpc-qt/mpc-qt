#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <QFile>
#include <QStringList>
#include <QTimer>
#include <QWidget>

namespace Ui {
class LogWindow;
}

class LogWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LogWindow(QWidget *parent = nullptr);
    ~LogWindow();

signals:
    void windowClosed();

public slots:
    void appendMessage(QString message);
    void appendMessageBlock(QStringList messages);
    void setLogLimit(int lines);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_copy_clicked();
    void on_save_clicked();
    void on_clear_clicked();
    
private:
    Ui::LogWindow *ui;
};



#endif // LOGWINDOW_H
