#import <AppKit/AppKit.h>
#include <QCursor>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QWidget>
#include <QWindow>
#include "fullscreen_mac.h"

static const NSApplicationPresentationOptions hiddenChrome =
        NSApplicationPresentationAutoHideMenuBar | NSApplicationPresentationAutoHideDock;
// Above the menu bar, so that menu bar tools drawing their own bar are covered too
static const NSWindowLevel coverLevel = NSStatusWindowLevel + 1;

static NSWindowStyleMask savedStyleMask = 0;
static bool savedRespectsSafeArea = true;
static id becameKeyObserver = nil;
static id resignedKeyObserver = nil;
static id mouseMoveMonitor = nil;

static NSWindow *nativeWindow(QWidget *window)
{
    return reinterpret_cast<NSView *>(window->window()->winId()).window;
}

void MacFullscreen::enter(QWidget *window)
{
    NSWindow *nsWindow = nativeWindow(window);
    savedStyleMask = nsWindow.styleMask;
    NSApp.presentationOptions = NSApp.presentationOptions | hiddenChrome;
    nsWindow.styleMask = NSWindowStyleMaskBorderless;
    [nsWindow setFrame:nsWindow.screen.frame display:YES];

    // Let the video reach the top edge instead of leaving a bar beside the
    // camera housing; letterboxed video has black there anyway.
    QWidget *top = window->window();
    savedRespectsSafeArea = top->testAttribute(Qt::WA_ContentsMarginsRespectsSafeArea);
    top->setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false);

    // A window that grows under a still pointer gets its mouse moves without a
    // window until the pointer crosses a window edge, so Qt never sees them and
    // the pointer and controls stay hidden.  Pass such moves to Qt directly.
    QWindow *qtWindow = top->windowHandle();
    mouseMoveMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskMouseMoved
                                                             handler:^NSEvent *(NSEvent *event) {
        NSInteger under = [NSWindow windowNumberAtPoint:NSEvent.mouseLocation
                                   belowWindowWithWindowNumber:0];
        if (event.window == nsWindow || under != nsWindow.windowNumber)
            return event;
        // The pointer on the far screen edges maps one pixel outside the window.
        QRect frame = qtWindow->geometry();
        QPointF global = QCursor::pos();
        global = QPointF(qBound(qreal(frame.left()), global.x(), qreal(frame.right())),
                         qBound(qreal(frame.top()), global.y(), qreal(frame.bottom())));
        QMouseEvent move(QEvent::MouseMove, qtWindow->mapFromGlobal(global), global,
                         Qt::NoButton, QGuiApplication::mouseButtons(),
                         QGuiApplication::keyboardModifiers());
        QCoreApplication::sendEvent(qtWindow, &move);
        return event;
    }];

    // Step down while a dialog or another application is in use, or it would
    // open behind the video.
    NSNotificationCenter *center = NSNotificationCenter.defaultCenter;
    becameKeyObserver = [center addObserverForName:NSWindowDidBecomeKeyNotification
                                            object:nsWindow queue:nil
                                        usingBlock:^(NSNotification *) {
        nsWindow.level = coverLevel;
    }];
    resignedKeyObserver = [center addObserverForName:NSWindowDidResignKeyNotification
                                              object:nsWindow queue:nil
                                          usingBlock:^(NSNotification *) {
        nsWindow.level = NSNormalWindowLevel;
    }];
    if (nsWindow.keyWindow)
        nsWindow.level = coverLevel;
}

void MacFullscreen::leave(QWidget *window)
{
    NSWindow *nsWindow = nativeWindow(window);
    [NSEvent removeMonitor:mouseMoveMonitor];
    mouseMoveMonitor = nil;
    [NSNotificationCenter.defaultCenter removeObserver:becameKeyObserver];
    [NSNotificationCenter.defaultCenter removeObserver:resignedKeyObserver];
    becameKeyObserver = resignedKeyObserver = nil;
    window->window()->setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, savedRespectsSafeArea);
    nsWindow.level = NSNormalWindowLevel;
    nsWindow.styleMask = savedStyleMask;
    NSApp.presentationOptions = NSApp.presentationOptions & ~hiddenChrome;
}
