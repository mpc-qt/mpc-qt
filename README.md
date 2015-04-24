## Media Player Classic Qute Theater

A clone of Media Player Classic reimplimented in Qt.

[Media Player Classic Home Cinema] is considered by many to be the
quintessential media player for the Windows desktop.  I would like mpc-qt to
reproduce most of the interface and functionality of mpc-hc while using
[libmpv] to play video instead of DirectShow.

## Features
This is pre-alpha software and most of the functionality isn't written yet,
however, you can still
* play back simple video files
* control playback using the control buttons
* seek with the seek bar
* resize the window automatically using zoom factors

As time goes on, features will be implemented, added or dropped given the
use-case of mpc-qt.  For example, there *are* a few things which will likely
never be implemented, such as the silly DirectShow output filter chains that
libmpv simply has no need for.  On the other hand, libmpv also provides nice
features that are not present in mpc-hc and these functionalities may be
exposed as the developer sees fit.

## I don't know git, how do I run this?
You'll have to perform a little bit of footwork beforehand.

* `cd` into your general source-code directory. If one does not exist, `mkdir` one.
```
tux@oniichan ~> mkdir src
tux@oniichan ~> cd ~/src
```
* Make a directory to checkout this git into.
```
tux@oniichan ~/src> mkdir mpc-qt
```
* `cd` into it.
```
tux@oniichan ~/src> cd mpc-qt
```
* Checkout this git repository.
```
tux@oniichan ~/s/mpc-qt> git checkout https://github.com/cmdrkotori/mpc-qt.git
```
* `cd` into the checked-out repository and run qtcreator.
```
tux@oniichan ~/s/mpc-qt> cd mpc-qt
tux@oniichan ~/s/m/mpc-qt> qtcreator mpc-qt.pro
```
* Use qtcreator's suggested build setup and press the big arrow button in the bottom-left corner.
* After this, the executable should be in `~/src/mpc-qt/build-mpc-qt...`. Look for the `mpc-qt` binary and run that whenever you want to, or stick a shortcut to that on your taskbar/desktop.

[Media Player Classic Home Cinema]:https://mpc-hc.org/
[libmpv]:https://github.com/mpv-player/mpv
[bomi]:https://github.com/xylosper/bomi
[baka]:https://github.com/u8sand/Baka-MPlayer
