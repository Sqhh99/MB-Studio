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
    # Recent MinGW-w64 GCC releases default to C23, which rejects the
    # pre-ANSI style declarations found in some of the older MB-System code.
    if(NOT CMAKE_C_STANDARD)
      set(CMAKE_C_STANDARD 17)
      set(CMAKE_C_EXTENSIONS ON)
    endif()
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

# Install-time deployment of the dependency DLLs, data and Qt run time
# (cmake/MBWindowsDeploy.cmake). The locations come from the dependency
# prefix (vcpkg installed tree or MB_DEPENDENCY_PREFIX) and the compiler.
function(mb_windows_install_deployment)
  option(MB_DEPLOY_DEPENDENCIES "Copy dependency DLLs and data when installing on Windows" ON)
  if(NOT MB_DEPLOY_DEPENDENCIES)
    return()
  endif()
  set(_prefix "")
  if(DEFINED MB_DEPENDENCY_PREFIX)
    set(_prefix "${MB_DEPENDENCY_PREFIX}")
  elseif(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
    set(_prefix "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
  endif()
  set(_dirs "")
  if(_prefix)
    list(APPEND _dirs "${_prefix}/bin")
  endif()
  get_filename_component(_compiler_dir "${CMAKE_C_COMPILER}" DIRECTORY)
  list(APPEND _dirs "${_compiler_dir}")
  # MinGW run time (libgcc, libstdc++, winpthread) of a cross compiler
  if(MINGW)
    execute_process(COMMAND "${CMAKE_CXX_COMPILER}" -print-file-name=libstdc++-6.dll
                    OUTPUT_VARIABLE _libstdcxx OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND "${CMAKE_C_COMPILER}" -print-file-name=libwinpthread-1.dll
                    OUTPUT_VARIABLE _pthread OUTPUT_STRIP_TRAILING_WHITESPACE)
    foreach(_f "${_libstdcxx}" "${_pthread}")
      if(IS_ABSOLUTE "${_f}" AND EXISTS "${_f}")
        get_filename_component(_d "${_f}" DIRECTORY)
        list(APPEND _dirs "${_d}")
      endif()
    endforeach()
  endif()
  list(APPEND _dirs ${MB_DEPLOY_EXTRA_DLL_DIRS})
  list(REMOVE_DUPLICATES _dirs)

  set(_gmt_exe "")
  foreach(_candidate "${_prefix}/tools/gmt/gmt.exe" "${_prefix}/bin/gmt.exe")
    if(EXISTS "${_candidate}")
      set(_gmt_exe "${_candidate}")
      break()
    endif()
  endforeach()

  # Set by src/mbstudio when the Qt user interface is built.
  set(_qt_plugins "${MB_QT_PLUGIN_DIR}")
  set(_qt_qml "${MB_QT_QML_DIR}")
  if(MB_QT_BIN_DIR)
    list(APPEND _dirs "${MB_QT_BIN_DIR}")
  endif()

  set(_objdump "")
  if(MINGW AND CMAKE_OBJDUMP)
    set(_objdump "${CMAKE_OBJDUMP}")
  endif()

  install(CODE "
    set(MB_DEPLOY_DEP_DIRS \"${_dirs}\")
    set(MB_DEPLOY_DATA_PREFIX \"${_prefix}\")
    set(MB_DEPLOY_GMT_EXE \"${_gmt_exe}\")
    set(MB_DEPLOY_QT_PLUGINS \"${_qt_plugins}\")
    set(MB_DEPLOY_QT_QML \"${_qt_qml}\")
    set(MB_DEPLOY_OBJDUMP \"${_objdump}\")
    include(\"${CMAKE_SOURCE_DIR}/cmake/MBWindowsDeploy.cmake\")
  ")
endfunction()
