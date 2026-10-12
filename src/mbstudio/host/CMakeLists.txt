#------------------------------------------------------------------------------
#    MB-Studio:  src/mbstudio/host/CMakeLists.txt
#
#    The mbstudio executable. It links only the kernel; the user interface and
#    all MB-System functionality are loaded as plugins at run time.
#------------------------------------------------------------------------------

add_executable(mbstudio main.cpp)
if(WIN32)
  string(REPLACE "." "," MBSTUDIO_VERSION_COMMAS "${MBSTUDIO_VERSION}")
  set(MBSTUDIO_ICON "${CMAKE_SOURCE_DIR}/packaging/windows/mbstudio.ico")
  configure_file(mbstudio.rc.in "${CMAKE_CURRENT_BINARY_DIR}/mbstudio.rc" @ONLY)
  target_sources(mbstudio PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/mbstudio.rc")
  # Windows GUI subsystem (no console window); the host attaches to the
  # parent console for its command line modes.
  set_target_properties(mbstudio PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
set_target_properties(mbstudio PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${MBSTUDIO_RUNTIME_BUILD_DIR}")
target_link_libraries(mbstudio PRIVATE mbstudio_kernel)
target_compile_definitions(mbstudio PRIVATE
  MBSTUDIO_VERSION="${MBSTUDIO_VERSION}"
  MBSTUDIO_PLUGIN_RELDIR="${MBSTUDIO_PLUGIN_RELDIR}"
  MBSTUDIO_BUILD_TOOL_DIR="${CMAKE_BINARY_DIR}/src/utilities")
install(TARGETS mbstudio DESTINATION ${CMAKE_INSTALL_BINDIR})
