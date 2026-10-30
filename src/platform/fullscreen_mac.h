#ifndef FULLSCREEN_MAC_H
#define FULLSCREEN_MAC_H

class QWidget;

// Full screen without a separate space, as on the other platforms: a borderless
// window over the whole screen with the menu bar and Dock hidden.  A full screen
// space keeps the menu bar when the user has asked macOS to show it there.
namespace MacFullscreen {
    void enter(QWidget *window);
    void leave(QWidget *window);
}

#endif // FULLSCREEN_MAC_H
