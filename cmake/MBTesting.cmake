#------------------------------------------------------------------------------
#    MB-Studio:  cmake/MBTesting.cmake
#
#    Portable registration of the Python-driven MB-System tests.
#
#    On POSIX systems the test scripts are executable and run directly. On
#    Windows a script cannot be executed directly, the programs need the DLL
#    directories on PATH, and PROJ/GMT need their data directories, so tests
#    are run through the interpreter in MB_TEST_PYTHON with that environment.
#
#    MB_TEST_PYTHON     - command used to run the scripts (a CMake list, e.g.
#                         "wine;C:/Python312/python.exe" when cross-testing)
#    MB_TEST_RUNTIME_PATH - extra directories holding dependency DLLs
#                         (defaults to the vcpkg installed bin directory)
#------------------------------------------------------------------------------

if(NOT DEFINED MB_TEST_PYTHON)
  if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_FOUND)
      set(MB_TEST_PYTHON "${Python3_EXECUTABLE}")
    endif()
  endif()
endif()
set(MB_TEST_PYTHON "${MB_TEST_PYTHON}" CACHE STRING "Interpreter command used to run the Python tests")

if(WIN32 AND NOT DEFINED MB_TEST_RUNTIME_PATH)
  set(_mb_runtime_path "")
  if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
    list(APPEND _mb_runtime_path "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
  endif()
  if(CMAKE_C_COMPILER)
    get_filename_component(_mb_compiler_dir "${CMAKE_C_COMPILER}" DIRECTORY)
    list(APPEND _mb_runtime_path "${_mb_compiler_dir}")
  endif()
  set(MB_TEST_RUNTIME_PATH "${_mb_runtime_path}" CACHE STRING "Directories with dependency DLLs needed by the tests")
endif()

# Apply the Windows run-time environment to an already registered test.
function(mb_set_test_environment test)
  if(NOT WIN32)
    return()
  endif()
  set(_mods "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin")
  foreach(_dir IN LISTS MB_TEST_RUNTIME_PATH)
    list(APPEND _mods "PATH=path_list_append:${_dir}")
  endforeach()
  foreach(_dir IN LISTS MB_TEST_RUNTIME_PATH)
    get_filename_component(_prefix "${_dir}" DIRECTORY)
    if(EXISTS "${_prefix}/share/proj")
      list(APPEND _mods "PROJ_DATA=set:${_prefix}/share/proj")
    endif()
    if(EXISTS "${_prefix}/share/gmt")
      list(APPEND _mods "GMT_SHAREDIR=set:${_prefix}/share/gmt")
    endif()
  endforeach()
  list(APPEND _mods "PYTHONIOENCODING=set:utf-8")
  set_tests_properties(${test} PROPERTIES ENVIRONMENT_MODIFICATION "${_mods}")
endfunction()

# mb_add_python_test(<name> <script> [WORKING_DIRECTORY dir])
function(mb_add_python_test name script)
  cmake_parse_arguments(ARG "" "WORKING_DIRECTORY" "" ${ARGN})
  if(NOT IS_ABSOLUTE "${script}")
    set(script "${CMAKE_CURRENT_SOURCE_DIR}/${script}")
  endif()
  set(_wd "${CMAKE_CURRENT_BINARY_DIR}")
  if(ARG_WORKING_DIRECTORY)
    set(_wd "${ARG_WORKING_DIRECTORY}")
  endif()
  if(MB_TEST_PYTHON)
    add_test(NAME ${name} COMMAND ${MB_TEST_PYTHON} "${script}" WORKING_DIRECTORY "${_wd}")
  else()
    add_test(NAME ${name} COMMAND "${script}" WORKING_DIRECTORY "${_wd}")
  endif()
  mb_set_test_environment(${name})
endfunction()
