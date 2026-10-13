#------------------------------------------------------------------------------
#    MB-Studio:  src/mbstudio/plugins/mbsystem/CMakeLists.txt
#
#    The MB-System adapter plugin. The implementation is a static library so
#    that the tests can exercise the services directly as well as through
#    the loaded plugin.
#------------------------------------------------------------------------------

find_package(NetCDF REQUIRED)

add_library(mbstudio_mbsystem_core STATIC
  FormatService.cpp
  GridRenderer.cpp
  GridService.cpp
  HelpService.cpp
  JobService.cpp
  MbSystemPlugin.cpp
  ProcessingService.cpp
  SwathService.cpp
  ToolService.cpp)
set_target_properties(mbstudio_mbsystem_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_features(mbstudio_mbsystem_core PUBLIC cxx_std_17)
target_include_directories(mbstudio_mbsystem_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(mbstudio_mbsystem_core PRIVATE MBSTUDIO_VERSION="${MBSTUDIO_VERSION}"
  # help pages of the build tree (the installed ones are found next to the program)
  MBSTUDIO_HELP_SOURCE_DIR="${MBSTUDIO_HELP_DIR}")
target_link_libraries(mbstudio_mbsystem_core PUBLIC mbstudio_kernel mbio NetCDF::NetCDF)

mbstudio_add_plugin(mbstudio_plugin_mbsystem mbsystem PluginEntry.cpp)
target_link_libraries(mbstudio_plugin_mbsystem PRIVATE mbstudio_mbsystem_core)
