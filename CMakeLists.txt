cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

project(mpc-qt CXX)
# set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -O2)

if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    add_compile_definitions(BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For clangd

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin CACHE PATH "CMAKE_RUNTIME_OUTPUT_DIRECTORY")

find_program(CCACHE_FOUND "ccache")
set(CCACHE_SUPPORT OFF CACHE BOOL "Enable ccache support")
if (CCACHE_FOUND AND CCACHE_SUPPORT)
    message(STATUS "Using ccache to speed up incremental builds")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" # GNU is GNU GCC
        OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        # without this compiler messages in `make` backend would be uncolored
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fdiagnostics-color=auto")
    endif()
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "ccache")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "ccache")
else()
    message(STATUS "ccache not found or disabled — incremental builds will be slower")
endif()

find_package(Qt6 REQUIRED COMPONENTS Gui LinguistTools Network OpenGLWidgets Svg Widgets)
find_package(PkgConfig REQUIRED)
find_package(Boost CONFIG QUIET)
if (Boost_FOUND)
    message(STATUS "Boost: FOUND (${Boost_VERSION})")
else()
    message(STATUS "Boost: NOT FOUND")
endif()
# These two SET CMAKE_AUTOMOC and CMAKE_AUTOUIC can be replaced with Qt 6.3 by
# qt_standard_project_setup()
# Starting with Qt 6.7,
# qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES ar ca de en es fi fr id it ja nb_NO nl pt_BR ru ta tr zh_CN)
# can be used instead so that the TS_FILES don't need to be listed below
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Allow overriding version from the command line
if(DEFINED MPCQT_VERSION)
    message(STATUS "Version provided on the command line: ${MPCQT_VERSION}")
    set(VERSTR "${MPCQT_VERSION}")
    set(VERSTR_WIN "${VERSTR}.0")
    set(FILEFLAGS_WIN "0x0L")
