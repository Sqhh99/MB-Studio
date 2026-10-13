/*--------------------------------------------------------------------
 *    MB-Studio:  mb_win32_runtime.h
 *
 *    See README.md file for copying and redistribution conditions.
 *--------------------------------------------------------------------*/
/**
 * @file
 * @brief Windows process start-up adaptation shared by MB-System programs.
 */

#ifndef MB_WIN32_RUNTIME_H_
#define MB_WIN32_RUNTIME_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MB_WIN32_PATH_MAX 4096

#ifdef _WIN32
/* Installation prefix: parent of the directory holding the executable. */
int mb_win32_install_prefix(char *prefix, size_t size);

/* Set environment variable name to <prefix>\relative_dir if it is unset and
   that directory exists. Returns 1 if set, 0 if already set, -1 otherwise. */
int mb_win32_default_env(const char *name, const char *relative_dir);

/* Run automatically at process start-up; exposed for tests. */
void mb_win32_runtime_init(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* MB_WIN32_RUNTIME_H_ */
