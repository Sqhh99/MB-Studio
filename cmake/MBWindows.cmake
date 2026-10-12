#------------------------------------------------------------------------------
#    MB-Studio:  cmake/MBWindows.cmake
#
#    Native Windows (MinGW-w64 or MSVC) adaptation of the MB-System build.
#
#    mb_windows_setup()     - called before the source tree is processed
#    mb_windows_finalize()  - called after all targets exist; it
#        * links the Windows runtime shim (src/win32) into every executable,
#        * collects every DLL in ${CMAKE_BINARY_DIR}/bin so the programs and
#          tests in the build tree can find them through a single PATH entry.
#
#    See README.md file for copying and redistribution conditions.
#------------------------------------------------------------------------------

macro(mb_windows_setup)
  # Windows has no RPATH: installed DLLs live beside the executables in bin/.
  set(CMAKE_SKIP_RPATH ON)
  # Use the ANSI (UTF-8 via the manifest) Win32 API everywhere and keep
  # <windows.h> from defining min/max macros that clash with C++ code.
  add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
  if(MINGW)
    # 64-bit off_t/fseeko/ftello so data files larger than 2 GB can be read.
    add_compile_definitions(_FILE_OFFSET_BITS=64 __USE_MINGW_ANSI_STDIO=1)
  endif()
  # MB-System programs keep large working arrays on the stack (several MB in
  # e.g. mbauvloglist). Linux and macOS reserve 8 MB or more for the main
  # thread, Windows only 1 MB, so reserve a Linux-like stack for executables.
  if(MINGW)
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -Wl,--stack,67108864")
  elseif(MSVC)
    string(APPEND CMAKE_EXE_LINKER_FLAGS " /STACK:67108864")
  endif()
  set(MB_WINDOWS_DLL_DIR "${CMAKE_BINARY_DIR}/bin")
  file(MAKE_DIRECTORY "${MB_WINDOWS_DLL_DIR}")
endmacro()

function(_mb_windows_collect_targets dir out_var)
  get_property(_targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
  get_property(_subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
  foreach(_sub IN LISTS _subdirs)
    _mb_windows_collect_targets("${_sub}" _sub_targets)
    list(APPEND _targets ${_sub_targets})
  endforeach()
  set(${out_var} ${_targets} PARENT_SCOPE)
endfunction()

function(mb_windows_finalize)
  _mb_windows_collect_targets("${CMAKE_SOURCE_DIR}" _all_targets)
  foreach(_t IN LISTS _all_targets)
    get_target_property(_type ${_t} TYPE)
    get_target_property(_imported ${_t} IMPORTED)
    if(_imported)
      continue()
    endif()
    if(_type STREQUAL "EXECUTABLE")
      target_link_libraries(${_t} PRIVATE mb_win32_runtime)
    elseif(_type STREQUAL "SHARED_LIBRARY")
      set_target_properties(${_t} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${MB_WINDOWS_DLL_DIR}")
    endif()
  endforeach()
endfunction()
