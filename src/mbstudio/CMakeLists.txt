#------------------------------------------------------------------------------
#    MB-Studio:  src/mbstudio/CMakeLists.txt
#
#    MB-Studio is a microkernel application:
#
#      kernel/            mbstudio_kernel shared library: plugin manager,
#                         service registry, event bus, command registry and
#                         the platform layer. No Qt, no MB-System.
#      plugins/mbsystem/  adapter plugin turning the MBIO library, netCDF
#                         grids and the MB-System programs into services and
#                         commands (no Qt).
#      plugins/qmlui/     the Qt6/QML graphical user interface, itself a
#                         plugin providing the IFrontend service.
#      host/              mbstudio executable: starts the kernel and hands
#                         control to the front end (or runs headless).
#
#    See src/mbstudio/README.md for the architecture.
#------------------------------------------------------------------------------

message("In src/mbstudio")

set(MBSTUDIO_PLUGIN_RELDIR "${CMAKE_INSTALL_LIBDIR}/mbstudio/plugins")
set(MBSTUDIO_PLUGIN_BUILD_DIR "${CMAKE_BINARY_DIR}/${MBSTUDIO_PLUGIN_RELDIR}")
set(MBSTUDIO_RUNTIME_BUILD_DIR "${CMAKE_BINARY_DIR}/bin")
file(MAKE_DIRECTORY "${MBSTUDIO_PLUGIN_BUILD_DIR}")

# mbstudio_add_plugin(<target> <name> sources...)
# Builds mbstudio-plugin-<name>.(dll|so|dylib) into the plugin directory.
function(mbstudio_add_plugin target name)
  add_library(${target} MODULE ${ARGN})
  set_target_properties(${target} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "mbstudio-plugin-${name}"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    LIBRARY_OUTPUT_DIRECTORY "${MBSTUDIO_PLUGIN_BUILD_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${MBSTUDIO_PLUGIN_BUILD_DIR}")
  target_compile_features(${target} PRIVATE cxx_std_17)
  target_link_libraries(${target} PRIVATE mbstudio_kernel)
  install(TARGETS ${target}
          LIBRARY DESTINATION ${MBSTUDIO_PLUGIN_RELDIR}
          RUNTIME DESTINATION ${MBSTUDIO_PLUGIN_RELDIR})
endfunction()

add_subdirectory(kernel)
add_subdirectory(plugins/mbsystem)

find_package(Qt6 6.4 COMPONENTS Core Gui Qml Quick QuickControls2 Concurrent Svg QUIET)
if(Qt6_FOUND)
  add_subdirectory(plugins/qmlui)
else()
  message(WARNING "Qt6 (Core Gui Qml Quick QuickControls2 Concurrent Svg) not found: "
                  "MB-Studio is built without its graphical user interface plugin")
endif()

add_subdirectory(host)

if(buildTests)
  add_subdirectory(tests)
endif()
