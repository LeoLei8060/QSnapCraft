cmake_minimum_required(VERSION 3.16)
project(QSnapCraft VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(QSnapCraft
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/uiinspector.cpp
    src/uiinspector.h
    src/mousehook.cpp
    src/mousehook.h
)

target_link_libraries(QSnapCraft PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    UIAutomationCore
)
