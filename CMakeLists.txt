cmake_minimum_required(VERSION 3.24)
project(QSnapCraft VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

find_package(Qt6 6.7 REQUIRED COMPONENTS Widgets Network Test)

set(QSNAPCRAFT_SOURCES
    src/app/applicationaction.cpp
    src/app/applicationaction.h
    src/app/applicationcontroller.cpp
    src/app/applicationcontroller.h
    src/app/singleapplication.cpp
    src/app/singleapplication.h
    src/app/windowmanager.cpp
    src/app/windowmanager.h
    src/capture/captureoptions.h
    src/capture/capturetypes.cpp
    src/capture/capturetypes.h
    src/capture/capturesession.cpp
    src/capture/capturesession.h
    src/capture/selectionstate.cpp
    src/capture/selectionstate.h
    src/capture/scrollstitcher.cpp
    src/capture/scrollstitcher.h
    src/config/globalconfig.cpp
    src/config/globalconfig.h
    src/config/pinoptions.h
    src/config/preferencessettings.cpp
    src/config/preferencessettings.h
    src/config/settingsdata.h
    src/config/snipsettings.cpp
    src/config/snipsettings.h
    src/output/outputservice.cpp
    src/output/outputservice.h
    src/paint/paintitem.cpp
    src/paint/paintitem.h
    src/paint/paintscene.cpp
    src/paint/paintscene.h
    src/paint/painttextitem.cpp
    src/paint/painttextitem.h
    src/paint/paintview.cpp
    src/paint/paintview.h
    src/paint/paintwidths.h
    src/pinboard/pinwindow.cpp
    src/pinboard/pinwindow.h
    src/pinboard/pinpool.cpp
    src/pinboard/pinpool.h
    src/pinboard/textimagerenderer.cpp
    src/pinboard/textimagerenderer.h
    src/platform/windows/desktopcapture.cpp
    src/platform/windows/desktopcapture.h
    src/platform/windows/elementdetector.cpp
    src/platform/windows/elementdetector.h
    src/platform/windows/foregroundwindow.cpp
    src/platform/windows/foregroundwindow.h
    src/platform/windows/ocrengine.cpp
    src/platform/windows/ocrengine.h
    src/platform/windows/shortcutmanager.cpp
    src/platform/windows/shortcutmanager.h
    src/platform/windows/systemutils.cpp
    src/platform/windows/systemutils.h
    src/ui/capturebargeometry.cpp
    src/ui/capturebargeometry.h
    src/ui/capturebar.cpp
    src/ui/capturebar.h
    src/ui/captureoverlay.cpp
    src/ui/captureoverlay.h
    src/ui/colorpickerwindow.cpp
    src/ui/colorpickerwindow.h
    src/ui/glyphicon.cpp
    src/ui/glyphicon.h
    src/ui/magnifier.cpp
    src/ui/magnifier.h
    src/ui/ocrdialog.cpp
    src/ui/ocrdialog.h
    src/ui/paintbar.cpp
    src/ui/paintbar.h
    src/ui/rulermeasurement.cpp
    src/ui/rulermeasurement.h
    src/ui/screenrulerwindow.cpp
    src/ui/screenrulerwindow.h
    src/ui/scrollcaptureindicator.cpp
    src/ui/scrollcaptureindicator.h
    src/ui/systemtray.cpp
    src/ui/systemtray.h
    src/ui/settings/aboutsettings.cpp
    src/ui/settings/aboutsettings.h
    src/ui/settings/controlsettings.cpp
    src/ui/settings/controlsettings.h
    src/ui/settings/generalsettings.cpp
    src/ui/settings/generalsettings.h
    src/ui/settings/interfacesettings.cpp
    src/ui/settings/interfacesettings.h
    src/ui/settings/ocrsettings.cpp
    src/ui/settings/ocrsettings.h
    src/ui/settings/outputsettings.cpp
    src/ui/settings/outputsettings.h
    src/ui/settings/pinsettings.cpp
    src/ui/settings/pinsettings.h
    src/ui/settings/screenshotsettings.cpp
    src/ui/settings/screenshotsettings.h
    src/ui/settings/settingswindow.cpp
    src/ui/settings/settingswindow.h
    src/ui/widgets/qhotkeylineedit.cpp
    src/ui/widgets/qhotkeylineedit.h
)

add_library(QSnapCraftCore STATIC ${QSNAPCRAFT_SOURCES})
target_include_directories(QSnapCraftCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(QSnapCraftCore PUBLIC
    Qt6::Widgets
    Qt6::Network
    Dwmapi
    WindowsApp
    oleacc
    ole32
    oleaut32
)

add_executable(QSnapCraft WIN32
    src/main.cpp
    resources/resources.qrc
    resources/QSnapCraft.rc
)
target_link_libraries(QSnapCraft PRIVATE QSnapCraftCore)

include(CTest)

set(QSNAPCRAFT_TEST_SOURCES
    tests/applicationactiontest.cpp
    tests/capturebargeometrytest.cpp
    tests/capturebartest.cpp
    tests/captureoverlaytest.cpp
    tests/capturesessiontest.cpp
    tests/colorpickertest.cpp
    tests/desktopcapturetest.cpp
    tests/elementdetectortest.cpp
    tests/foregroundwindowtest.cpp
    tests/outputservicetest.cpp
    tests/ocrenginetest.cpp
    tests/paintbartest.cpp
    tests/paintscenetest.cpp
    tests/pinwindowtest.cpp
    tests/preferencessettingstest.cpp
    tests/rulermeasurementtest.cpp
    tests/screenrulerwindowtest.cpp
    tests/selectionstatetest.cpp
    tests/shortcutmanagertest.cpp
    tests/settingswindowtest.cpp
    tests/scrollstitchertest.cpp
    tests/snipsettingstest.cpp
    tests/systemtraytest.cpp
    tests/textimagerenderertest.cpp
)

set_source_files_properties(src/main.cpp PROPERTIES
    OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/app/singleapplication.h"
)
set_property(SOURCE src/main.cpp APPEND PROPERTY
    OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/app/applicationcontroller.h"
)

# The localized MSVC /showIncludes output does not populate JOM's compiler
# dependency files on this toolchain. Keep class-layout dependencies explicit
# for translation units that allocate or destroy complete class types.
set_property(SOURCE src/app/applicationcontroller.cpp APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/app/applicationcontroller.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/capture/capturesession.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/app/windowmanager.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/config/globalconfig.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/capturebar.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/windows/shortcutmanager.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/systemtray.h"
)
set_property(SOURCE src/app/windowmanager.cpp APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/app/windowmanager.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/pinboard/pinwindow.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/settings/settingswindow.h"
)
set_property(SOURCE src/config/globalconfig.cpp APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/config/globalconfig.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/config/preferencessettings.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/config/settingsdata.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/capture/captureoptions.h"
)
set_property(SOURCE src/config/preferencessettings.cpp APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/config/preferencessettings.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/config/settingsdata.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/config/snipsettings.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/capture/captureoptions.h"
)
set_property(SOURCE src/capture/capturesession.cpp APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/capture/capturesession.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/capture/capturetypes.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/output/outputservice.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/paint/paintscene.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/captureoverlay.h"
)
set_property(SOURCE src/paint/paintitem.cpp APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/paint/paintitem.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/paint/paintscene.h"
)
set_property(SOURCE src/paint/paintscene.cpp APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/paint/paintitem.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/paint/paintscene.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/paint/painttextitem.h"
)
set_property(SOURCE src/paint/painttextitem.cpp APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/paint/painttextitem.h"
)
set_property(SOURCE src/paint/paintview.cpp APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/paint/paintscene.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/paint/paintview.h"
)
set_property(SOURCE src/ui/captureoverlay.cpp APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/paint/paintscene.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/paint/paintview.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/captureoverlay.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/paintbar.h"
)
set_property(SOURCE src/ui/paintbar.cpp APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/paint/paintscene.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/paintbar.h"
)
set_property(SOURCE src/pinboard/pinwindow.cpp APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/pinboard/pinwindow.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/glyphicon.h"
)
set_property(SOURCE src/ui/settings/settingswindow.cpp APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/settings/settingswindow.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/settings/aboutsettings.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/settings/controlsettings.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/settings/generalsettings.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/settings/interfacesettings.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/settings/outputsettings.h;"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/settings/screenshotsettings.h"
)

set(QSNAPCRAFT_HEADERS ${QSNAPCRAFT_SOURCES})
list(FILTER QSNAPCRAFT_HEADERS INCLUDE REGEX "\\.h$")
set(QSNAPCRAFT_HEADER_DEPENDENCIES)
foreach(header IN LISTS QSNAPCRAFT_HEADERS)
    list(APPEND QSNAPCRAFT_HEADER_DEPENDENCIES
        "${CMAKE_CURRENT_SOURCE_DIR}/${header}"
    )
endforeach()
set(QSNAPCRAFT_SOURCE_FILES ${QSNAPCRAFT_SOURCES})
list(FILTER QSNAPCRAFT_SOURCE_FILES EXCLUDE REGEX "\\.h$")
foreach(source_file IN LISTS QSNAPCRAFT_SOURCE_FILES)
    set_property(SOURCE ${source_file} APPEND PROPERTY OBJECT_DEPENDS
        "${QSNAPCRAFT_HEADER_DEPENDENCIES}"
    )
endforeach()
foreach(test_source IN LISTS QSNAPCRAFT_TEST_SOURCES)
    set_property(SOURCE ${test_source} APPEND PROPERTY OBJECT_DEPENDS
        "${QSNAPCRAFT_HEADER_DEPENDENCIES}"
    )
endforeach()

if(BUILD_TESTING)
    add_executable(ApplicationActionTest
        tests/applicationactiontest.cpp
    )
    target_link_libraries(ApplicationActionTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME ApplicationAction COMMAND ApplicationActionTest)
    set_tests_properties(ApplicationAction PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(CaptureBarGeometryTest
        tests/capturebargeometrytest.cpp
    )
    target_link_libraries(CaptureBarGeometryTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME CaptureBarGeometry COMMAND CaptureBarGeometryTest)
    set_tests_properties(CaptureBarGeometry PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(CaptureBarTest
        tests/capturebartest.cpp
    )
    target_link_libraries(CaptureBarTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME CaptureBar COMMAND CaptureBarTest)
    set_tests_properties(CaptureBar PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(ForegroundWindowTest
        tests/foregroundwindowtest.cpp
    )
    target_link_libraries(ForegroundWindowTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME ForegroundWindow COMMAND ForegroundWindowTest)
    set_tests_properties(ForegroundWindow PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(OutputServiceTest
        tests/outputservicetest.cpp
    )
    target_link_libraries(OutputServiceTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME OutputService COMMAND OutputServiceTest)
    set_tests_properties(OutputService PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(OcrEngineTest
        tests/ocrenginetest.cpp
    )
    target_link_libraries(OcrEngineTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME OcrEngine COMMAND OcrEngineTest)
    set_tests_properties(OcrEngine PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(DesktopCaptureTest
        tests/desktopcapturetest.cpp
    )
    target_link_libraries(DesktopCaptureTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME DesktopCapture COMMAND DesktopCaptureTest)
    set_tests_properties(DesktopCapture PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(ColorPickerTest
        tests/colorpickertest.cpp
    )
    target_link_libraries(ColorPickerTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME ColorPicker COMMAND ColorPickerTest)
    set_tests_properties(ColorPicker PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(SelectionStateTest
        tests/selectionstatetest.cpp
    )
    target_link_libraries(SelectionStateTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME SelectionState COMMAND SelectionStateTest)
    set_tests_properties(SelectionState PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(ElementDetectorTest
        tests/elementdetectortest.cpp
    )
    target_link_libraries(ElementDetectorTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME ElementDetector COMMAND ElementDetectorTest)
    set_tests_properties(ElementDetector PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(PaintSceneTest
        tests/paintscenetest.cpp
    )
    target_link_libraries(PaintSceneTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME PaintScene COMMAND PaintSceneTest)
    set_tests_properties(PaintScene PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(PinWindowTest
        tests/pinwindowtest.cpp
    )
    target_link_libraries(PinWindowTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME PinWindow COMMAND PinWindowTest)
    set_tests_properties(PinWindow PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(SnipSettingsTest
        tests/snipsettingstest.cpp
    )
    target_link_libraries(SnipSettingsTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME SnipSettings COMMAND SnipSettingsTest)
    set_tests_properties(SnipSettings PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(ShortcutManagerTest
        tests/shortcutmanagertest.cpp
    )
    target_link_libraries(ShortcutManagerTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME ShortcutManager COMMAND ShortcutManagerTest)
    set_tests_properties(ShortcutManager PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(PreferencesSettingsTest
        tests/preferencessettingstest.cpp
    )
    target_link_libraries(PreferencesSettingsTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME PreferencesSettings COMMAND PreferencesSettingsTest)
    set_tests_properties(PreferencesSettings PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(SystemTrayTest
        tests/systemtraytest.cpp
    )
    target_link_libraries(SystemTrayTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME SystemTray COMMAND SystemTrayTest)
    set_tests_properties(SystemTray PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(TextImageRendererTest
        tests/textimagerenderertest.cpp
    )
    target_link_libraries(TextImageRendererTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME TextImageRenderer COMMAND TextImageRendererTest)
    set_tests_properties(TextImageRenderer PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(SettingsWindowTest
        tests/settingswindowtest.cpp
        resources/resources.qrc
    )
    target_link_libraries(SettingsWindowTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME SettingsWindow COMMAND SettingsWindowTest)
    set_tests_properties(SettingsWindow PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(ScrollStitcherTest
        tests/scrollstitchertest.cpp
    )
    target_link_libraries(ScrollStitcherTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME ScrollStitcher COMMAND ScrollStitcherTest)
    set_tests_properties(ScrollStitcher PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(PaintBarTest
        tests/paintbartest.cpp
    )
    target_link_libraries(PaintBarTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME PaintBar COMMAND PaintBarTest)
    set_tests_properties(PaintBar PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(CaptureOverlayTest
        tests/captureoverlaytest.cpp
    )
    target_link_libraries(CaptureOverlayTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME CaptureOverlay COMMAND CaptureOverlayTest)
    set_tests_properties(CaptureOverlay PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(RulerMeasurementTest
        tests/rulermeasurementtest.cpp
    )
    target_link_libraries(RulerMeasurementTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME RulerMeasurement COMMAND RulerMeasurementTest)
    set_tests_properties(RulerMeasurement PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(ScreenRulerWindowTest
        tests/screenrulerwindowtest.cpp
    )
    target_link_libraries(ScreenRulerWindowTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME ScreenRulerWindow COMMAND ScreenRulerWindowTest)
    set_tests_properties(ScreenRulerWindow PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )

    add_executable(CaptureSessionTest
        tests/capturesessiontest.cpp
    )
    target_link_libraries(CaptureSessionTest PRIVATE
        QSnapCraftCore
        Qt6::Test
    )
    add_test(NAME CaptureSession COMMAND CaptureSessionTest)
    set_tests_properties(CaptureSession PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
endif()
