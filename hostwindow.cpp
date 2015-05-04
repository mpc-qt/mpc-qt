#include <sstream>
#include <stdexcept>

#include "hostwindow.h"
#include "ui_hostwindow.h"
#include <QDesktopWidget>
#include <QWindow>
#include <QMenuBar>
#include <QJsonDocument>
#include <QFileDialog>
#include <QTime>
#include <QMessageBox>
#include <QLibraryInfo>
#include <QDebug>

HostWindow::HostWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::HostWindow)
{
    is_fullscreen = false;
    is_playing = false;
    speed = 1.0;
    size_factor = 1;
    no_video_size = QSize(500,270);

    ui->setupUi(this);
    ui->info_stats->setVisible(false);
    connect(this, &HostWindow::fire_update_size, this, &HostWindow::send_update_size,
            Qt::QueuedConnection);

    ui_position = new QMediaSlider(this);
    ui->seekbar->layout()->addWidget(ui_position);
    connect(ui_position, &QMediaSlider::sliderMoved, this, &HostWindow::position_sliderMoved);

    mpvw = new MpvWidget(this);
    connect(mpvw, &MpvWidget::me_play_time, this, &HostWindow::me_play_time);
    connect(mpvw, &MpvWidget::me_length, this, &HostWindow::me_length);
    connect(mpvw, &MpvWidget::me_started, this, &HostWindow::me_started);
    connect(mpvw, &MpvWidget::me_pause, this, &HostWindow::me_pause);
    connect(mpvw, &MpvWidget::me_finished, this, &HostWindow::me_finished);
    connect(mpvw, &MpvWidget::me_title, this, &HostWindow::me_title);
    connect(mpvw, &MpvWidget::me_chapters, this, &HostWindow::me_chapters);
    connect(mpvw, &MpvWidget::me_tracks, this, &HostWindow::me_tracks);
    connect(mpvw, &MpvWidget::me_size, this, &HostWindow::me_size);

    // Wrap mpvw in a special QMainWindow widget so that the playlist window
    // will dock around it rather than ourselves
    mpv_host = new QMainWindow(this);
    mpv_host->setStyleSheet("background-color: black; background: center url("
                            ":/images/bitmaps/blank-screen.png) no-repeat;");
    mpv_host->setCentralWidget(mpvw);
    mpv_host->setSizePolicy(QSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred));
    ui->mpv_widget->layout()->addWidget(mpv_host);

    build_menu();
    setMenuBar(menubar);
    ui_reset_state(false);

    // Guarantee that the layout has been calculated.  It seems pointless, but
    // Without it the window will temporarily display at a larger size than
    // it needs to.
    setAttribute (Qt::WA_DontShowOnScreen, true);
    show();
    QEventLoop EventLoop (this);
    while (EventLoop.processEvents()) {}
    hide();
    setAttribute (Qt::WA_DontShowOnScreen, false);

    update_size(true);
}

HostWindow::~HostWindow()
{
    delete ui;
}

void HostWindow::menu_file_quick_open()
{
    // Do nothing special for the moment, call menu_file_open instead
    menu_file_open();
}

void HostWindow::menu_file_open()
{
    QString filename = QFileDialog::getOpenFileName(this, "Open file");
    mpvw->command_file_open(filename);
}

void HostWindow::menu_file_close()
{
    on_stop_clicked();
}

void HostWindow::menu_view_hide_menu(bool checked)
{
    // View/hide are unmanaged when in fullscreen mode
    if (is_fullscreen)
        return;

    if (checked)
        menubar->show();
    else
        menubar->hide();
    fire_update_size();
}

void HostWindow::menu_view_hide_seekbar(bool checked)
{
    if (checked)
        ui->seekbar->show();
    else
        ui->seekbar->hide();
    ui->control_section->adjustSize();
    fire_update_size();
}

void HostWindow::menu_view_hide_controls(bool checked)
{
    if (checked)
        ui->controlbar->show();
    else
        ui->controlbar->hide();
    ui->control_section->adjustSize();
    fire_update_size();
}

void HostWindow::menu_view_hide_information(bool checked)
{
    if (checked)
        ui->info_stats->show();
    else
        ui->info_stats->hide();
    ui->info_section->adjustSize();
    fire_update_size();
}

void HostWindow::menu_view_hide_statistics(bool checked)
{
    // this currently does nothing, because info and statistics are controlled
    // by the same widget.  We're going to manage what's shown by it
    // ourselves, and turn that on or off depending upon the settings here.
    (void)checked;

    fire_update_size();
}

void HostWindow::menu_view_hide_status(bool checked)
{
    if (checked)
        ui->statusbar->show();
    else
        ui->statusbar->hide();
    ui->info_section->adjustSize();
    fire_update_size();
}

void HostWindow::menu_view_hide_subresync(bool checked)
{
    if (checked)
        ui->statusbar->show();
    else
        ui->statusbar->hide();

    fire_update_size();
}

