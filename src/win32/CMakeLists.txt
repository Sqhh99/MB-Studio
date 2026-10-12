#------------------------------------------------------------------------------
#    MB-Studio:  src/win32/CMakeLists.txt
#
#    Windows runtime support linked into every MB-System executable built for
#    Windows (see cmake/MBWindows.cmake, which attaches it automatically):
#
#      - mb_win32_runtime.c  : process start-up hook that puts stdout/stderr in
#                              binary mode and points PROJ/GMT/GDAL at the data
#                              shipped beside the executables
#      - mb_win32_runtime.rc : embeds mb_win32.manifest (UTF-8 active code page,
#                              long path awareness, Windows 10/11 compatibility)
#
#    See README.md file for copying and redistribution conditions.
#------------------------------------------------------------------------------

message("In src/win32")

add_library(mb_win32_runtime OBJECT mb_win32_runtime.c)
if(MINGW OR MSVC)
  target_sources(mb_win32_runtime PRIVATE mb_win32_runtime.rc)
  set_source_files_properties(mb_win32_runtime.rc PROPERTIES
    OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mb_win32.manifest")
endif()
target_include_directories(mb_win32_runtime INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
