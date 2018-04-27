## Media Player Classic Qute Theater

A clone of Media Player Classic reimplemented in Qt.

![screenshot]

[Media Player Classic Home Cinema][mpc-hc] (mpc-hc) is considered by many to
be the quintessential media player for the Windows desktop.  Media Player
Classic Qute Theater (mpc-qt) aims to reproduce most of the interface and
functionality of mpc-hc while using [libmpv] to play video instead of
DirectShow.


## Releases

There is no RC.  Despite this situation, you may test it out and determine if
what works is satisfying for you.  If not, please open an issue that may
motivate the developer in a helpful direction.

The best version is git master, and everyone are encouraged to increase their
computer-fu by compiling from source. (see sections below.)  Compiling from
source gives you several advantages over the usual user, such the ability to
use latest and pre-release software regardless of where it comes from.  Unix
users, there could even be packages in your distro that help with this. (e.g.
[aur], [ports].)

There are builds for Windows users on the release page.  Every now and then
the developer makes a Windows build based on a recent commit and posts it on
the releases page.  These use time-based versioning (e.g. 17.07 corresponds to
2017 July), are provided for the convenience of Windows users who usually do
not have a development environment, and should not be considered to represent
any serious release-worthy snapshot in any way.  This may change when the
program is more feature-complete.


## Features

Nearly everything that mpc-hc does.  For the most part, unwritten
portions relate to setting options and streaming from devices.


### Improvements over mpc-hc

**Multiple playlists:**  When you're watching shows on your backlog, load
every show into separate playlists and still keep track of the last played
file *for each playlist*.  Finally you can eliminate the need to keep track of
your progress in a spreadsheet, all while never leaving the comfort of your
favorite media player.

**Quick queuing:**  Out-of-order playback in the same style of xmms/qmmp.
Got some compilation albums in a playlist, but want to hear only some rock
tracks for a while?  Now you can, without obliterating your playlist.

**Playlist searching:**  Multi-threaded playlist searching, in the same style
as other media players.  Find the tracks you want, when you want them.

**Screenshot templates:**  Take screenshots with a custom, sleek and stylized
filename.  Only include the information that you want.

**Looped playback:** Selectively show part of video/music tracks.  Amazing,
isn't it?

**Custom metadata:**  Display custom metadata in the playlist window.  Want to
show the artist as well as the title, down to even the encoder used?  Nothing
is stopping you.


### Upcoming features

**Native filter-chain support:**  Comprehensive integration of mpv/ffmpeg's
filter interface/library, right inside your media player.

**Encoding support like VirtualDub:**  Churn out memes faster.  No need to
open a video editor when your media player can do your job for you.

**Race Inspired Cosmetic Enhancements:**  In-app custom styling support.

Suggestions welcome.


## Compiling

### Prerequisites

You need the Qt5 SDK installed and a recent edition of libmpv.  On Ubuntu you
can usually install the required libraries with the ``qtcreator``,
``qt5-default``, ``libqt5x11extras5-dev``, ``qttools5-dev-tools`` and
``libmpv-dev`` packages.  A recent edition of [libmpv] means either from git
head or at least version 0.29.0.  The mpv options for this are
``--enable-libmpv-shared`` for shared library support, and
``--enable-libarchive`` if you want to use mpc-qt as a comic book viewer.


### I don't know git, how do I do this?

First ensure you have the prerequisites as mentioned above, then open a terminal
and `cd` into your general source-code directory. If one does not exist,
`mkdir` one.

>mkdir src

>cd ~/src

Then clone this git repository using the following command:

>git clone https://github.com/cmdrkotori/mpc-qt.git

Finally, `cd` into the checked-out repository.

>cd mpc-qt

Then build with qmake+make.

>qmake

>make -j *threads*

>sudo make install

You're done!  Later on, performing a git pull from inside the source code
directory will get the latest changes.

>git pull origin master

Rebuild by following the qmake+make steps as described above.

## Compiling on Windows

While this program is meant for Unix, it is possible to compile it on Windows
with the [MSYS2 edition of Qt Creator] due to the largely cross-platform Qt
toolkit.  MSVC is not supported.

Mpc-Qt can be compiled with a libmpv linked to MSYS2's ffmpeg libraries, or by
using the prebuilt library from mpv.srsfckn.biz.  To use the prebuilt library
after cloning this repository, download libmpv from the
[mpv windows release page], and extract it somewhere.  Place the libraries for
your architechture from mpv-dev.7z (e.g. `mpv-dev.7z/64`) into `mpv-dev/lib`.
Then place the include files from mpv-dev.7z (usually at `mpv-dev.zip/include`)
into `mpv-dev/include/mpv`.  Compile with the 64bit Qt framework as usual.

[screenshot]:https://gist.githubusercontent.com/cmdrkotori/c26e75fa01341ec54b648f1ff082a71a/raw/cdc453a1b3ff74c9ef074f3cc54fb47b386d0ac4/screenshot%252020160621.png
[mpc-hc]:https://mpc-hc.org/
[libmpv]:https://github.com/mpv-player/mpv
[mwe]:https://github.com/cmdrkotori/mpc-qt/commit/9400f595
[aur]:https://aur.archlinux.org/packages/mpc-qt-git/
[ports]:https://www.freshports.org/multimedia/mpc-qt
[mpv-build]:https://github.com/mpv-player/mpv-build
[bomi]:https://github.com/xylosper/bomi
[baka]:https://github.com/u8sand/Baka-MPlayer
[mpv windows release page]:https://mpv.srsfckn.biz/
[MSYS2 edition of Qt Creator]:https://wiki.qt.io/MSYS2
