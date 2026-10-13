#------------------------------------------------------------------------------
#    MB-Studio:  src/mbstudio/plugins/project/CMakeLists.txt
#
#    Survey projects: the project file, survey lines, products, processing
#    history and the processing chain on top of the MB-System services. It
#    uses Qt Core for JSON only (no user interface). The implementation is a
#    static library so that the tests can link it directly.
#------------------------------------------------------------------------------

add_library(mbstudio_project_core STATIC
  ProjectPlugin.cpp
  ProjectService.cpp)
set_target_properties(mbstudio_project_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_features(mbstudio_project_core PUBLIC cxx_std_17)
target_include_directories(mbstudio_project_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(mbstudio_project_core PRIVATE MBSTUDIO_VERSION="${MBSTUDIO_VERSION}")
target_link_libraries(mbstudio_project_core PUBLIC mbstudio_kernel Qt6::Core)

mbstudio_add_plugin(mbstudio_plugin_project project PluginEntry.cpp)
target_link_libraries(mbstudio_plugin_project PRIVATE mbstudio_project_core)
