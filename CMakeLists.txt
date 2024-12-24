cmake_minimum_required(VERSION 3.14)

project(QSnapCraft VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/screencapture.cpp
    src/screencapture.h
)

add_executable(QSnapCraft
    ${PROJECT_SOURCES}
)

target_link_libraries(QSnapCraft PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
)

if(WIN32)
    target_link_libraries(QSnapCraft PRIVATE
        dwmapi
    )
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(QSnapCraft)
endif()
