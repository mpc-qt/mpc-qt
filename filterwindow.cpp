#include "filterwindow.h"
#include "ui_filterwindow.h"

// Setting vf to "lavfi=graph=\"gradfun=radius=30:strength=20,vflip\"" (as per
// the mpv manual) upon casual inspection with our json ipc server like so
//   echo '{ "command": "getMpvProperty", "name": "vf" }' | socat /tmp/cmdrkotori.mpc-qt -
// yields the following data structure:
//   [
//     {
//       "enabled":true,
//       "name":"lavfi",
//       "params": {
//         "graph":"gradfun=radius=30:strength=20,vflip"
//       }
//     }
//   ]
// So it would seem that setting a lavfi graph requires building a graph
// string rather than doing anything special with variants and the like.

FilterWindow::FilterWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FilterWindow)
{
    ui->setupUi(this);
}

FilterWindow::~FilterWindow()
{
    delete ui;
}
