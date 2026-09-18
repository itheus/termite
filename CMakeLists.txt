cmake_minimum_required(VERSION 3.21)

project(termite
    VERSION 0.6.0
    DESCRIPTION "A terminal that lives under your KDE desktop"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.6 REQUIRED COMPONENTS Widgets Network)
find_package(KF6GlobalAccel REQUIRED)
find_package(KF6WindowSystem REQUIRED)
find_package(LayerShellQt REQUIRED)
find_package(qtermwidget6 REQUIRED)

qt_standard_project_setup()

qt_add_executable(termite
    src/main.cpp
    src/app.h
    src/app.cpp
    src/capture.h
    src/capture.cpp
    src/shortcut.h
    src/shortcut.cpp
    src/overlay.h
    src/overlay.cpp
    src/settings.h
    src/settings.cpp
    src/runtime.h
    src/runtime.cpp
    src/ipc.h
    src/ipc.cpp
    src/launchpath.h
    src/launchpath.cpp
    src/settingsui.h
    src/settingsui.cpp
    src/shortcutrecorder.h
    src/shortcutrecorder.cpp
)

qt_add_resources(termite "termite_resources"
    PREFIX "/"
    FILES assets/termite.png assets/termite-alt.png assets/icon.png
)

target_link_libraries(termite PRIVATE
    Qt6::Widgets
    Qt6::Network
    KF6::GlobalAccel
    KF6::WindowSystem
    LayerShellQt::Interface
    qtermwidget6
)

include(GNUInstallDirs)

install(TARGETS termite RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES packaging/termite.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)

install(FILES assets/icon.png
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/800x800/apps
    RENAME termite.png)