void HostWindow::menu_view_hide_playlist(bool checked)
{
    // playlist window is unimplemented for now
    (void)checked;

    fire_update_size();
}

void HostWindow::menu_view_hide_capture(bool checked)
{
    (void)checked;

    fire_update_size();
}

void HostWindow::menu_view_hide_navigation(bool checked)
{
    (void)checked;

    fire_update_size();
}

void HostWindow::menu_view_fullscreen(bool checked)
{
    is_fullscreen = checked;

    if (checked) {
        showFullScreen();
        menubar->hide();
        ui->control_section->hide();
        ui->info_section->hide();
    } else {
        showNormal();
        if (action_view_hide_menu->isChecked())
            menubar->show();
        ui->control_section->show();
        ui->info_section->show();
    }
}

void HostWindow::menu_view_zoom_50()
{
    size_factor = 0.5;
    update_size();
}

void HostWindow::menu_view_zoom_100()
{
    size_factor = 1.0;
    update_size();
}

void HostWindow::menu_view_zoom_200()
{
    size_factor = 2.0;
    update_size();
}

void HostWindow::menu_view_zoom_auto()
{
    // TODO: work out the logic for this.  In the meantime, set to manual
    // sized.
    size_factor = 0.0;
}

void HostWindow::menu_view_zoom_autolarger()
{
    // TODO: work out the logic for this.  In the meantime, set to manual
    // sized.
    size_factor = 0.0;
}

void HostWindow::menu_view_zoom_disable()
{
    size_factor = 0.0;
}

void HostWindow::menu_play_rate_reset()
{
    if (speed == 1.0)
        return;
    speed = 1.0;
    mpv_set_speed(speed);
}

void HostWindow::menu_play_volume_up()
{
    int newvol = std::min(ui->volume->value() + 10, 100);
    mpv_set_volume(newvol);
    ui->volume->setValue(newvol);
}

void HostWindow::menu_play_volume_down()
{
    int newvol = std::max(ui->volume->value() - 10, 0);
    mpv_set_volume(newvol);
    ui->volume->setValue(newvol);
}

void HostWindow::menu_navigate_chapters(int data)
{
    mpvw->property_chapter_set(data);
}

void HostWindow::menu_help_about()
{
    QMessageBox::about(this, "About Media Player Classic Qute Theater",
      "<h2>Media Player Classic Qute Theater</h2>"
      "<p>A clone of Media Player Classic written in Qt"
      "<p>Based on Qt " QT_VERSION_STR " and " + mpvw->property_version_get() +
      "<p>Built on " __DATE__ " at " __TIME__
      "<h3>LICENSE</h3>"
      "<p>   Copyright (C) 2015"
      "<p>"
      "This program is free software; you can redistribute it and/or modify "
      "it under the terms of the GNU General Public License as published by "
      "the Free Software Foundation; either version 2 of the License, or "
      "(at your option) any later version."
      "<p>"
      "This program is distributed in the hope that it will be useful, "
      "but WITHOUT ANY WARRANTY; without even the implied warranty of "
      "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
      "GNU General Public License for more details."
      "<p>"
      "You should have received a copy of the GNU General Public License along "
      "with this program; if not, write to the Free Software Foundation, Inc., "
      "51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.");
}

