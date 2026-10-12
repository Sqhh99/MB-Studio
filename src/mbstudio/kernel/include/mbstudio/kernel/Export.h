// MB-Studio kernel - symbol export macros.
//
// The kernel is a shared library that the host executable and every plugin
// link against, so that there is exactly one service registry, event bus and
// command registry per process.
#pragma once

#if defined(_WIN32)
#  if defined(MBSTUDIO_KERNEL_STATIC)
#    define MBSTUDIO_KERNEL_API
#  elif defined(mbstudio_kernel_EXPORTS)
#    define MBSTUDIO_KERNEL_API __declspec(dllexport)
#  else
#    define MBSTUDIO_KERNEL_API __declspec(dllimport)
#  endif
#  define MBSTUDIO_PLUGIN_EXPORT __declspec(dllexport)
#else
#  define MBSTUDIO_KERNEL_API __attribute__((visibility("default")))
#  define MBSTUDIO_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif
