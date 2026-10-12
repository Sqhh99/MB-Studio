#------------------------------------------------------------------------------
#    MB-Studio:  cmake/MBWindowsDeploy.cmake
#
#    Install-time deployment for Windows: makes ${CMAKE_INSTALL_PREFIX} a
#    self-contained application directory that runs on a stock Windows
#    system (it is what the Inno Setup installer packages):
#
#      bin/                    MB-System programs, mbstudio.exe, gmt.exe and
#                              every DLL they need (dependency closure)
#      bin/platforms, ...      Qt plugins          bin/qml/  Qt Quick modules
#      bin/gmt_plugins/        MB-System GMT supplement (mbsystem.dll)
#      lib/mbstudio/plugins/   MB-Studio plugins
#      share/{mbsystem,gmt,proj,gdal}  run-time data
#
#    Variables set by the install(CODE) call in CMakeLists.txt:
#      MB_DEPLOY_DEP_DIRS      directories searched for dependency DLLs
#      MB_DEPLOY_DATA_PREFIX   dependency prefix holding share/gmt, share/proj ...
#      MB_DEPLOY_GMT_EXE       the gmt driver program (optional)
#      MB_DEPLOY_QT_PLUGINS    Qt plugin directory (optional)
#      MB_DEPLOY_QT_QML        Qt QML module directory (optional)
#      MB_DEPLOY_OBJDUMP       objdump used to read DLL imports (MinGW)
#------------------------------------------------------------------------------

set(_prefix "${CMAKE_INSTALL_PREFIX}")
set(_bin "${_prefix}/bin")
message(STATUS "MB-Studio Windows deployment into ${_prefix}")

function(_mb_copy_dir src dst)
  if(EXISTS "${src}")
    file(COPY "${src}/" DESTINATION "${dst}")
    message(STATUS "  data: ${src} -> ${dst}")
  else()
    message(STATUS "  data: ${src} not found (skipped)")
  endif()
endfunction()

# Run-time data of the dependencies
if(MB_DEPLOY_DATA_PREFIX)
  _mb_copy_dir("${MB_DEPLOY_DATA_PREFIX}/share/gmt" "${_prefix}/share/gmt")
  _mb_copy_dir("${MB_DEPLOY_DATA_PREFIX}/share/proj" "${_prefix}/share/proj")
  _mb_copy_dir("${MB_DEPLOY_DATA_PREFIX}/share/gdal" "${_prefix}/share/gdal")
  # vcpkg bookkeeping files are not data
  file(REMOVE "${_prefix}/share/gmt/vcpkg.spdx.json" "${_prefix}/share/gmt/vcpkg_abi_info.txt"
              "${_prefix}/share/proj/vcpkg.spdx.json" "${_prefix}/share/proj/vcpkg_abi_info.txt"
              "${_prefix}/share/gdal/vcpkg.spdx.json" "${_prefix}/share/gdal/vcpkg_abi_info.txt")
  # GMT's own supplements, if built
  _mb_copy_dir("${MB_DEPLOY_DATA_PREFIX}/bin/gmt_plugins" "${_bin}/gmt_plugins")
endif()

if(MB_DEPLOY_GMT_EXE AND EXISTS "${MB_DEPLOY_GMT_EXE}")
  file(COPY "${MB_DEPLOY_GMT_EXE}" DESTINATION "${_bin}")
  message(STATUS "  gmt driver: ${MB_DEPLOY_GMT_EXE}")
endif()

# Qt: platform/image plugins next to the executables, QML modules in bin/qml.
# A relocatable Qt resolves its paths relative to the Qt6Core.dll it was
# built with (<prefix>/bin -> <prefix>/Qt6/qml), which does not match this
# layout, so a qt.conf next to the programs states it explicitly.
file(WRITE "${_bin}/qt.conf" "[Paths]\nPrefix = .\nPlugins = .\nQmlImports = qml\n")
if(MB_DEPLOY_QT_PLUGINS AND EXISTS "${MB_DEPLOY_QT_PLUGINS}")
  foreach(_kind platforms imageformats styles iconengines tls)
    if(EXISTS "${MB_DEPLOY_QT_PLUGINS}/${_kind}")
      file(GLOB _plugins "${MB_DEPLOY_QT_PLUGINS}/${_kind}/*.dll")
      list(FILTER _plugins EXCLUDE REGEX "d\\.dll$")  # debug variants
      file(COPY ${_plugins} DESTINATION "${_bin}/${_kind}")
    endif()
  endforeach()
  message(STATUS "  Qt plugins: ${MB_DEPLOY_QT_PLUGINS}")
