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
    src/core/screenshot/screenshotwindow.h
    src/core/screenshot/screenshotwindow.cpp
    src/core/editor/editorwindow.h
    src/core/editor/editorwindow.cpp
    src/core/editor/toolbar.h
    src/core/editor/toolbar.cpp
    src/core/pinboard/pinwindow.h
    src/core/pinboard/pinwindow.cpp
    src/utils/systemtray.h
    src/utils/systemtray.cpp
    src/utils/shortcutmanager.h
    src/utils/shortcutmanager.cpp
    src/utils/uiinspector.h
    src/utils/uiinspector.cpp
    src/utils/singleapplication.h
    src/utils/singleapplication.cpp
    src/utils/mousehook.h
    src/utils/mousehook.cpp
    src/utils/magnifier.h
    src/utils/magnifier.cpp
    src/utils/fontmanager.cpp
    src/utils/fontmanager.h
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