void HostWindow::build_menu()
{
    /*
                         =          , ,=   =@,   ==  <,    ,=          ,
               '    ,   4       '  @'Af  <668   @8   6J    [{  {   `   1,
              l{l  A   @l/    <l  @ {f  @S A   A{'  Akl    l$  { |  ,   T
             A}I  /   @[7    <l  } {f  &f A  ,'<l  {@]    { %  { ]  !    Y
             '/  '  <%@ '   <[  }'{h =$NPZ  &==j  A~$/   @[}{  &j]  {    ,,
            {A A   <6&[]    8l  [/A AY;pz @p[8&  A  !'   &.h@  +j}  {    |{
            (Uf   }R@"l[   6{  6/,U6%%[6@@[[%"l f   $   6U%u} &z[$  } {  }+
            6    ,?{Vy$j  $ { /w`{y """"""Y[[@<'   {[  "/0@@@A~{[[  @ {  f,,
           f    ^,}l $[[ <[ {}{'   '    [   `k`    [ ,JA ~$1@k@@[j }} { }'!j
         ,'    ',+/ [ '[ &[ {/`@  {     l         ^/r {   Y,"<@D8' [} [ k {'
        A    ,  }Wj "k/{,[%       /    {          +'   ;   { j &{ @<[/'{  {
       A    A  <l'! <#@(Y%{      {     l             =R    {{}" '&[W7^#'  /
      A,   ^   %/ {V U]j;^\j  n, ],   {  ,,,,,             [l` /}j%k<^
     #,  `'   Ak  {J".}"  Th+' ] @ \  &@""   , `"*~=,,    {  '  'lA\\
     .  /'    l   {l  @s,+f /  ! [ + @C,@@@@@@@@@@@@, ^[  A M  / } {
    @' }l    +        )'<' <   l '  @@@@@@@@@@@@@@@k[U{[  '   }'!'u  ,
    l  &     K     [~' ^   '~ + {   k@8%"%%@@@@@@@@[@@@l A    'j/  ,,j
   {  @"    \{    /   {   {   { {   [ ? X===,;;`"$k@@@[ }   =Y h    |l
   !  [[\   ^ [   )   %   +[   [{   "*~~~~==,,;``""'%h    =' {6   ! }
   ) {j$;,   V+   ^,  ^[   $   T$   }      ,.  ``"*<R  ;j@  e"`   j !
   ) Jj^%+    .\   Y   1   ^    "    ,    ^      ,   ~`,-^
   1 j[ VY^    ,\  .,   l            Tv,         ;=F^`          -` `
    \'T  V"(.   :\  !    j            Y `'=, ,;-`<l           -    ,
    ^[ V  };Y;,  YV  V,   .            \    ^A   '          ^
     1[`@` ^v;~;,  %p Tk,               ;   }[        ,==@@u
   .^ ^u^k,  S%@]Y, ^%,T[u,             {   '^;,,===A"' #8% "@
r`      X,"[@j"u`"zW@,^y%j Y;     `    ,{= [ +[`  6k"     [   ' '-,  '
         T[^@^[*s@, 'tU,Y[  1Yu=@==;,@h'  }'  ^[     `""~'{        `~,

    "Why is this function so huge," the developer laments.  "Perhaps it
    should be in a separate module... no, that would be wrong, it belongs
    here."

    As she thought about it some more, it slowly occurs to her that its
    ugliness would be there forever.  Qt Creator would not let her add
    menus to QWidgets in the designer.  Her only hope was making the
    module so huge that the function was not so obviously large, and she
    hates code that looks like that.

    She holds her face in her hands and cries in front of the computer.
    */
    QMenuBar *bar = new QMenuBar(this);
    QMenu *menu, *submenu, *subsubmenu;
    QAction *action;
    menu = bar->addMenu(tr("&File"));
        action = new QAction(tr("&Quick Open File..."), this);
        action->setShortcut(QKeySequence("Ctrl+Q"));
        connect(action, &QAction::triggered, this, &HostWindow::menu_file_quick_open);
        menu->addAction(action);

        menu->addSeparator();

        action = new QAction(tr("&Open File..."), this);
        action->setShortcuts(QKeySequence::Open);
        connect(action, &QAction::triggered, this, &HostWindow::menu_file_open);
        menu->addAction(action);
        addAction(action);

        action = new QAction(tr("Open &DVD/BD..."), this);
        action->setShortcut(QKeySequence("Ctrl+D"));
        //connect(action, &QAction::triggered, this, &HostWindow::);
        menu->addAction(action);
        addAction(action);

        action = new QAction(tr("Open De&vice..."), this);
        action->setShortcut(QKeySequence("Ctrl+V"));
        //connect(action, &QAction::triggered, this, &HostWindow::);
        menu->addAction(action);
        addAction(action);

        action = new QAction(tr("Open Dir&ectory"), this);
        //connect(action, &QAction::triggered, this, &HostWindow::);
        menu->addAction(action);
        addAction(action);

        submenu = menu->addMenu(tr("O&pen Disc"));
            submenu->setEnabled(false);

        submenu = menu->addMenu(tr("Recent &Files"));
            action = new QAction(tr("&Clear list"), this);
            //connect(action, &QAction::triggered, this, &HostWindow::);
            submenu->addAction(action);
            addAction(action);

            submenu->addSeparator();

        action = new QAction(tr("&Close file"), this);
        action->setShortcuts(QKeySequence::Close);
        connect(action, &QAction::triggered, this, &HostWindow::menu_file_close);
        menu->addAction(action);
        addAction(action);

        menu->addSeparator();

        action = new QAction(tr("&Save a Copy..."), this);
        //connect(action, &QAction::triggered, this, &HostWindow::);
        menu->addAction(action);
        addAction(action);

        action = new QAction(tr("Save &Image..."), this);
        action->setShortcut(QKeySequence("Alt+I"));
        //connect(action, &QAction::triggered, this, &HostWindow::);
        menu->addAction(action);
        addAction(action);

        action = new QAction(tr("Save &Thumbnails"), this);
        //connect(action, &QAction::triggered, this, &HostWindow::);
        menu->addAction(action);
        addAction(action);

        menu->addSeparator();

        action = new QAction(tr("&Load Subtitle..."), this);
        action->setShortcut(QKeySequence("Ctrl+L"));
        //connect(action, &QAction::triggered, this, &HostWindow::);
        menu->addAction(action);
        addAction(action);

        action = new QAction(tr("Save S&ubtitle..."), this);
        action->setShortcut(QKeySequence("Ctrl+S"));
        //connect(action, &QAction::triggered, this, &HostWindow::);
        menu->addAction(action);
        addAction(action);

        submenu = menu->addMenu(tr("Subtitle Data&base"));
            action = new QAction(tr("&Search..."), this);
            //connect(action, &QAction::triggered, this, &HostWindow::);
            submenu->addAction(action);
            addAction(action);

            action = new QAction(tr("&Upload..."), this);
            //connect(action, &QAction::triggered, this, &HostWindow::);
            submenu->addAction(action);
            addAction(action);

            action = new QAction(tr("&Download..."), this);
            action->setShortcut(QKeySequence("D"));
            //connect(action, &QAction::triggered, this, &HostWindow::);
            submenu->addAction(action);
            addAction(action);

        menu->addSeparator();

        action = new QAction(tr("P&roperties"), this);
        action->setShortcut(QKeySequence("Shift+F10"));
        //connect(action, &QAction::triggered, this, &HostWindow::);
        menu->addAction(action);
        addAction(action);

        menu->addSeparator();

        action = new QAction(tr("E&xit"), this);
        action->setShortcut(QKeySequence::Quit);
        connect(action, &QAction::triggered, this, &HostWindow::close);
        menu->addAction(action);
        addAction(action);

    menu = bar->addMenu(tr("&View"));
        action = new QAction(tr("Hide &Menu"), this);
        action->setShortcut(QKeySequence("Ctrl+0"));
        action->setCheckable(true);
        action->setChecked(true);
        connect(action, &QAction::toggled, this, &HostWindow::menu_view_hide_menu);
        menu->addAction(action);
        addAction(action);
        action_view_hide_menu = action;

        action = new QAction(tr("See&k Bar"), this);
        action->setShortcut(QKeySequence("Ctrl+1"));
        action->setCheckable(true);
        action->setChecked(true);
        connect(action, &QAction::toggled, this, &HostWindow::menu_view_hide_seekbar);
        menu->addAction(action);
        addAction(action);

        action = new QAction(tr("&Controls"), this);
        action->setShortcut(QKeySequence("Ctrl+2"));
        action->setCheckable(true);
        action->setChecked(true);
        connect(action, &QAction::toggled, this, &HostWindow::menu_view_hide_controls);
        menu->addAction(action);
        addAction(action);

        action = new QAction(tr("&Information"), this);
        action->setShortcut(QKeySequence("Ctrl+3"));
        action->setCheckable(true);
        connect(action, &QAction::toggled, this, &HostWindow::menu_view_hide_information);
        menu->addAction(action);
        addAction(action);

        action = new QAction(tr("&Statistics"), this);
        action->setShortcut(QKeySequence("Ctrl+4"));
        action->setCheckable(true);
        connect(action, &QAction::toggled, this, &HostWindow::menu_view_hide_statistics);
        menu->addAction(action);
        addAction(action);

        action = new QAction(tr("S&tatus"), this);
        action->setShortcut(QKeySequence("Ctrl+5"));
        action->setCheckable(true);
        action->setChecked(true);
        connect(action, &QAction::toggled, this, &HostWindow::menu_view_hide_status);
        menu->addAction(action);
        addAction(action);

        action = new QAction(tr("Su&bresync"), this);
        action->setShortcut(QKeySequence("Ctrl+6"));
        action->setCheckable(true);
        action->setEnabled(false);
        connect(action, &QAction::toggled, this, &HostWindow::menu_view_hide_subresync);
        menu->addAction(action);
        addAction(action);

        action = new QAction(tr("Play&list"), this);
        action->setShortcut(QKeySequence("Ctrl+7"));
        action->setCheckable(true);
        connect(action, &QAction::toggled, this, &HostWindow::menu_view_hide_playlist);
        menu->addAction(action);
        addAction(action);

        action = new QAction(tr("Captu&re"), this);
        action->setShortcut(QKeySequence("Ctrl+8"));
        action->setCheckable(true);
        action->setEnabled(false);
        connect(action, &QAction::toggled, this, &HostWindow::menu_view_hide_capture);
        menu->addAction(action);
        addAction(action);

        action = new QAction(tr("Na&vigation"), this);
        action->setShortcut(QKeySequence("Ctrl+9"));
        action->setCheckable(true);
        action->setEnabled(false);
        connect(action, &QAction::toggled, this, &HostWindow::menu_view_hide_navigation);
        menu->addAction(action);
        addAction(action);

        submenu = menu->addMenu(tr("&Presets"));
            action = new QAction(tr("&Minimal"), this);
            action->setShortcut(QKeySequence("1"));
            //connect(action, &QAction::triggered, this, &HostWindow::);
            submenu->addAction(action);
            addAction(action);

            action = new QAction(tr("&Compact"), this);
            action->setShortcut(QKeySequence("2"));
            //connect(action, &QAction::triggered, this, &HostWindow::);
            submenu->addAction(action);
            addAction(action);

            action = new QAction(tr("&Normal"), this);
            action->setShortcut(QKeySequence("3"));
            //connect(action, &QAction::triggered, this, &HostWindow::);
            submenu->addAction(action);
            addAction(action);

        menu->addSeparator();

        action = new QAction(tr("F&ullscreen"), this);
        action->setShortcuts(QKeySequence::FullScreen);
        action->setCheckable(true);
        connect(action, &QAction::toggled, this, &HostWindow::menu_view_fullscreen);
        menu->addAction(action);
        addAction(action);

        submenu = menu->addMenu(tr("&Zoom"));
            action = new QAction(tr("&50%"), this);
            action->setShortcut(QKeySequence("Alt+1"));
            connect(action, &QAction::triggered, this, &HostWindow::menu_view_zoom_50);
            submenu->addAction(action);
            addAction(action);

            action = new QAction(tr("&100%"), this);
            action->setShortcut(QKeySequence("Alt+2"));
            connect(action, &QAction::triggered, this, &HostWindow::menu_view_zoom_100);
            submenu->addAction(action);
            addAction(action);

            action = new QAction(tr("&200%"), this);
            action->setShortcut(QKeySequence("Alt+3"));
            connect(action, &QAction::triggered, this, &HostWindow::menu_view_zoom_200);
            submenu->addAction(action);
            addAction(action);

            action = new QAction(tr("Auto &Fit"), this);
            action->setShortcut(QKeySequence("Alt+4"));
            connect(action, &QAction::triggered, this, &HostWindow::menu_view_zoom_auto);
            submenu->addAction(action);
            addAction(action);

            action = new QAction(tr("Auto Fit (&Larger Only)"), this);
            action->setShortcut(QKeySequence("Alt+5"));
            connect(action, &QAction::triggered, this, &HostWindow::menu_view_zoom_autolarger);
            submenu->addAction(action);
            addAction(action);

            submenu->addSeparator();

            action = new QAction(tr("&Disable snapping"), this);
            action->setShortcut(QKeySequence("Alt+0"));
            connect(action, &QAction::triggered, this, &HostWindow::menu_view_zoom_disable);
            submenu->addAction(action);
            addAction(action);

        menu->addSeparator();

        submenu = menu->addMenu(tr("On &Top"));
            action = new QAction(tr("&Default"), this);
            //connect(action, &QAction::triggered, this, &HostWindow::);
            submenu->addAction(action);
            addAction(action);

            action = new QAction(tr("&Always"), this);
            action->setShortcut(QKeySequence("Ctrl+A"));
            //connect(action, &QAction::triggered, this, &HostWindow::);
            submenu->addAction(action);
            addAction(action);

            action = new QAction(tr("While &Playing"), this);
            //connect(action, &QAction::triggered, this, &HostWindow::);
            submenu->addAction(action);
            addAction(action);

            action = new QAction(tr("While Playing &Video"), this);
            //connect(action, &QAction::triggered, this, &HostWindow::);
            submenu->addAction(action);
            addAction(action);

        action = new QAction(tr("&Options..."), this);
        action->setShortcut(QKeySequence("O"));
        //connect(action, &QAction::triggered, this, &HostWindow::);
        menu->addAction(action);
        addAction(action);

    menu = bar->addMenu(tr("&Play"));
        action = new QAction(tr("&Pause"), this);
        action->setShortcut(QKeySequence("Space"));
        action->setCheckable(true);
        connect(action, &QAction::triggered, this, &HostWindow::on_pause_clicked);
        menu->addAction(action);
        addAction(action);
        action_play_pause = action;

        action = new QAction(tr("&Stop"), this);
        action->setShortcut(QKeySequence("Period"));
        connect(action, &QAction::triggered, this, &HostWindow::on_stop_clicked);
        menu->addAction(action);
        addAction(action);
        action_play_stop = action;

        action = new QAction(tr("F&rame Step Forward"), this);
        action->setShortcut(QKeySequence("Ctrl+Right"));
        connect(action, &QAction::triggered, this, &HostWindow::on_stepForward_clicked);
        menu->addAction(action);
        addAction(action);
        action_play_frame_forward = action;

        action = new QAction(tr("Fra&me Step Backward"), this);
        action->setShortcut(QKeySequence("Ctrl+Left"));
        connect(action, &QAction::triggered, this, &HostWindow::on_skipBackward_clicked);
        menu->addAction(action);
        addAction(action);
        action_play_frame_backward = action;

        menu->addSeparator();

        action = new QAction(tr("&Decrease Rate"), this);
        action->setShortcut(QKeySequence("Ctrl+Down"));
        connect(action, &QAction::triggered, this, &HostWindow::on_speedDecrease_clicked);
        menu->addAction(action);
        addAction(action);
        action_play_rate_decrease = action;

        action = new QAction(tr("&Increase Rate"), this);
        action->setShortcut(QKeySequence("Ctrl+Up"));
        connect(action, &QAction::triggered, this, &HostWindow::on_speedIncrease_clicked);
        menu->addAction(action);
        addAction(action);
        action_play_rate_increase = action;

        action = new QAction(tr("R&eset Rate"), this);
        action->setShortcut(QKeySequence("Ctrl+R"));
        connect(action, &QAction::triggered, this, &HostWindow::menu_play_rate_reset);
        menu->addAction(action);
        addAction(action);
        action_play_rate_reset = action;

        menu->addSeparator();

        submenu = menu->addMenu(tr("&Audio"));
            submenu->setEnabled(false);

        submenu = menu->addMenu(tr("Su&bitles"));
            submenu->setEnabled(false);

        submenu = menu->addMenu(tr("&Video Stream"));
            submenu->setEnabled(false);

        menu->addSeparator();

        submenu = menu->addMenu(tr("&Volume"));
            action = new QAction(tr("&Up"), this);
            action->setShortcut(QKeySequence("Up"));
            connect(action, &QAction::triggered, this, &HostWindow::menu_play_volume_up);
            submenu->addAction(action);
            addAction(action);
            action_play_volume_up = action;

            action = new QAction(tr("&Down"), this);
            action->setShortcut(QKeySequence("Down"));
            connect(action, &QAction::triggered, this, &HostWindow::menu_play_volume_down);
            submenu->addAction(action);
            addAction(action);
            action_play_volume_down = action;

            action = new QAction(tr("&Mute"), this);
            action->setShortcut(QKeySequence("Ctrl+M"));
            action->setCheckable(true);
            connect(action, &QAction::toggled, this, &HostWindow::on_mute_clicked);
            submenu->addAction(action);
            addAction(action);
            action_play_volume_mute = action;

        submenu = menu->addMenu(tr("Af&ter Playback"));
            submenu->addSection(tr("Once"));

            action = new QAction(tr("&Exit"), this);
            //connect(action, &QAction::triggered, this, &HostWindow::);
            submenu->addAction(action);
            addAction(action);

            action = new QAction(tr("&Stand By"), this);
            //connect(action, &QAction::triggered, this, &HostWindow::);
            submenu->addAction(action);
            addAction(action);

            action = new QAction(tr("&Hibernate"), this);
            //connect(action, &QAction::triggered, this, &HostWindow::);
            submenu->addAction(action);
            addAction(action);

            action = new QAction(tr("Shut&down"), this);
            //connect(action, &QAction::triggered, this, &HostWindow::);
            submenu->addAction(action);
            addAction(action);

            action = new QAction(tr("Log &Off"), this);
            //connect(action, &QAction::triggered, this, &HostWindow::);
            submenu->addAction(action);
            addAction(action);

            action = new QAction(tr("&Lock"), this);
            //connect(action, &QAction::triggered, this, &HostWindow::);
            submenu->addAction(action);
            addAction(action);

            submenu->addSection(tr("Every time"));

            action = new QAction(tr("E&xit"), this);
            //connect(action, &QAction::triggered, this, &HostWindow::);
            submenu->addAction(action);
            addAction(action);

            action = new QAction(tr("Do &Nothing"), this);
            //connect(action, &QAction::triggered, this, &HostWindow::);
            submenu->addAction(action);
            addAction(action);

            action = new QAction(tr("Play next in the &folder"), this);
            //connect(action, &QAction::triggered, this, &HostWindow::);
            submenu->addAction(action);
            addAction(action);

    menu = bar->addMenu(tr("&Navigate"));
        action = new QAction(tr("&Previous"), this);
        action->setShortcut(QKeySequence("PgUp"));
        connect(action, &QAction::triggered, this, &HostWindow::on_skipBackward_clicked);
        menu->addAction(action);
        addAction(action);

        action = new QAction(tr("&Next"), this);
        action->setShortcut(QKeySequence("PgDown"));
        connect(action, &QAction::triggered, this, &HostWindow::on_skipForward_clicked);
        menu->addAction(action);
        addAction(action);

        action = new QAction(tr("&Go To..."), this);
        action->setShortcut(QKeySequence("Ctrl+G"));
        //connect(action, &QAction::triggered, this, &HostWindow::);
        menu->addAction(action);
        addAction(action);

        submenu = menu->addMenu(tr("&Chapters"));
            submenu->setEnabled(false);
        submenu_navigate_chapters = submenu;

        menu->addSeparator();

        action = new QAction(tr("&Title Menu"), this);
        action->setShortcut(QKeySequence("Alt+T"));
        action->setEnabled(false);
        //connect(action, &QAction::triggered, this, &HostWindow::);
        menu->addAction(action);
        addAction(action);

        action = new QAction(tr("&Root Menu"), this);
        action->setShortcut(QKeySequence("Alt+R"));
        action->setEnabled(false);
        //connect(action, &QAction::triggered, this, &HostWindow::);
        menu->addAction(action);
        addAction(action);

        action = new QAction(tr("&Subtitle Menu"), this);
        action->setEnabled(false);
        //connect(action, &QAction::triggered, this, &HostWindow::);
        menu->addAction(action);
        addAction(action);

        action = new QAction(tr("&Audio Menu"), this);
        action->setEnabled(false);
        //connect(action, &QAction::triggered, this, &HostWindow::);
        menu->addAction(action);
        addAction(action);

        action = new QAction(tr("An&gle Menu"), this);
        action->setEnabled(false);
        //connect(action, &QAction::triggered, this, &HostWindow::);
        menu->addAction(action);
        addAction(action);

        action = new QAction(tr("&Chapter Menu"), this);
        action->setEnabled(false);
        //connect(action, &QAction::triggered, this, &HostWindow::);
        menu->addAction(action);
        addAction(action);

    menu = bar->addMenu(tr("F&avorites"));
        action = new QAction(tr("&Add to Favorites..."), this);
        //connect(action, &QAction::triggered, this, &HostWindow::);
        menu->addAction(action);
        addAction(action);

        action = new QAction(tr("&Organize Favorites..."), this);
        //connect(action, &QAction::triggered, this, &HostWindow::);
        menu->addAction(action);
        addAction(action);

        menu->addSeparator();

    menu = bar->addMenu(tr("&Help"));
        action = new QAction(tr("&Home Page"), this);
        //connect(action, &QAction::triggered, this, &HostWindow::);
        menu->addAction(action);
        addAction(action);

        menu->addSeparator();

        action = new QAction(tr("&About..."), this);
        connect(action, &QAction::triggered, this, &HostWindow::menu_help_about);
        menu->addAction(action);
        addAction(action);

    menubar = bar;
    (void)subsubmenu;
}

