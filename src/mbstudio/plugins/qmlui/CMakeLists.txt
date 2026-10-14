#------------------------------------------------------------------------------
#    MB-Studio:  src/mbstudio/plugins/qmlui/CMakeLists.txt
#
#    The Qt6/QML user interface plugin. Layers:
#      viewmodels/  C++ adapters between kernel services and QML (no UI)
#      qml/         the QML presentation (no logic beyond presentation)
#    The view models and resources form an object library shared by the
#    plugin and its tests.
#------------------------------------------------------------------------------

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_library(mbstudio_qmlui_core OBJECT
  QmlFrontend.cpp
  viewmodels/AppController.cpp
  viewmodels/EditorViews.cpp
  viewmodels/FormatListModel.cpp
  viewmodels/GridViewModel.cpp
  viewmodels/JobListModel.cpp
  viewmodels/LogModel.cpp
  viewmodels/MapView.cpp
  viewmodels/ProcessingViewModel.cpp
  viewmodels/ProjectViewModel.cpp
  viewmodels/SceneRenderer.cpp
  viewmodels/SoundingEditorModel.cpp
  viewmodels/SwathViewModel.cpp
  viewmodels/TerrainView.cpp
  viewmodels/TerrainViewModel.cpp
  viewmodels/ToolsViewModel.cpp
  viewmodels/TrackPlot.cpp
  viewmodels/Translator.cpp
  viewmodels/VariantListModel.cpp
  viewmodels/WindowChrome.cpp
  viewmodels/WorkspaceModel.cpp
  qmlui.qrc)
set_target_properties(mbstudio_qmlui_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_features(mbstudio_qmlui_core PUBLIC cxx_std_17)
target_include_directories(mbstudio_qmlui_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(mbstudio_qmlui_core PUBLIC
  MBSTUDIO_VERSION="${MBSTUDIO_VERSION}")
target_link_libraries(mbstudio_qmlui_core PUBLIC
  mbstudio_kernel Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Concurrent)

mbstudio_add_plugin(mbstudio_plugin_qmlui qmlui QmlUiPlugin.cpp)
target_link_libraries(mbstudio_plugin_qmlui PRIVATE mbstudio_qmlui_core)

# Translation catalogues must cover every qsTr()/tr() text of the sources.
if(buildTests)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(NAME mbstudio_translations_test
             COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/i18n/check_translations.py"
                     "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()
endif()

# Used by the Windows packaging step (windeployqt --qmldir).
set(MBSTUDIO_QML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/qml" CACHE INTERNAL "MB-Studio QML sources")
