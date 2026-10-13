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

# Help pages of the MB-System programs: Markdown, one folder per language
# (help/<language>/<program>.md). They are installed for the help service and
# turned into an HTML help (share/doc/mbstudio/help) for offline reading.
set(MBSTUDIO_HELP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/help")
set(MBSTUDIO_HELP_LANGUAGES en ja zh_CN)
foreach(_lang IN LISTS MBSTUDIO_HELP_LANGUAGES)
  install(DIRECTORY "${MBSTUDIO_HELP_DIR}/${_lang}" DESTINATION ${CMAKE_INSTALL_DATADIR}/mbstudio/help
          FILES_MATCHING PATTERN "*.md")
endforeach()
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
  file(GLOB_RECURSE _help_pages CONFIGURE_DEPENDS "${MBSTUDIO_HELP_DIR}/*.md")
  set(MBSTUDIO_HELP_HTML_DIR "${CMAKE_BINARY_DIR}/share/doc/mbstudio/help")
  add_custom_command(
    OUTPUT "${MBSTUDIO_HELP_HTML_DIR}/index.html"
    COMMAND Python3::Interpreter "${MBSTUDIO_HELP_DIR}/build_html.py"
            --version "${MBSTUDIO_VERSION}" "${MBSTUDIO_HELP_DIR}" "${MBSTUDIO_HELP_HTML_DIR}"
    DEPENDS ${_help_pages} "${MBSTUDIO_HELP_DIR}/build_html.py"
    COMMENT "Generating the HTML help of the MB-System programs"
    VERBATIM)
  add_custom_target(mbstudio_help_html ALL DEPENDS "${MBSTUDIO_HELP_HTML_DIR}/index.html")
  install(DIRECTORY "${MBSTUDIO_HELP_HTML_DIR}/" DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/mbstudio/help)
else()
  message(WARNING "Python 3 not found: the HTML help of the MB-System programs is not generated")
endif()

add_subdirectory(kernel)
add_subdirectory(plugins/mbsystem)

find_package(Qt6 6.4 COMPONENTS Core Gui Qml Quick QuickControls2 Concurrent QUIET)
if(Qt6_FOUND)
  # Qt run-time locations, needed by the Windows deployment step at the top
  # level (find_package results are only visible in this directory scope).
  set(MB_QT_PLUGIN_DIR "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_PLUGINS}" CACHE INTERNAL "Qt plugin directory")
  set(MB_QT_QML_DIR "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_QML}" CACHE INTERNAL "Qt QML module directory")
  set(MB_QT_BIN_DIR "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}" CACHE INTERNAL "Qt binary directory")
  add_subdirectory(plugins/qmlui)
else()
  message(WARNING "Qt6 (Core Gui Qml Quick QuickControls2 Concurrent) not found: "
                  "MB-Studio is built without its graphical user interface plugin")
endif()

add_subdirectory(host)

if(buildTests)
  add_subdirectory(tests)
endif()
