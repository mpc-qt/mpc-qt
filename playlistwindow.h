#ifndef PLAYLISTWINDOW_H
#define PLAYLISTWINDOW_H

#include <QDockWidget>

namespace Ui {
class PlaylistWindow;
}

class PlaylistWindow : public QDockWidget
{
    Q_OBJECT

public:
    explicit PlaylistWindow(QWidget *parent = 0);
    ~PlaylistWindow();

private:
    Ui::PlaylistWindow *ui;
};

#endif // PLAYLISTWINDOW_H
