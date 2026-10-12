# Overlay port for GMT (not available in the vcpkg registry).
#
# GMT is required by MB-System both as a library (grid I/O through the GMT
# API in mbaux/utilities) and as the host of the MB-System GMT supplement
# (mbswath, mbcontour, ...). Only shared builds are supported because the GMT
# module loader resolves supplements as DLLs at run time.
vcpkg_check_linkage(ONLY_DYNAMIC_LIBRARY)

vcpkg_download_distfile(ARCHIVE
    URLS "https://github.com/GenericMappingTools/gmt/releases/download/${VERSION}/gmt-${VERSION}-src.tar.gz"
    FILENAME "gmt-${VERSION}-src.tar.gz"
    SHA512 89a483b26d1b7ab3b561d9f9c5cc4cbf203e465a7c29249f09bd015a9675c51791715b814388bfe5cf393e26d6512d57361a58ea7b97c81baf0864ffc92b68ae
)

vcpkg_extract_source_archive(SOURCE_PATH
    ARCHIVE "${ARCHIVE}"
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DGMT_INSTALL_RELOCATABLE=ON
        -DGMT_INSTALL_TRADITIONAL_FOLDERNAMES=OFF
        -DGMT_BINDIR=bin
        -DGMT_LIBDIR=lib
        -DGMT_INCLUDEDIR=include/gmt
        -DGMT_DATADIR=share/gmt
        -DGMT_DOCDIR=share/doc/gmt
        -DGMT_MANDIR=share/doc/gmt/man
        -DGMT_INSTALL_MODULE_LINKS=OFF
        -DGMT_ENABLE_OPENMP=OFF
        -DGMT_USE_THREADS=OFF
        -DGMT_EXCLUDE_GEOS=ON
        -DGMT_EXCLUDE_PCRE=ON
        -DGMT_EXCLUDE_LAPACK=ON
        -DGMT_EXCLUDE_BLAS=ON
        -DLICENSE_RESTRICTED=LGPL
        -DBUILD_SUPPLEMENTS=ON
        -DBUILD_DEVELOPER=ON
        -DGMT_DOCS_DEPEND_ON_GMT=OFF
        -DDO_EXAMPLES=OFF
        -DDO_TESTS=OFF
        -DDO_API_TESTS=OFF
        -DNETCDF_ROOT=${CURRENT_INSTALLED_DIR}
        -DGDAL_ROOT=${CURRENT_INSTALLED_DIR}
        -DFFTW3_ROOT=${CURRENT_INSTALLED_DIR}
        -DPCRE2_ROOT=${CURRENT_INSTALLED_DIR}
        -DCURL_ROOT=${CURRENT_INSTALLED_DIR}
        -DZLIB_ROOT=${CURRENT_INSTALLED_DIR}
        -DCMAKE_DISABLE_FIND_PACKAGE_Git=ON
        -DCMAKE_DISABLE_FIND_PACKAGE_LATEX=ON
        -DCMAKE_DISABLE_FIND_PACKAGE_Sphinx=ON
        -DCMAKE_DISABLE_FIND_PACKAGE_GLIB=ON
        -DCMAKE_DISABLE_FIND_PACKAGE_OpenMP=ON
)

vcpkg_cmake_install()

# Keep the gmt driver (needed to run MB-System's GMT supplement modules) and
# GMT's helper scripts as tools; bin/ keeps only the DLLs and gmt_plugins/.
# The installer copies gmt.exe next to the MB-System programs.
file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/tools/${PORT}")
file(GLOB _gmt_bin_entries LIST_DIRECTORIES false "${CURRENT_PACKAGES_DIR}/bin/*")
foreach(_entry IN LISTS _gmt_bin_entries)
    if(NOT _entry MATCHES "\\.dll$")
        get_filename_component(_name "${_entry}" NAME)
        file(RENAME "${_entry}" "${CURRENT_PACKAGES_DIR}/tools/${PORT}/${_name}")
    endif()
endforeach()
if(NOT VCPKG_TARGET_IS_WINDOWS)
    # On non-Windows targets the shared libraries live in lib/, so bin/ is empty.
    file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/bin")
endif()

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
    "${CURRENT_PACKAGES_DIR}/debug/share"
    "${CURRENT_PACKAGES_DIR}/debug/etc"
    "${CURRENT_PACKAGES_DIR}/etc"
    "${CURRENT_PACKAGES_DIR}/share/doc"
)

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
set(VCPKG_POLICY_SKIP_ABSOLUTE_PATHS_CHECK enabled)
set(VCPKG_POLICY_ALLOW_EXES_IN_BIN enabled)
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.TXT" "${SOURCE_PATH}/COPYING.LESSERv3")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
