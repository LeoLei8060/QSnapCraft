cmake_minimum_required(VERSION 3.16)
project(QSnapCraft VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

add_subdirectory(src/hooks)

set(PROJECT_SOURCES
    src/main.cpp
    src/core/screenshot/screenshot_window.h
    src/core/screenshot/screenshot_window.cpp
    src/core/screenshot/magnifier.h
    src/core/screenshot/magnifier.cpp
    src/core/editor/editor_window.h
    src/core/editor/editor_window.cpp
    src/core/pinboard/pin_window.h
    src/core/pinboard/pin_window.cpp
    src/utils/system_tray.h
    src/utils/system_tray.cpp
    src/utils/shortcut_manager.h
    src/utils/shortcut_manager.cpp
    src/utils/uiinspector.h
    src/utils/uiinspector.cpp
    src/utils/SingleApplication.h
    src/utils/SingleApplication.cpp
    src/utils/mousehook.h
    src/utils/mousehook.cpp
    resources/resources.qrc
)

add_executable(QSnapCraft
    ${PROJECT_SOURCES}
)

target_link_libraries(QSnapCraft PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    hooks
)

target_include_directories(QSnapCraft PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
