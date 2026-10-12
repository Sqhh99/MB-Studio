# MB-Studio Windows triplet: native x64 Windows binaries built with the
# MinGW-w64 GCC toolchain, shared libraries, release-only.
#
# MB-System is written against POSIX APIs (getopt, unistd.h, strings.h, ...)
# that MinGW-w64 provides natively, so this triplet produces ordinary Win32
# executables and DLLs that run on stock Windows without WSL, Cygwin or Docker.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)
set(VCPKG_ENV_PASSTHROUGH PATH)

set(VCPKG_CMAKE_SYSTEM_NAME MinGW)
set(VCPKG_POLICY_DLLS_WITHOUT_LIBS enabled)

# Only release binaries are shipped; skipping the debug variants halves the
# dependency build time on CI.
set(VCPKG_BUILD_TYPE release)
