#------------------------------------------------------------------------------
#    MB-Studio:  src/mbstudio/tests/qmlui/CMakeLists.txt
#
#    Tests of the user interface layer: view models against fake and real
#    services, and a smoke test that loads every QML page offscreen.
#------------------------------------------------------------------------------

find_package(Qt6 COMPONENTS Test REQUIRED)

add_executable(mbstudio_qmlui_test QmlUiTest.cpp)
set_target_properties(mbstudio_qmlui_test PROPERTIES AUTOMOC ON)
target_link_libraries(mbstudio_qmlui_test PRIVATE
  mbstudio_qmlui_core mbstudio_mbsystem_core Qt6::Test)
target_compile_definitions(mbstudio_qmlui_test PRIVATE
  MBSTUDIO_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
  MBSTUDIO_TOOL_DIR="${CMAKE_BINARY_DIR}/src/utilities"
  MBSTUDIO_SCREENSHOT_DIR="${CMAKE_CURRENT_BINARY_DIR}/screenshots")
add_test(NAME mbstudio_qmlui_test COMMAND mbstudio_qmlui_test)
mb_set_test_environment(mbstudio_qmlui_test)

# Run headless, with the software scene graph, against the Qt plugins and
# QML modules of the Qt installation used for the build.
# QT_FORCE_STDERR_LOGGING: on Windows Qt otherwise sends its messages to the
# debugger when no console window is attached (as on CI), hiding failures.
set(_qt_env "QT_QPA_PLATFORM=offscreen" "QT_QUICK_BACKEND=software" "QT_QUICK_CONTROLS_STYLE=Material"
            "QT_FORCE_STDERR_LOGGING=1")
if(DEFINED QT6_INSTALL_PREFIX)
  list(APPEND _qt_env
    "QT_PLUGIN_PATH=${QT6_INSTALL_PREFIX}/${QT6_INSTALL_PLUGINS}"
    "QML_IMPORT_PATH=${QT6_INSTALL_PREFIX}/${QT6_INSTALL_QML}"
    "QML2_IMPORT_PATH=${QT6_INSTALL_PREFIX}/${QT6_INSTALL_QML}")
endif()
set_property(TEST mbstudio_qmlui_test APPEND PROPERTY ENVIRONMENT "${_qt_env}")
message(STATUS "MB-Studio UI test Qt plugins: ${QT6_INSTALL_PREFIX}/${QT6_INSTALL_PLUGINS}")
message(STATUS "MB-Studio UI test QML modules: ${QT6_INSTALL_PREFIX}/${QT6_INSTALL_QML}")
