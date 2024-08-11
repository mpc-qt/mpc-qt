## Media Player Classic Qute Theater

A clone of Media Player Classic reimplemented in Qt.

![screenshot]

[Media Player Classic Home Cinema][mpc-hc] (mpc-hc) is considered by many to
be the quintessential media player for the Windows desktop.  Media Player
Classic Qute Theater (mpc-qt) aims to reproduce most of the interface and
functionality of mpc-hc while using [libmpv] to play video instead of
DirectShow.


## Releases

There are builds for Windows users on the release page; these use time-based
versioning (e.g. 17.07 corresponds to a 2017 July).  There are also build
artifacts from Github actions.

Note that unimplemented features in the options dialog are hidden and (where
this is not possible) disabled.  This is to preserve the familiar options layout
from mpc-hc in the meantime.


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

**Custom metadata:**  Display custom metadata in the playlist window.  Want to
show the artist as well as the title, down to even the encoder used?  Nothing
is stopping you.

**Race Inspired Cosmetic Enhancements:**  In-app custom styling support.  Make
the app go faster!

### Possible upcoming features

These features are wishlist items and are subject to the developer's time.

**Encoding support like VirtualDub:**  Churn out clips faster.  No need to
open a video editor when your media player can do your job for you.  Requires
writing an encoder-orientated backend.

Suggestions welcome.

## Contributing

If you are a coder, try to follow the conventions in the code and make a PR.
If you are a translator, we will be using [Weblate] to translate.
Alternatively, you can submit the translations as a PR.

## Compiling

The best version is git master, and everyone is encouraged to increase their
computer-fu by compiling from source (see sections below).  Compiling from
source gives you several advantages over the usual user, such as the ability to
use latest and pre-release software regardless of where it comes from.  Unix
users, there could even be packages in your distro that help with this (e.g.
[aur], [ports]).

### Prerequisites

You need the Qt6 SDK installed and a recent edition of libmpv.  On Ubuntu you
can usually install the required libraries and tools with the
`build-essential`, `qmake6`, `qt6-base-dev`, `qt6-l10n-tools`, `libqt6svg6-dev`,
and `libmpv-dev` packages.  A recent edition of [libmpv] means either from git
head or at least version 0.37.0.

### I don't know git, how do I do this?

First ensure you have the prerequisites as mentioned above, then open a terminal
and `cd` into your general source-code directory. If one does not exist,
`mkdir` one.

>mkdir src

>cd ~/src

Then clone this git repository using the following command:

>git clone https://github.com/mpc-qt/mpc-qt.git

Finally, `cd` into the checked-out repository.

>cd mpc-qt

Then build with qmake+make.

>qmake6

>make -j\`nproc\`

>sudo make install

You're done!  Later on, performing a git pull from inside the source code
directory will get the latest changes.

>git pull origin master

Rebuild by following the qmake+make steps as described above.

### I have compiler/linker errors

Some distros have an ancient version of mpv in their repos.  You can install
libmpv by following the instructions at the [mpv-build repo].

## Compiling on Windows

While this program is meant for Unix, it is possible to compile it on Windows
with MSYS2.  MSVC is not supported due to the nature of the build process, but
may work in theory if you throw enough stubbornness at it.

From a fresh installation of MSYS2, run the following from an MSYS2 MINGW64
prompt.  First, update the packages

>pacman -Syu

Restart MSYS2 MINGW64 when prompted to do so and re-run pacman.

>pacman -Syu

At this stage packages required for building can be installed (Copy and paste
this as one line).

>pacman -S base-devel git mingw-w64-x86_64-toolchain mingw-w64-x86_64-clang
>mingw-w64-x86_64-cmake mingw-w64-x86_64-qt6 mingw-w64-x86_64-qt-creator
>mingw-w64-x86_64-qt6-doc mingw-w64-x86_64-imagemagick mingw-w64-x86_64-librsvg
>mingw-w64-x86_64-inkscape

At the time of writing, there is a slight bug in qmake where it looks for the wrong
version of lupdate/lrelease.  Create the required symlinks.

>ln -s lrelease-qt6.exe /mingw64/bin/lrelease.exe

>ln -s lupdate-qt6.exe /mingw64/bin/lupdate.exe

Mpc-Qt can be compiled with a libmpv linked to MSYS2's ffmpeg libraries, or by
using the prebuilt library released on sourceforge.  To use the prebuilt
library after cloning this repository, download libmpv from [shinchiro's
release page], and extract it somewhere.  Place the files in the root folder
of mpv-dev-x86_64-*.7z into `mpv-dev/lib`. Then place the files in its include
folder into `mpv-dev/include/mpv`.  If you do this, compile with the
`ENABLE_LOCAL_MPV=1` option.

>qmake6 ENABLE_LOCAL_MPV=1 mpc-qt.pro

>make -j\`nproc\`

Congratulations, you have now built mpc-qt!

[screenshot]:https://raw.githubusercontent.com/mpc-qt/mpc-qt-screenshots/master/Screenshot_20220226_155532.png
[mpc-hc]:https://mpc-hc.org/
[libmpv]:https://github.com/mpv-player/mpv
[weblate]:https://weblate.org/en/
[aur]:https://aur.archlinux.org/packages/mpc-qt-git/
[ports]:https://www.freshports.org/multimedia/mpc-qt
[mpv-build repo]:https://github.com/mpv-player/mpv-build
[shinchiro's release page]:https://sourceforge.net/projects/mpv-player-windows/files/libmpv/
[MSYS2 edition of Qt Creator]:https://wiki.qt.io/MSYS2
