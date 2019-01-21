#ifndef THUMBNAILERWINDOW_H
#define THUMBNAILERWINDOW_H

#include <QWidget>

namespace Ui {
class ThumbnailerWindow;
}

class ThumbnailerWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ThumbnailerWindow(QWidget *parent = nullptr);
    ~ThumbnailerWindow();

private:
    Ui::ThumbnailerWindow *ui;
};

#endif // THUMBNAILERWINDOW_H
