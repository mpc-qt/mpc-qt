#ifndef FILTERWINDOW_H
#define FILTERWINDOW_H

#include <QWidget>

namespace Ui {
class FilterWindow;
}

class FilterWindow : public QWidget
{
    Q_OBJECT

public:
    explicit FilterWindow(QWidget *parent = nullptr);
    ~FilterWindow();

private:
    Ui::FilterWindow *ui;
};

#endif // FILTERWINDOW_H
