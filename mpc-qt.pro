#-------------------------------------------------
#
# Project created by QtCreator 2015-04-12T18:21:51
#
#-------------------------------------------------

QT       += core gui network widgets

QMAKE_CXXFLAGS += -Wall

TARGET = mpc-qt
TEMPLATE = app

CONFIG += c++14

unix:QT += x11extras dbus

!win32:CONFIG += link_pkgconfig
!win32:PKGCONFIG += mpv

win32:LIBS += -L$$PWD/mpv-dev/lib/ -llibmpv
win32:INCLUDEPATH += $$PWD/mpv-dev/include
win32:DEPENDPATH += $$PWD/mpv-dev

unix:SOURCES += platform/unix_qscreensaver.cpp
unix:HEADERS += platform/unix_qscreensaver.h

win32:SOURCES += platform/win_qscreensaver.cpp
win32:HEADERS += platform/win_qscreensaver.h

macx:SOURCES += platform/mac_qscreensaver.cpp
macx:HEADERS += platform/mac_qscreensaver.h

SOURCES += main.cpp\
    mpvwidget.cpp \
    mainwindow.cpp \
    qdrawnslider.cpp \
    playlist.cpp \
    qdrawnplaylist.cpp \
    manager.cpp \
    helpers.cpp \
    playlistwindow.cpp \
    storage.cpp \
    settingswindow.cpp \
    qactioneditor.cpp \
    qdrawnstatus.cpp \
    ipc.cpp \
    openfiledialog.cpp \
    platform/qabstractscreensaver.cpp \
    QScreenSaver.cpp

HEADERS  += \
    mpvwidget.h \
    mainwindow.h \
    qdrawnslider.h \
    playlist.h \
    qdrawnplaylist.h \
    manager.h \
    main.h \
    helpers.h \
    playlistwindow.h \
    storage.h \
    settingswindow.h \
    qactioneditor.h \
    qdrawnstatus.h \
    ipc.h \
    openfiledialog.h \
    platform/qabstractscreensaver.h \
    QScreenSaver.h


FORMS    += \
    mainwindow.ui \
    playlistwindow.ui \
    settingswindow.ui \
    openfiledialog.ui

RESOURCES += \
    res.qrc

OTHER_FILES += \
    LICENSE \
    README.md \
    DOCS/codebase.svg

DISTFILES += \
    generate-localinstall-desktop.py \
    DOCS/codebase2.svg \
    DOCS/ipc.md
