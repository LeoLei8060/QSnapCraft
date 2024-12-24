cmake_minimum_required(VERSION 3.16)
project(UIAutomationViewer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(UIAutomationViewer
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/uiautomation.cpp
    src/uiautomation.h
    src/screencapture.cpp
    src/screencapture.h
)

target_link_libraries(UIAutomationViewer PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    uiautomationcore
)
