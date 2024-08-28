#-------------------------------------------------
#
# Project created by QtCreator 2015-04-12T18:21:51
#
#-------------------------------------------------

QT       += core gui network widgets openglwidgets

QMAKE_CXXFLAGS += -Wall

TARGET = mpc-qt
TEMPLATE = app

CONFIG += c++14

unix {
    DESTDIR=bin
    OBJECTS_DIR=.obj
    MOC_DIR=.moc
    UI_DIR=.ui
}

!isEmpty(MPCQT_VERSION) {
    message("Version provided on the commandline: $$MPCQT_VERSION")
    VERSTR = "$$MPCQT_VERSION"
    VERSTR_WIN = "$$VERSTR"".0"
} else:exists(.git) {
    # bare qmake, likely invoked by myself in qtcreator or by brave users
    message("Deducing version from git information...")
    VERSTR = $$system(git describe --tags --abbrev=0)
    VERSTR_BLD = $$system(git rev-list $${VERSTR}..HEAD --count)
    message("     ...the last tag is $$VERSTR"".")
    message("     ...there are $$VERSTR_BLD commits since the last tag.")
    VERSTR = $$replace(VERSTR,'v','')
    VERSTR_WIN = "$$VERSTR"".""$$VERSTR_BLD"
    !equals(VERSTR_BLD,"0") {
        # use expanded version when past the last tag
        message("     ...this is probably a beta build, using descriptive version information.")
        VERSTR=$$system(git describe --tags)
        VERSTR=$$replace(VERSTR,v,'')
        DEFINES += MPCQT_DEVELOPMENT
    }
}
!defined(VERSTR,var) {
    message("Not in a git repo and no version information provided.")
    message("This will appear as an unversioned development build.")
    message("To pass a version to qmake, run it like this:")
    message("    qmake \"MPCQT_VERSION=\$VERSION\" mpc-qt.pro")
    VERSTR = "Unspecified_version"
    VERSTR_WIN = 0.0.0.0
}
VERSTR_DECLARE = MPCQT_VERSION_STR=\\\"$${VERSTR}\\\"
DEFINES += ""$${VERSTR_DECLARE}""

message("The version appears to be $$VERSTR"", and the manifest would say this is $$VERSTR_WIN"".")

CONFIG(release,debug|release) {
    win32:QMAKE_TARGET_COMPANY="The Mpc-Qt developers"
    win32:QMAKE_TARGET_COPYRIGHT="Copyright (c) 2015 The Mpc-Qt developers"
    win32:QMAKE_TARGET_PRODUCT="Media Player Classic Qute Theater"
    win32:QMAKE_TARGET_DESCRIPTION="MPC-QT"
    VERSION = $$VERSTR_WIN
}

unix:QT += dbus
unix:LIBS += $$QMAKE_LIBS_DYNLOAD

unix:CONFIG += link_pkgconfig
unix:PKGCONFIG += mpv

win32:LIBS += -lpowrprof
!isEmpty(ENABLE_LOCAL_MPV) {
win32:LIBS += -L$$PWD/mpv-dev/lib/ -llibmpv
win32:INCLUDEPATH += $$PWD/mpv-dev/include
win32:DEPENDPATH += $$PWD/mpv-dev
} else {
win32:CONFIG += link_pkgconfig
win32:PKGCONFIG += mpv
}

CONFIG += lrelease embed_translations
#NOTE: lupdate is run with "-locations none -no-ui-lines".  Edit your config.
LRELEASE_DIR=.
QM_FILES_RESOURCE_PREFIX=/i18n/
TRANSLATIONS += translations/mpc-qt_en.ts \
		translations/mpc-qt_es.ts \
                translations/mpc-qt_fi.ts \
                translations/mpc-qt_it.ts \
                translations/mpc-qt_ru.ts \
                translations/mpc-qt_zh_CN.ts

LCONVERT_LANGS=es fi it ru zh_CN
include(lconvert.pri)

unix {
    isEmpty(PREFIX) {
        PREFIX=/usr/local
    }
    DEFINES += MPCQT_PREFIX=\\\"$$PREFIX\\\"

    target.path = $$PREFIX/bin

    docs.files = DOCS/ipc.md
    docs.path = $$PREFIX/share/doc/mpc-qt/

    shortcut.files = mpc-qt.desktop
    shortcut.path = $$PREFIX/share/applications/

    logo.files = images/icon/mpc-qt.svg
    logo.path = $$PREFIX/share/icons/hicolor/scalable/apps/

    INSTALLS += target docs shortcut logo
}

unix:SOURCES += platform/screensaver_unix.cpp \
                platform/devicemanager_unix.cpp \
                ipc/mpris.cpp
unix:HEADERS += platform/screensaver_unix.h \
                platform/devicemanager_unix.h \
                ipc/mpris.h

win32:RC_ICONS = $$system( bash make-win-icon.sh )
win32:SOURCES += platform/screensaver_win.cpp \
                 platform/devicemanager_win.cpp
win32:HEADERS += platform/screensaver_win.h \
                 platform/devicemanager_win.h

SOURCES += main.cpp\
    librarywindow.cpp \
    mpvwidget.cpp \
    mainwindow.cpp \
    platform/windowmanager.cpp \
    playlist.cpp \
    manager.cpp \
    helpers.cpp \
    playlistwindow.cpp \
    storage.cpp \
    settingswindow.cpp \
    ipc/http.cpp \
    ipc/json.cpp \
    openfiledialog.cpp \
    propertieswindow.cpp \
    platform/unify.cpp \
    widgets/drawncollection.cpp \
    widgets/logowidget.cpp \
    widgets/paletteeditor.cpp \
    favoriteswindow.cpp \
    widgets/actioneditor.cpp \
    widgets/drawnplaylist.cpp \
    widgets/drawnslider.cpp \
    widgets/drawnstatus.cpp \
    platform/screensaver.cpp \
    platform/devicemanager.cpp \
    logwindow.cpp \
    logger.cpp \
    thumbnailerwindow.cpp \
    widgets/screencombo.cpp

HEADERS  += \
    librarywindow.h \
    mpvwidget.h \
    mainwindow.h \
    platform/windowmanager.h \
    playlist.h \
    manager.h \
    main.h \
    helpers.h \
    playlistwindow.h \
    storage.h \
    settingswindow.h \
    ipc/http.h \
    ipc/json.h \
    openfiledialog.h \
    propertieswindow.h \
    platform/unify.h \
    widgets/drawncollection.h \
    widgets/logowidget.h \
    widgets/paletteeditor.h \
    favoriteswindow.h \
    widgets/actioneditor.h \
    widgets/drawnplaylist.h \
    widgets/drawnslider.h \
    widgets/drawnstatus.h \
    platform/screensaver.h \
    platform/devicemanager.h \
    logwindow.h \
    logger.h \
    thumbnailerwindow.h \
    widgets/screencombo.h

FORMS    += \
    librarywindow.ui \
    mainwindow.ui \
    playlistwindow.ui \
    settingswindow.ui \
    openfiledialog.ui \
    propertieswindow.ui \
    favoriteswindow.ui \
    logwindow.ui \
    thumbnailerwindow.ui

RESOURCES += \
    res.qrc

OTHER_FILES += \
    LICENSE \
    README.md \
    .github/workflows/linux.yml \
    .github/workflows/windows-msys2.yml \
    make-win-icon.sh \
    make-release-msys2.sh \
    make-release-win.sh \
    DOCS/codebase2.svg \
    DOCS/codebase.svg \
    'DOCS/coding standards.md' \
    lconvert.pri

DISTFILES += \
    DOCS/ipc.md \
    mpc-qt.desktop