void HostWindow::ui_reset_state(bool enabled)
{
    ui_position->setEnabled(enabled);

    ui->play->setEnabled(enabled);
    ui->pause->setEnabled(enabled);
    ui->stop->setEnabled(enabled);
    ui->stepBackward->setEnabled(enabled);
    ui->speedDecrease->setEnabled(enabled);
    ui->speedIncrease->setEnabled(enabled);
    ui->stepForward->setEnabled(enabled);
    ui->skipBackward->setEnabled(enabled);
    ui->skipForward->setEnabled(enabled);

    ui->mute->setEnabled(enabled);
    ui->volume->setEnabled(enabled);

    ui->pause->setChecked(false);
    action_play_pause->setChecked(false);

    action_play_pause->setEnabled(enabled);
    action_play_stop->setEnabled(enabled);
    action_play_frame_backward->setEnabled(enabled);
    action_play_frame_forward->setEnabled(enabled);
    action_play_rate_decrease->setEnabled(enabled);
    action_play_rate_increase->setEnabled(enabled);
    action_play_rate_reset->setEnabled(enabled);
    action_play_volume_up->setEnabled(enabled);
    action_play_volume_down->setEnabled(enabled);
    action_play_volume_mute->setEnabled(enabled);

    submenu_navigate_chapters->setEnabled(enabled);
}

