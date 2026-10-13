/*--------------------------------------------------------------------
 *    MB-Studio:  mb_win32_runtime.c
 *
 *    See README.md file for copying and redistribution conditions.
 *--------------------------------------------------------------------*/
/**
 * @file
 * @brief Process start-up adaptation for MB-System programs on Windows.
 *
 * MB-System's command line programs were written for POSIX systems, where
 * there is no distinction between text and binary streams and where the
 * locations of the PROJ and GMT support data are compiled in. On Windows
 * every executable gets this translation unit linked in (via an OBJECT
 * library, so the constructor below can never be dropped by the linker):
 *
 *  - stdout and stderr are switched to binary mode, so that programs emit
 *    exactly the same bytes as on Linux and macOS ("\n" line endings and no
 *    0x1A end-of-file handling). This keeps output piped between programs
 *    and compared by the test suite identical across platforms.
 *  - PROJ_DATA, GMT_SHAREDIR and GDAL_DATA default to the share/ directory
 *    of the installation (the parent of the directory holding the
 *    executable) when they are not already set, so that the programs work
 *    from any console without extra configuration.
 */

#ifdef _WIN32

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "mb_win32_runtime.h"

/* Return the installation prefix (the parent of the directory containing
   the running executable) in prefix, or an empty string on failure. */
int mb_win32_install_prefix(char *prefix, size_t size) {
  if (prefix == NULL || size == 0)
    return -1;
  prefix[0] = '\0';

  char exe[MB_WIN32_PATH_MAX];
  const DWORD n = GetModuleFileNameA(NULL, exe, (DWORD)sizeof(exe));
  if (n == 0 || n >= sizeof(exe))
    return -1;

  /* strip the executable name and then the bin directory */
  for (int level = 0; level < 2; level++) {
    char *sep = strrchr(exe, '\\');
    char *alt = strrchr(exe, '/');
    if (alt != NULL && (sep == NULL || alt > sep))
      sep = alt;
    if (sep == NULL)
      return -1;
    *sep = '\0';
  }

  if (strlen(exe) + 1 > size)
    return -1;
  strcpy(prefix, exe);
  return 0;
}

static int mb_win32_is_directory(const char *path) {
  const DWORD attributes = GetFileAttributesA(path);
  return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

int mb_win32_default_env(const char *name, const char *relative_dir) {
  const char *current = getenv(name);
  if (current != NULL && current[0] != '\0')
    return 0; /* respect the user's setting */

  char prefix[MB_WIN32_PATH_MAX];
  if (mb_win32_install_prefix(prefix, sizeof(prefix)) != 0)
    return -1;

  char path[MB_WIN32_PATH_MAX];
  const int len = snprintf(path, sizeof(path), "%s\\%s", prefix, relative_dir);
  if (len < 0 || (size_t)len >= sizeof(path))
    return -1;
  if (!mb_win32_is_directory(path))
    return -1;
  return _putenv_s(name, path) == 0 ? 1 : -1;
}

void mb_win32_runtime_init(void) {
  /* Binary stdout/stderr: identical output bytes on every platform */
  fflush(stdout);
  fflush(stderr);
  _setmode(_fileno(stdout), _O_BINARY);
  _setmode(_fileno(stderr), _O_BINARY);

  /* Locate the support data shipped with the installation */
  mb_win32_default_env("PROJ_DATA", "share\\proj");
  mb_win32_default_env("GMT_SHAREDIR", "share\\gmt");
  mb_win32_default_env("GDAL_DATA", "share\\gdal");
}

#if defined(__GNUC__)
__attribute__((constructor)) static void mb_win32_runtime_constructor(void) {
  mb_win32_runtime_init();
}
#elif defined(_MSC_VER)
static void __cdecl mb_win32_runtime_constructor(void) {
  mb_win32_runtime_init();
}
#pragma section(".CRT$XCU", read)
__declspec(allocate(".CRT$XCU")) void(__cdecl *mb_win32_runtime_constructor_ptr)(void) =
    mb_win32_runtime_constructor;
#endif

#endif /* _WIN32 */
