
# Generate version.h with version information
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
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

if(Qt6_VERSION VERSION_GREATER_EQUAL 6.7.0)
    # Starting with Qt 6.7, this can be used to exclude some files from lupdate
    set_property(TARGET mpc-qt PROPERTY QT_EXCLUDE_SOURCES_FROM_TRANSLATION
        ${CMAKE_SOURCE_DIR}/res/images/*
    )
endif()

target_include_directories(mpc-qt PRIVATE ${CMAKE_BINARY_DIR})
if (Boost_FOUND)
    target_include_directories(mpc-qt PRIVATE ${Boost_INCLUDE_DIRS})
endif()

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
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/mpc-qt.rc.in
        ${CMAKE_BINARY_DIR}/mpc-qt.rc
    )

    # Add the resource file to the target
    target_sources(mpc-qt PRIVATE ${CMAKE_BINARY_DIR}/mpc-qt.rc)
endif()
