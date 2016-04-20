## Media Player Classic Qute Theater

A clone of Media Player Classic reimplimented in Qt.

[Media Player Classic Home Cinema] (mpc-hc) is considered by many to be the
quintessential media player for the Windows desktop.  Media Player Classic
Qute Theater (mpc-qt) aims to reproduce most of the interface and
functionality of mpc-hc while using [libmpv] to play video instead of
DirectShow.


## Features

* play back simple video files
* control playback using the control buttons
* seek with the seek bar
* navigate by chapters
* select audio/video/subtitle tracks
* change playback rate
* resize the window automatically using zoom factors
* build playlists with the quick open command
* batch play from playlists
* launch files from from the command line
* stream video from streaming sites
* take screenshots with a customizable filename template

**Note:**  This is alpha software and some of the functionality isn't written
yet.  For the most part, unwritten portions relate to setting options,
streaming from devices, and storing favorites.


### Improvements over mpc-hc

**Multiple playlists:**  When you're watching shows on your backlog, load
every show into seperate playlists and still keep track of the last played
file *for each playlist*.  Finally you can eliminate the need to keep track of
your progress in a spreadsheet, all while never leaving the comfort of your
favorite media player.

**Quick queueing:**  Out-of-order playback in the same style of xmms/qmmp.

**More to come:** Comprehensive video-output filter support; Encoding support.
Suggestions welcome.


## Prequisities

You need the Qt5 sdk installed and a recent edition of libmpv.  On ubuntu you
can usually install the Qt5 sdk with the ``ubuntu-sdk`` or ``qtcreator``
packages.  You will need to compile [libmpv] from git head or at least version
0.17.0 with the ``--enable-libmpv-shared`` option.  Make sure ldconfig is run
after compiling libmpv, or you may end up with linker errors.


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
executable should be in `~/src/mpc-qt/build-mpc-qt...`.

Now you can create a desktop entry with the provided script, for the purposes
of sticking a shortcut to mpc-qt on your desktop. (requires Python 3.)

>./generate-localinstall-desktop.py

This will search the filesystem for the needed binaries and create the file
`Media Player Classic Qute Theater.desktop`, a usuable `.desktop` file, even
if you forgot to (or can't) run ldconfig.

## Compiling on Windows

While this program is meant for Linux, it is possible to compile it on Windows
with the [MSYS2 edition of Qt Creator] due to the largely cross-platform Qt
toolkit.

Mpc-Qt can be compiled with a libmpv linked to MSYS2's ffmpeg libraries, or by
using the prebuilt library from mpv.srsfckn.biz.  To use the prebuilt library
after cloning this repository, download libmpv from the
[mpv windows release page], and extract it somewhere.  Place the libaries for
your architechture from mpv-dev.7z (e.g. `mpv-dev.7z/64`) into `mpv-dev/lib`.
Then place the include files from mpv-dev.7z (usually at `mpv-dev.zip/include`)
into `mpv-dev/include/mpv`.  Compile with the 64bit Qt framework as usual.

[Media Player Classic Home Cinema]:https://mpc-hc.org/
[libmpv]:https://github.com/mpv-player/mpv
[mpv-build]:https://github.com/mpv-player/mpv-build
[bomi]:https://github.com/xylosper/bomi
[baka]:https://github.com/u8sand/Baka-MPlayer
[mpv windows release page]:https://mpv.srsfckn.biz/
[MSYS2 edition of Qt Creator]:https://wiki.qt.io/MSYS2