else()
    add_compile_definitions(MPCQT_DEVELOPMENT)
    # Check if we are in a git repository
    find_package(Git QUIET)
    if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
        message(STATUS "Deducing version from git information...")

        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
            OUTPUT_VARIABLE VERSTR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-list ${VERSTR}..HEAD --count
            OUTPUT_VARIABLE VERSTR_BLD
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        message(STATUS "     ...the last tag is ${VERSTR}.")
        message(STATUS "     ...there are ${VERSTR_BLD} commits since the last tag.")

        if(NOT "${VERSTR}" STREQUAL "")
            string(REPLACE "v" "" VERSTR ${VERSTR})
            set(VERSTR_WIN "${VERSTR}.${VERSTR_BLD}")
	    string(REPLACE "-master" "" VERSTR_WIN ${VERSTR_WIN})
        endif()

        if(NOT "${VERSTR_BLD}" STREQUAL "0")
            message(STATUS "     ...this is probably a beta build, using descriptive version information.")
            execute_process(
                COMMAND ${GIT_EXECUTABLE} describe --tags
                OUTPUT_VARIABLE VERSTR
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            string(REPLACE "v" "" VERSTR ${VERSTR})
        endif()
    else()
        message(STATUS "Not in a git repo and no version information provided.")
        message(STATUS "This will appear as an unversioned development build.")
        message(STATUS "To pass a version to CMake, run it like this:")
        message(STATUS "    cmake -DMPCQT_VERSION=<version> ..")
        set(VERSTR "Unspecified_version")
        set(VERSTR_WIN "0.0.0.0")
    endif()
endif()

# Generate version.h with version information
configure_file(
    ${CMAKE_SOURCE_DIR}/version.h.in
    ${CMAKE_BINARY_DIR}/version.h
)

message(STATUS "The version appears to be ${VERSTR}, and the Windows manifest would say this is ${VERSTR_WIN}.")

set(SOURCES
    favoriteswindow.cpp favoriteswindow.h favoriteswindow.ui
    gotowindow.cpp gotowindow.h gotowindow.ui
    helpers.cpp helpers.h
    ipc/http.cpp ipc/http.h
    ipc/json.cpp ipc/json.h
    librarywindow.cpp librarywindow.h librarywindow.ui
    logger.cpp logger.h
    logwindow.cpp logwindow.h logwindow.ui
    main.cpp main.h
    mainwindow.cpp mainwindow.h mainwindow.ui
    manager.cpp manager.h
    mpvwidget.cpp mpvwidget.h
    openfiledialog.cpp openfiledialog.h openfiledialog.ui
    platform/devicemanager.cpp platform/devicemanager.h
    platform/screensaver.cpp platform/screensaver.h
    platform/unify.cpp platform/unify.h
    platform/windowmanager.cpp platform/windowmanager.h
    playlist.cpp playlist.h
    playlistwindow.cpp playlistwindow.h playlistwindow.ui
    propertieswindow.cpp propertieswindow.h propertieswindow.ui
    settingswindow.cpp settingswindow.h settingswindow.ui
    storage.cpp storage.h
    thumbnailerwindow.cpp thumbnailerwindow.h thumbnailerwindow.ui
    widgets/actioneditor.cpp widgets/actioneditor.h
    widgets/drawncollection.cpp widgets/drawncollection.h
    widgets/drawnplaylist.cpp widgets/drawnplaylist.h
    widgets/drawnslider.cpp widgets/drawnslider.h
    widgets/drawnstatus.cpp widgets/drawnstatus.h
    widgets/logowidget.cpp widgets/logowidget.h
    widgets/paletteeditor.cpp widgets/paletteeditor.h
    widgets/screencombo.cpp widgets/screencombo.h
    widgets/tooltip.cpp widgets/tooltip.h
    widgets/videopreview.cpp widgets/videopreview.h
)

qt_add_executable(mpc-qt WIN32 MACOSX_BUNDLE ${SOURCES})

target_include_directories(mpc-qt PRIVATE ${CMAKE_BINARY_DIR})
if (Boost_FOUND)
    target_include_directories(mpc-qt PRIVATE ${Boost_INCLUDE_DIRS})
endif()

set(TS_FILES
    translations/mpc-qt_ar.ts
    translations/mpc-qt_ca.ts
    translations/mpc-qt_de.ts
    translations/mpc-qt_en.ts
    translations/mpc-qt_es.ts
    translations/mpc-qt_fi.ts
    translations/mpc-qt_fr.ts
    translations/mpc-qt_id.ts
    translations/mpc-qt_it.ts
    translations/mpc-qt_ja.ts
    translations/mpc-qt_nb_NO.ts
    translations/mpc-qt_nl.ts
    translations/mpc-qt_pt_BR.ts
    translations/mpc-qt_ru.ts
    translations/mpc-qt_ta.ts
    translations/mpc-qt_tr.ts
    translations/mpc-qt_zh_CN.ts
)

if(Qt6_VERSION VERSION_GREATER_EQUAL 6.7.0)
    # Starting with Qt 6.7, this can be used to exclude some files from lupdate
    set_property(TARGET mpc-qt PROPERTY QT_EXCLUDE_SOURCES_FROM_TRANSLATION
        images/*
    )
endif()

# Generate QM files
# lupdate can be called with cmake --build build --target update_translations
qt_add_translations(mpc-qt
    TS_FILES ${TS_FILES}
    RESOURCE_PREFIX "/i18n/"
    TS_FILE_DIR translations
    LUPDATE_OPTIONS -locations none -no-ui-lines
)

if(WIN32) # Add the Qt interface translations on Windows
    file(GLOB QT_QM_FILES "${PROJECT_SOURCE_DIR}/translations/qt/qtbase_*.qm")

    qt_add_resources(mpc-qt "qt_translations"
        PREFIX "/i18n/"
        BASE "${PROJECT_SOURCE_DIR}/translations/qt"
        FILES ${QT_QM_FILES}
    )
endif()

target_link_libraries(mpc-qt PRIVATE
    Qt::Gui
    Qt::Network
    Qt::OpenGLWidgets
    Qt::Svg
    Qt::Widgets
)

if(UNIX)
    target_sources(mpc-qt PUBLIC
        ipc/mpris.cpp ipc/mpris.h
        platform/devicemanager_unix.cpp platform/devicemanager_unix.h
        platform/screensaver_unix.cpp platform/screensaver_unix.h
    )
    find_package(Qt6 REQUIRED COMPONENTS DBus)
    target_link_libraries(mpc-qt PRIVATE Qt::DBus)
    if (Boost_FOUND)
        add_compile_definitions(BOOST_STACKTRACE_USE_ADDR2LINE)
    endif()
endif()

if(WIN32)
    target_sources(mpc-qt PUBLIC
        platform/devicemanager_win.cpp platform/devicemanager_win.h
        platform/screensaver_win.cpp platform/screensaver_win.h
    )
    target_link_libraries(mpc-qt PRIVATE powrprof)

    # Convert version string to comma-separated format for RC file
    string(REPLACE "." "," VERSTR_WIN_COMMA "${VERSTR_WIN}")

    # Configure the RC file with version information
    configure_file(
        ${CMAKE_SOURCE_DIR}/platform/mpc-qt.rc.in
        ${CMAKE_BINARY_DIR}/mpc-qt.rc
    )

    # Add the resource file to the target
    target_sources(mpc-qt PRIVATE ${CMAKE_BINARY_DIR}/mpc-qt.rc)
endif()

if(UNIX)
    # Use built version of MPV for the AppImage
    find_library(MPV_LIBRARY mpv)
    if (MPV_LIBRARY)
        target_link_libraries(mpc-qt PRIVATE ${MPV_LIBRARY})
    else()
        # Use pkg-config to find MPV
        pkg_check_modules(MPV REQUIRED mpv)

        target_include_directories(mpc-qt PRIVATE ${MPV_INCLUDE_DIRS})
        target_link_libraries(mpc-qt PRIVATE ${MPV_LIBRARIES})
        add_compile_definitions(${MPV_CFLAGS_OTHER})
    endif()
endif()

if(WIN32)
    option(ENABLE_LOCAL_MPV "Use local mpv" OFF)
    if(ENABLE_LOCAL_MPV)
        target_include_directories(mpc-qt PRIVATE ${CMAKE_SOURCE_DIR}/mpv-dev/include)
        target_link_directories(mpc-qt PRIVATE ${CMAKE_SOURCE_DIR}/mpv-dev/lib)
        target_link_libraries(mpc-qt PRIVATE mpv)
    else()
        pkg_check_modules(MPV REQUIRED mpv)

        target_include_directories(mpc-qt PRIVATE ${MPV_INCLUDE_DIRS})
        target_link_libraries(mpc-qt PRIVATE ${MPV_LIBRARIES})
        add_compile_definitions(${MPV_CFLAGS_OTHER})
    endif()
endif()


set(res_resource_files
    http/browser.html
    http/index.html
    http/info.html
    http/variables.html
    images/icon/mpc-qt.svg
    images/icon/tinyicon.svg
    images/logo/cinema-screen.svg
    images/logo/mpv-vlc.svg
    images/logo/triangle-circle.svg
    images/theme/black/document-export.svg
    images/theme/black/document-import.svg
    images/theme/black/go-next.svg
    images/theme/black/go-previous.svg
    images/theme/black/media-playback-pause.svg
    images/theme/black/media-playback-start.svg
    images/theme/black/media-playback-stop.svg
    images/theme/black/media-queue-visible.svg
    images/theme/black/media-seek-backward.svg
    images/theme/black/media-seek-forward.svg
    images/theme/black/media-skip-backward.svg
    images/theme/black/media-skip-forward.svg
    images/theme/black/player-volume-muted.svg
    images/theme/black/player-volume.svg
    images/theme/black/tab-close.svg
    images/theme/black/tab-duplicate.svg
    images/theme/black/tab-new.svg
    images/theme/black/view-media-queue.svg
    images/theme/black/view-media-subtitles-hidden.svg
    images/theme/black/view-media-subtitles.svg
    images/theme/black/zone-in.svg
    images/theme/black/zone-out.svg
    images/theme/white/document-export.svg
    images/theme/white/document-import.svg
    images/theme/white/go-next.svg
    images/theme/white/go-previous.svg
    images/theme/white/media-playback-pause.svg
    images/theme/white/media-playback-start.svg
    images/theme/white/media-playback-stop.svg
    images/theme/white/media-queue-visible.svg
    images/theme/white/media-seek-backward.svg
    images/theme/white/media-seek-forward.svg
    images/theme/white/media-skip-backward.svg
    images/theme/white/media-skip-forward.svg
    images/theme/white/player-volume-muted.svg
    images/theme/white/player-volume.svg
    images/theme/white/tab-close.svg
    images/theme/white/tab-duplicate.svg
    images/theme/white/tab-new.svg
    images/theme/white/video-x-generic-16.svg
    images/theme/white/video-x-generic-32.svg
    images/theme/white/view-media-queue.svg
    images/theme/white/view-media-subtitles-hidden.svg
    images/theme/white/view-media-subtitles.svg
    images/theme/white/zone-in.svg
    images/theme/white/zone-out.svg
    text/encodeFormat.html
    text/playlistFormat.html
)

qt_add_resources(mpc-qt "res"
    PREFIX
        "/"
    FILES
        ${res_resource_files}
)

if(UNIX)
    install(TARGETS mpc-qt DESTINATION bin)
    install(FILES DOCS/ipc.md DESTINATION share/doc/mpc-qt)
    install(FILES data/io.github.mpc_qt.mpc-qt.desktop DESTINATION share/applications)
    install(FILES data/io.github.mpc_qt.mpc-qt.metainfo.xml DESTINATION share/metainfo)
    install(FILES images/icon/mpc-qt.svg DESTINATION share/icons/hicolor/scalable/apps)

    add_custom_target(uninstall COMMAND xargs rm -vf < install_manifest.txt)
endif()
