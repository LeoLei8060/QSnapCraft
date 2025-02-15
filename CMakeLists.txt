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
    src/core/windowmanager.h
    src/core/windowmanager.cpp
    resources/resources.qrc
    resources/QSnapCraft.rc
)

set(SOURCES_CORE_EDITOR
    src/core/editor/shapes/arrow.cpp
    src/core/editor/shapes/arrow.h
    src/core/editor/shapes/ellipse.cpp
    src/core/editor/shapes/ellipse.h
    src/core/editor/shapes/eraser.h
    src/core/editor/shapes/eraser.cpp
    src/core/editor/shapes/mosaic.cpp
    src/core/editor/shapes/mosaic.h
    src/core/editor/shapes/rectangle.cpp
    src/core/editor/shapes/rectangle.h
    src/core/editor/shapes/text.cpp
    src/core/editor/shapes/text.h
    src/core/editor/shapes/polyline.cpp
    src/core/editor/shapes/polyline.h
    src/core/editor/shapes/freehand.cpp
    src/core/editor/shapes/freehand.h
    src/core/editor/shapes/highlighter.cpp
    src/core/editor/shapes/highlighter.h
    src/core/editor/shapes/shape.h
    src/core/editor/toolbar.cpp
    src/core/editor/toolbar.h
    src/core/editor/colorpick.cpp
    src/core/editor/colorpick.h
    src/core/editor/editorwindow.cpp
    src/core/editor/editorwindow.h
)

set(SOURCES_CORE_SCREENSHOT
    src/core/screenshot/screenshotwindow.cpp
    src/core/screenshot/screenshotwindow.h
)

set(SOURCES_CORE_PINBOARD
    src/core/pinboard/pinwindow.cpp
    src/core/pinboard/pinwindow.h
)

set(SOURCES_UTILS
    src/utils/systemtray.cpp
    src/utils/systemtray.h
    src/utils/shortcutmanager.cpp
    src/utils/shortcutmanager.h
    src/utils/uiinspector.cpp
    src/utils/uiinspector.h
    src/utils/singleapplication.cpp
    src/utils/singleapplication.h
    src/utils/mousehook.cpp
    src/utils/mousehook.h
    src/utils/magnifier.cpp
    src/utils/magnifier.h
    src/utils/fontmanager.cpp
    src/utils/fontmanager.h
    src/utils/systemutils.h
    src/utils/systemutils.cpp
)

set(SOURCES_SETTINGS
    src/settings/settingswindow.cpp
    src/settings/settingswindow.h
    src/settings/settingswindow.ui
    src/settings/generalsettings.cpp
    src/settings/generalsettings.h
    src/settings/general.ui
    src/settings/screenshotsettings.cpp
    src/settings/screenshotsettings.h
    src/settings/screenshot.ui
    src/settings/outputsettings.cpp
    src/settings/outputsettings.h
    src/settings/output.ui
    src/settings/controlsettings.cpp
    src/settings/controlsettings.h
    src/settings/control.ui
    src/settings/aboutsettings.cpp
    src/settings/aboutsettings.h
    src/settings/about.ui
    src/settings/globalconfig.h
    src/settings/globalconfig.cpp
    src/settings/settingsdata.h
)

set(SOURCES_WIDGETS
    src/widgets/qhotkeylineedit.h
    src/widgets/qhotkeylineedit.cpp
)

add_executable(QSnapCraft WIN32
    ${PROJECT_SOURCES}
    ${SOURCES_CORE_EDITOR}
    ${SOURCES_CORE_SCREENSHOT}
    ${SOURCES_CORE_PINBOARD}
    ${SOURCES_UTILS}
    ${SOURCES_SETTINGS}
    ${SOURCES_WIDGETS}
)

target_link_libraries(QSnapCraft PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    hooks
)

target_include_directories(QSnapCraft PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets
)