endif()
if(MB_DEPLOY_QT_QML AND EXISTS "${MB_DEPLOY_QT_QML}")
  foreach(_module QML QtCore QtQml QtQuick Qt/labs/folderlistmodel Qt/labs/settings)
    if(EXISTS "${MB_DEPLOY_QT_QML}/${_module}")
      file(COPY "${MB_DEPLOY_QT_QML}/${_module}/" DESTINATION "${_bin}/qml/${_module}"
           PATTERN "*.qmltypes" EXCLUDE PATTERN "*d.dll" EXCLUDE)
    endif()
  endforeach()
  # modules that are not needed by MB-Studio
  file(REMOVE_RECURSE "${_bin}/qml/QtQuick/VirtualKeyboard" "${_bin}/qml/QtQuick/Scene2D"
                      "${_bin}/qml/QtQuick/Scene3D" "${_bin}/qml/QtQuick/Controls/designer")
  message(STATUS "  QML modules: ${MB_DEPLOY_QT_QML}")
endif()

# Dependency closure of every installed program and library.
file(GLOB_RECURSE _modules LIST_DIRECTORIES false "${_prefix}/*.exe" "${_prefix}/*.dll")
set(_executables "")
set(_libraries "")
foreach(_m IN LISTS _modules)
  if(_m MATCHES "\\.exe$")
    list(APPEND _executables "${_m}")
  else()
    list(APPEND _libraries "${_m}")
  endif()
endforeach()

if(MB_DEPLOY_OBJDUMP)
  set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM "windows+pe")
  set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL "objdump")
  set(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND "${MB_DEPLOY_OBJDUMP}")
endif()

set(_search_dirs "${_bin}" ${MB_DEPLOY_DEP_DIRS})
file(GET_RUNTIME_DEPENDENCIES
  EXECUTABLES ${_executables}
  LIBRARIES ${_libraries}
  DIRECTORIES ${_search_dirs}
  RESOLVED_DEPENDENCIES_VAR _resolved
  UNRESOLVED_DEPENDENCIES_VAR _unresolved
  CONFLICTING_DEPENDENCIES_PREFIX _conflicts
  # (the MSVC run time is excluded: MinGW builds do not use it)
  PRE_EXCLUDE_REGEXES "^api-ms-" "^ext-ms-" "^(vcruntime|msvcp|concrt|vccorlib|vcomp)[0-9_a-z]*\\.dll$" "^(kernel32|user32|gdi32|advapi32|shell32|ole32|oleaut32|ws2_32|comdlg32|winspool|uuid|msvcrt|ucrtbase|crypt32|bcrypt|secur32|shlwapi|version|winmm|dwmapi|uxtheme|imm32|setupapi|netapi32|userenv|iphlpapi|dnsapi|wldap32|normaliz|rpcrt4|d3d11|d3d12|dxgi|d3dcompiler_47|opengl32|glu32|mpr|dbghelp|wtsapi32|authz|comctl32|shcore|dwrite|d2d1|ntdll|cfgmgr32|powrprof|hid|ncrypt|mswsock|bthprops|wevtapi|wintrust|psapi|odbc32|odbccp32|winhttp|wininet|urlmon|PSAPI|ODBC32)\\.dll$"
  POST_EXCLUDE_REGEXES "[Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\\\][Ss]ystem32" "[Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\\\][Ss]ys[Ww][Oo][Ww]64")

set(_copied 0)
foreach(_dll IN LISTS _resolved)
  get_filename_component(_dir "${_dll}" DIRECTORY)
  get_filename_component(_name "${_dll}" NAME)
  if(NOT EXISTS "${_bin}/${_name}")
    file(COPY "${_dll}" DESTINATION "${_bin}")
    math(EXPR _copied "${_copied} + 1")
  endif()
endforeach()
message(STATUS "  ${_copied} dependency DLLs copied to bin")
foreach(_c IN LISTS _conflicts_FILENAMES)
  message(STATUS "  note: ${_c} found in several directories: ${_conflicts_${_c}}")
endforeach()
if(_unresolved)
  message(WARNING "MB-Studio deployment: unresolved DLLs: ${_unresolved}")
endif()
