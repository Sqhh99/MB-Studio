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

# Build C dependencies as C17: GCC 15+ defaults to C23, which some older C
# code in the dependency tree does not compile with.
set(VCPKG_C_FLAGS "-std=gnu17")
# (vcpkg requires VCPKG_CXX_FLAGS whenever VCPKG_C_FLAGS is set)
set(VCPKG_CXX_FLAGS "")
