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

!isEmpty(MPCQT_VERSION) {
    # version provided on qmake commandline.
    # e.g. distro makers, make-release-win script
    VERSTR = $$MPCQT_VERSION
    VERSTR_WIN = $${MPCQT_VERSION}.0
} else:exists(.git) {
    # bare qmake, likely by myself or brave users
    VERSTR = $$system(git describe --tags --abbrev=0)
    VERSTR_BLD = $$system(git rev-list $${VERSTR}..HEAD --count)
    VERSTR = $$replace(VERSTR,'v','')
    VERSTR_WIN = $$VERSTR.$$VERSTR_BLD
    # use expanded version when past the last tag
    !isEqual($$VERSTR_BLD,0) {
        VERSTR=$$system(git describe --tags)
        VERSTR=$$replace(VERSTR,v,'')
        DEFINES += MPCQT_DEVELOPMENT
    }
}
!isEmpty(VERSTR) {
    VERSTR_DECLARE = MPCQT_VERSION_STR=\\\"$${VERSTR}\\\"
    DEFINES += ""$${VERSTR_DECLARE}""
} else {
    # not in a git repo and no parsable version given.
    # this is most probably not a disaster, since the
    # version string may have been provided on the
    # command line, and we have a sane fallback.
    VERSTR_WIN = 0.0.0.0
}

CONFIG(release,debug|release) {
    win32:QMAKE_TARGET_COMPANY="The Mpc-Qt developers"
    win32:QMAKE_TARGET_COPYRIGHT="Copyright (c) 2015 The Mpc-Qt developers"
    win32:QMAKE_TARGET_PRODUCT="Media Player Classic Qute Theater"
    win32:QMAKE_TARGET_DESCRIPTION="MPC-QT"
    VERSION = $$VERSTR_WIN
}

unix:!macx:QT += x11extras dbus gui-private
unix:!macx:LIBS += $$QMAKE_LIBS_DYNLOAD

!win32:CONFIG += link_pkgconfig
!win32:PKGCONFIG += mpv

win32:LIBS += -L$$PWD/mpv-dev/lib/ -llibmpv -lpowrprof
win32:INCLUDEPATH += $$PWD/mpv-dev/include
win32:DEPENDPATH += $$PWD/mpv-dev

TRANSLATIONS += translations/mpc-qt_it.ts\
                            translations/mpc-qt_ru.ts

isEmpty(QMAKE_LUPDATE) {
    win32:QMAKE_LUPDATE = $$[QT_INSTALL_BINS]\\lupdate.exe
    else:QMAKE_LUPDATE = $$[QT_INSTALL_BINS]/lupdate
    unix {
        !exists($$QMAKE_LUPDATE) { QMAKE_LUPDATE = lupdate-qt5 }
    } else {
        !exists($$QMAKE_LUPDATE) { QMAKE_LUPDATE = lupdate }
    }
}

isEmpty(QMAKE_LRELEASE) {
    win32:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\\lrelease.exe
    else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
    unix {
        !exists($$QMAKE_LRELEASE) { QMAKE_LRELEASE = lrelease-qt5 }
    } else {
        !exists($$QMAKE_LRELEASE) { QMAKE_LRELEASE = lrelease }
    }
}

lupdate.input = TRANSLATIONS
lupdate.output = translations/%{QMAKE_FILE_IN}.ts
lupdate.commands = $${QMAKE_LUPDATE} -locations none -no-ui-lines $$_PRO_FILE_
lupdate.CONFIG += no_link target_predeps
lrelease.input = TRANSLATIONS
lrelease.output = resources/translations/${QMAKE_FILE_BASE}.qm
lrelease.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN} -qm resources/translations/${QMAKE_FILE_BASE}.qm
lrelease.CONFIG += no_link target_predeps
QMAKE_EXTRA_COMPILERS += lupdate lrelease

unix {
    isEmpty(PREFIX) {
        PREFIX=/usr/local
    }
    DEFINES += MPCQT_PREFIX=\\\"$$PREFIX\\\"

    target.path = $$PREFIX/bin

    docs.files = DOCS/ipc.md
    docs.path = $$PREFIX/share/doc/mpc-qt/

    translations.files = resources/translations
    translations.path = $$PREFIX/share/mpc-qt/

    shortcut.files = mpc-qt.desktop
    shortcut.path = $$PREFIX/share/applications/

    logo.files = images/icon/mpc-qt.svg
    logo.path = $$PREFIX/share/icons/hicolor/scalable/apps/

    INSTALLS += target docs shortcut logo translations
}

unix:!macx:SOURCES += platform/screensaver_unix.cpp \
                      platform/devicemanager_unix.cpp \
                      ipcmpris.cpp
unix:!macx:HEADERS += platform/screensaver_unix.h \
                      platform/devicemanager_unix.h \
                      ipcmpris.h

win32:RC_ICONS = $$system( bash make-win-icon.sh )
win32:SOURCES += platform/screensaver_win.cpp \
                 platform/devicemanager_win.cpp
win32:HEADERS += platform/screensaver_win.h \
                 platform/devicemanager_win.h

macx:SOURCES += platform/screensaver_mac.cpp \
                platform/devicemanager_mac.cpp
macx:HEADERS += platform/screensaver_mac.h \
                platform/devicemanager_mac.h

SOURCES += main.cpp\
    mpvwidget.cpp \
    mainwindow.cpp \
    playlist.cpp \
    manager.cpp \
    helpers.cpp \
    playlistwindow.cpp \
    storage.cpp \
    settingswindow.cpp \
    ipcjson.cpp \
    openfiledialog.cpp \
    propertieswindow.cpp \
    platform/unify.cpp \
    paletteeditor.cpp \
    favoriteswindow.cpp \
    actioneditor.cpp \
    drawnplaylist.cpp \
    drawnslider.cpp \
    drawnstatus.cpp \
    platform/screensaver.cpp \
    platform/devicemanager.cpp

HEADERS  += \
    mpvwidget.h \
    mainwindow.h \
    playlist.h \
    manager.h \
    main.h \
    helpers.h \
    playlistwindow.h \
    storage.h \
    settingswindow.h \
    ipcjson.h \
    openfiledialog.h \
    propertieswindow.h \
    platform/unify.h \
    paletteeditor.h \
    favoriteswindow.h \
    actioneditor.h \
    drawnplaylist.h \
    drawnslider.h \
    drawnstatus.h \
    platform/screensaver.h \
    platform/devicemanager.h

FORMS    += \
    mainwindow.ui \
    playlistwindow.ui \
    settingswindow.ui \
    openfiledialog.ui \
    propertieswindow.ui \
    favoriteswindow.ui

RESOURCES += \
    res.qrc

OTHER_FILES += \
    LICENSE \
    README.md \
    make-win-icon.sh \
    make-release-win.sh \
    DOCS/codebase2.svg \
    DOCS/codebase.svg \
    'DOCS/coding standards.md'

DISTFILES += \
    DOCS/ipc.md \
    mpc-qt.desktop

