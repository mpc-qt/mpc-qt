## Media Player Classic Qute Theater

A clone of Media Player Classic reimplemented in Qt.

![screenshot]

[Media Player Classic Home Cinema][mpc-hc] (mpc-hc) is considered by many to
be the quintessential media player for the Windows desktop.  Media Player
Classic Qute Theater (mpc-qt) aims to reproduce most of the interface and
functionality of mpc-hc while using [libmpv] to play video instead of
DirectShow.


## Releases

There may be builds for Windows users on the release page.  These use time-
based versioning (e.g. 17.07 corresponds to a 2017 July).

The best version is git master, and everyone are encouraged to increase their
computer-fu by compiling from source. (see sections below.)  Compiling from
source gives you several advantages over the usual user, such the ability to
use latest and pre-release software regardless of where it comes from.  Unix
users, there could even be packages in your distro that help with this. (e.g.
[aur], [ports].)

The program is not yet feature-complete and some controls may have greyed-out
placeholder areas, such as in the options dialog.


## Features and improvements over mpc-hc

**Clone:** Nearly everything that mpc-hc does for the casual user.

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


### Possible upcoming features

These features are wishlist items and are subject to the developer's time.

**Native filter-chain support:**  Comprehensive integration of mpv/ffmpeg's
filter interface/library, right inside your media player.  This feature may
entail a lot of code.

**Encoding support like VirtualDub:**  Churn out memes faster.  No need to
open a video editor when your media player can do your job for you.  Requires
writing an encoder-orientated backend.

**Race Inspired Cosmetic Enhancements:**  In-app custom styling support.
Requires a few dropdowns/tabs and a multi-line text editor in the settings.

Suggestions welcome.


## Compiling

### Prerequisites

You need the Qt5 SDK installed and a recent edition of libmpv.  On Ubuntu you
can usually install the required libraries with the ``qtcreator``,
``qt5-default``, ``libqt5x11extras5-dev``, ``qttools5-dev-tools``, 
``qtbase5-private-dev`` and ``libmpv-dev`` packages.  You may also need to install
``git``, ``build-essential`` and ``pkg-config``.  A recent edition of [libmpv]
means either from git head or at least version 0.29.0.  The mpv options for this
are ``--enable-libmpv-shared`` for shared library support, and
``--enable-libarchive`` if you want to use mpc-qt as a comic book viewer.

### I don't know git, how do I do this?

First ensure you have the prerequisites as mentioned above, then open a terminal
and `cd` into your general source-code directory. If one does not exist,
`mkdir` one.

>mkdir src

>cd ~/src

Then clone this git repository using the following command:

>git clone https://github.com/cmdrkotori/mpc-qt-origin.git

Finally, `cd` into the checked-out repository.

>cd mpc-qt-origin

Then build with qmake+make.

>qmake

>make -j *threads*

>sudo make install

You're done!  Later on, performing a git pull from inside the source code
directory will get the latest changes.

>git pull origin master

Rebuild by following the qmake+make steps as described above.

### I have compiler/linker errors

Some distros have an ancient version of mpv in their repos.  You can install
libmpv in the following method:

Uninstall any libmpv package you may have.

>sudo apt purge libmpv-dev

Fetch the mpv-build repo.

>cd ~/src

>git clone https://github.com/mpv-player/mpv-build.git

>cd mpv-build

Select the master versions to compile.

>./use-ffmpeg-master

>./use-libass-master

>./use-mpv-master

Follow the instructions for debian and ubuntu about making a build-deps
package. (or whatever method for your distro.)

>sudo apt-get install git devscripts equivs

>rm -f mpv-build-deps\_\*\_\*.deb

>mk-build-deps -s sudo -i

Build libmpv.

>echo --enable-libmpv-shared > mpv_options

>./update

>./build -j4

>sudo ./install

>sudo ldconfig

libmpv should now be installed to `/usr/local/*`.


## Compiling on Windows

While this program is meant for Unix, it is possible to compile it on Windows
with the [MSYS2 edition of Qt Creator] due to the largely cross-platform Qt
toolkit.  MSVC is not supported.  In addition, the build process needs the
imagemagick, librsvg and inkscape packages to create the windows ico file.
Use `pacman -Ss <package description/name/etc>` to find them.

Mpc-Qt can be compiled with a libmpv linked to MSYS2's ffmpeg libraries, or by
using the prebuilt library from mpv.srsfckn.biz.  To use the prebuilt library
after cloning this repository, download libmpv from the
[mpv windows release page], and extract it somewhere.  Place the libraries for
your architechture from mpv-dev.7z (e.g. `mpv-dev.7z/64`) into `mpv-dev/lib`.
Then place the include files from mpv-dev.7z (usually at `mpv-dev.zip/include`)
into `mpv-dev/include/mpv`.  Compile with the 64bit Qt framework as usual.

Bleeding-edge git master builds that use new features not yet in a release can
usually be made with [shinchiro builds] of libmpv.  Unpack in the same manner
as above.

[screenshot]:https://gist.githubusercontent.com/cmdrkotori/f419febc57df9610b922eb06de7c0c1d/raw/189394a9454c34f9384a3deb83a1c29277057766/Screenshot_20180427_202825.png
[mpc-hc]:https://mpc-hc.org/
[libmpv]:https://github.com/mpv-player/mpv
[mwe]:https://github.com/cmdrkotori/mpc-qt/commit/9400f595
[aur]:https://aur.archlinux.org/packages/mpc-qt-git/
[ports]:https://www.freshports.org/multimedia/mpc-qt
[mpv-build]:https://github.com/mpv-player/mpv-build
[bomi]:https://github.com/xylosper/bomi
[baka]:https://github.com/u8sand/Baka-MPlayer
[mpv windows release page]:https://mpv.srsfckn.biz/
[shinchiro builds]:https://sourceforge.net/projects/mpv-player-windows/files/libmpv/
[MSYS2 edition of Qt Creator]:https://wiki.qt.io/MSYS2
