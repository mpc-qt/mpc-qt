#-------------------------------------------------
#
# Project created by QtCreator 2015-04-12T18:21:51
#
#-------------------------------------------------

QT       += core gui network widgets

QMAKE_CXXFLAGS += -Wall -Werror

!win32 {
QT += x11extras
}

TARGET = mpc-qt
TEMPLATE = app

CONFIG += c++11

!win32 {
CONFIG += link_pkgconfig
PKGCONFIG += mpv
}

win32 {
LIBS += -L$$PWD/mpv-dev/lib/ -llibmpv
INCLUDEPATH += $$PWD/mpv-dev/include
DEPENDPATH += $$PWD/mpv-dev
}

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
    ipc.cpp

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
    ipc.h

FORMS    += \
    mainwindow.ui \
    playlistwindow.ui \
    settingswindow.ui

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