static QString to_date_fmt(double secs) {
    if (secs < 0.0)
        secs = 0;
    int hr = secs/3600;
    int mn = fmod(secs/60, 60);
    int se = fmod(secs, 60);
    int fr = fmod(secs,1)*100;
    return QString("%1:%2:%3.%4").arg(QString().number(hr))
            .arg(QString().number(mn),2,'0')
            .arg(QString().number(se),2,'0')
            .arg(QString().number(fr),2,'0');
}

void HostWindow::update_time()
{
    double play_time = mpvw->state_play_time_get();
    double play_length = mpvw->state_play_length_get();
    ui->time->setText(QString("%1 / %2").arg(to_date_fmt(play_time),to_date_fmt(play_length)));
}

void HostWindow::update_status()
{
    ui->status->setText(is_playing ? is_paused ? "Paused" : "Playing" : "Stopped");
}

void HostWindow::update_size(bool first_run)
{
    if (size_factor <= 0 || is_fullscreen || isMaximized())
        return;

    QSize sz_player = is_playing ? mpvw->state_video_size_get() : no_video_size;
    double factor_to_use = is_playing ? size_factor : std::max(1.0, size_factor);
    QSize sz_wanted(sz_player.width()*factor_to_use + 0.5,
                    sz_player.height()*factor_to_use + 0.5);
    QSize sz_current = mpvw->size();
    QSize sz_window = size();
    QSize sz_desired = sz_wanted + sz_window - sz_current;

    QDesktopWidget *desktop = qApp->desktop();
    if (first_run)
        setGeometry(QStyle::alignedRect(
                    Qt::LeftToRight, Qt::AlignCenter, sz_desired,
                    desktop->availableGeometry(desktop->screenNumber(QCursor::pos()))));
    else
        setGeometry(QStyle::alignedRect(
                    Qt::LeftToRight, Qt::AlignCenter, sz_desired,
                             desktop->availableGeometry(this)));
}

