# Cross-compile MB-Studio for 64-bit Windows from a Linux host with the
# MinGW-w64 GCC toolchain (developer convenience; the official Windows builds
# are produced natively on Windows by .github/workflows/build.yml).
#
#   cmake -S . -B build-win -G Ninja \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/x86_64-w64-mingw32.cmake \
#         -DCMAKE_PREFIX_PATH=/path/to/windows/dependency/prefix
#
# Tests can then be run under Wine with `ctest` because
# CMAKE_CROSSCOMPILING_EMULATOR is set to wine when it is available.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(_mingw_prefix x86_64-w64-mingw32)
find_program(CMAKE_C_COMPILER NAMES ${_mingw_prefix}-gcc-posix ${_mingw_prefix}-gcc REQUIRED)
find_program(CMAKE_CXX_COMPILER NAMES ${_mingw_prefix}-g++-posix ${_mingw_prefix}-g++ REQUIRED)
find_program(CMAKE_RC_COMPILER NAMES ${_mingw_prefix}-windres REQUIRED)

set(CMAKE_FIND_ROOT_PATH /usr/${_mingw_prefix})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

find_program(_mb_wine NAMES wine wine64)
if(_mb_wine)
  set(CMAKE_CROSSCOMPILING_EMULATOR ${_mb_wine})
endif()
