## Media Player Classic Qute Theater

A clone of Media Player Classic reimplimented in Qt.

[Media Player Classic Home Cinema] is considered by many to be the
quintessential media player for the Windows desktop.  I would like mpc-qt to
reproduce most of the interface and functionality of mpc-hc while using
[libmpv] to play video instead of DirectShow.


## Features

This is alpha software and some of the functionality isn't written yet. For
the most part, unwritten portions relate to setting options, streaming from
the internet and devices, and storing favorites.  However, you can still

* play back simple video files
* control playback using the control buttons
* seek with the seek bar
* navigate by chapters
* select audio/video/subtitle tracks
* change playback rate
* resize the window automatically using zoom factors
* build playlists with the quick open command
* batch play from playlists

As time goes on, features will be implemented, added or dropped given the
use-case of mpc-qt.  For example, there *are* a few things which will likely
never be implemented, such as the silly DirectShow output filter chains that
libmpv simply has no need for.  On the other hand, libmpv also provides nice
features that are not present in mpc-hc and these functionalities may be
exposed as the developer sees fit.


### Improvements over mpc-hc

**Multiple playlists:**  When you're watching shows on your backlog, load
every show into seperate playlists and still keep track of the last played
file *for each playlist*.  Finally you can eliminate the need to keep track of
your progress in a spreadsheet, all while never leaving the comfort of your
favorite media player.

**More to come:** Comprehensive video-output filter support; Encoding support.
Suggestions welcome.


## Prequisities

You need the Qt5 sdk installed and an edition of libmpv.  On ubuntu you can
install them with

>sudo apt-get install qtcreator libmpv-dev

If you want Qt5 documentation in Qt Creator, add *qt5-doc* to that command.

However, I recommend compiling libmpv from git head due to the fast
development pace of mpv and old packages in package repositories. (I develop
against mpv's head.) So you may want to follow the instructions at [mpv-build]
while making sure to place the line `--enable-libmpv-shared` inside the
`mpv_options` file before building as it says.  The libmpv api does not change
much however, so you should be okay if you don't feel like doing this and are
willing to put up with missing features.


## I don't know git, how do I run this?

You'll have to perform a little bit of footwork beforehand.  What you're going
to do is make a directory for this repo to sit in, and then compile it.

First ensure you have the prequisites as mentioned above, then open a terminal
and `cd` into your general source-code directory. If one does not exist,
`mkdir` one.

>mkdir src

>cd ~/src

Then, make a directory to checkout this git into, and `cd` into that.

>mkdir mpc-qt

>cd mpc-qt

At this stage you can checkout this git repository using the following
command:

>git checkout https://github.com/cmdrkotori/mpc-qt.git

Finally, `cd` into the checked-out repository and run qtcreator.

>cd mpc-qt

>qtcreator mpc-qt.pro

Use qtcreator's suggested build setup and click "Configure Project", then
press the big green arrow button in the bottom-left corner.  After this, the
executable should be in `~/src/mpc-qt/build-mpc-qt...`. Look for the `mpc-qt`
binary and run that whenever you want to, or stick a shortcut to that on your
taskbar/desktop.

[Media Player Classic Home Cinema]:https://mpc-hc.org/
[libmpv]:https://github.com/mpv-player/mpv
[mpv-build]:https://github.com/mpv-player/mpv-build
[bomi]:https://github.com/xylosper/bomi
[baka]:https://github.com/u8sand/Baka-MPlayer