void HostWindow::mpv_stop(bool dry_run)
{
    if (!dry_run)
        mpvw->command_stop();
    is_playing = false;
    update_status();
    update_size();
}

void HostWindow::mpv_set_speed(double speed)
{
    mpvw->property_speed_set(speed);
    mpvw->show_message(QString("Speed: %1").arg(speed));
}

void HostWindow::mpv_set_volume(int volume)
{
    mpvw->property_volume_set(volume);
    mpvw->show_message(QString("Volume :%1%").arg(volume));
}

void HostWindow::me_play_time()
{
    double time = mpvw->state_play_time_get();
    ui_position->setValue(time >= 0 ? time : 0);
    update_time();
}

void HostWindow::me_length()
{
    double length = mpvw->state_play_length_get();
    ui_position->setMaximum(length >= 0 ? length : 0);
    update_time();
}

void HostWindow::me_started()
{
    is_playing = true;
    me_pause(false);
    ui_reset_state(true);
}

void HostWindow::me_pause(bool yes)
{
    is_paused = yes;
    update_status();
}

void HostWindow::me_finished()
{
    mpv_stop(true);
    ui_reset_state(false);
}

void HostWindow::me_title()
{
    QString window_title("Media Player Classic Qute Theater");
    QString media_title = mpvw->property_media_title_get();

    if (!media_title.isEmpty())
        window_title.append(" - ").append(media_title);
    setWindowTitle(window_title);
}

