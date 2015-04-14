#ifndef HOSTWINDOW_H
#define HOSTWINDOW_H

#include <QWidget>

namespace Ui {
class HostWindow;
}

class HostWindow : public QWidget
{
    Q_OBJECT

public:
    explicit HostWindow(QWidget *parent = 0);
    ~HostWindow();

private:
    Ui::HostWindow *ui;
};

#endif // HOSTWINDOW_H