void HostWindow::me_chapters()
{
    QVariantList chapters = mpvw->state_chapters_get();
    // Here we add (named) ticks to the position slider.
    ui_position->clearTicks();
    for (QVariant v : chapters) {
        QMap<QString, QVariant> node = v.toMap();
        ui_position->setTick(node["time"].toDouble(), node["title"].toString());
    }

    // Here we populate the chapters menu with the chapters.
    QAction *action;
    data_emitter *emitter;
    submenu_navigate_chapters->clear();
    int index = 0;
    for (QVariant v : chapters) {
        QMap<QString, QVariant> node = v.toMap();
        action = new QAction(this);
        action->setText(QString("[%1] - %2").arg(
                            to_date_fmt(node["time"].toDouble()),
                            node["title"].toString()));
        emitter = new data_emitter(action);
        emitter->data = index;
        connect(action, &QAction::triggered, emitter, &data_emitter::got_something);
        connect(emitter, &data_emitter::heres_something, this, &HostWindow::menu_navigate_chapters);
        submenu_navigate_chapters->addAction(action);
        index++;
    }
}

void HostWindow::me_tracks()
{
    QVariantList tracks = mpvw->state_tracks_get();
    (void)tracks;
}

void HostWindow::me_size()
{
    update_size();
}

void HostWindow::position_sliderMoved(int position)
{
    mpvw->property_time_set(position);
}

void HostWindow::on_pause_clicked(bool checked)
{
    mpvw->property_pause_set(checked);
    me_pause(checked);    

    ui->pause->setChecked(checked);
    action_play_pause->setChecked(checked);
}

void HostWindow::on_play_clicked()
{
    if (!is_playing)
        return;
    if (is_paused) {
        mpvw->property_pause_set(false);
        me_pause(false);
        ui->pause->setChecked(false);
    }
    if (speed != 1.0) {
        speed = 1.0;
        mpv_set_speed(speed);
    }
}

void HostWindow::on_stop_clicked()
{
    mpv_stop();
}

void HostWindow::on_speedDecrease_clicked()
{
    if (speed <= 0.125)
        return;
    speed /= 2;
    mpv_set_speed(speed);
}

void HostWindow::on_speedIncrease_clicked()
{
    if (speed >= 8.0)
        return;
    speed *= 2;
    mpv_set_speed(speed);
}

void HostWindow::on_skipBackward_clicked()
{
    int64_t chapter = mpvw->property_chapter_get();
    if (chapter > 0) chapter--;
    mpvw->property_chapter_set(chapter);
}

void HostWindow::on_skipForward_clicked()
{
    int64_t chapter = mpvw->property_chapter_get();
    chapter++;
    if (!mpvw->property_chapter_set(chapter)) {
        // most likely the reason why we're here is because the requested
        // chapter number is a past-the-end value, so halt playback.  If mpv
        // was playing back a playlist, this stops it.  But we intend to do
        // our own playlist parsing anyway, so no biggie.
        mpv_stop();
    }
}

void HostWindow::on_stepBackward_clicked()
{
    mpvw->command_step_backward();
    is_paused = true;
    update_status();
}

void HostWindow::on_stepForward_clicked()
{
    mpvw->command_step_forward();
    is_paused = true;
    update_status();
}

void HostWindow::on_volume_valueChanged(int position)
{
    mpv_set_volume(position);
}

void HostWindow::on_mute_clicked(bool checked)
{
    if (!is_playing)
        return;
    mpvw->property_mute_set(checked);
    ui->mute->setIcon(QIcon(checked ? ":/images/controls/speaker2.png" :
                                      ":/images/controls/speaker1.png"));
    action_play_volume_mute->setChecked(checked);
    ui->mute->setChecked(checked);
}

void HostWindow::send_update_size()
{
    update_size();
}
