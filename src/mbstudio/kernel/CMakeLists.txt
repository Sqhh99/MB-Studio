#------------------------------------------------------------------------------
#    MB-Studio:  src/mbstudio/kernel/CMakeLists.txt
#------------------------------------------------------------------------------

add_library(mbstudio_kernel SHARED
  src/CommandRegistry.cpp
  src/Contributions.cpp
  src/EventBus.cpp
  src/Geodesy.cpp
  src/Kernel.cpp
  src/Paths.cpp
  src/PluginManager.cpp
  src/Process.cpp
  src/ServiceRegistry.cpp
  src/SharedLibrary.cpp
  src/Value.cpp)
target_compile_features(mbstudio_kernel PUBLIC cxx_std_17)
set_target_properties(mbstudio_kernel PROPERTIES
  VERSION "${MBSTUDIO_VERSION}" SOVERSION "0"
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON)
target_include_directories(mbstudio_kernel PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_definitions(mbstudio_kernel PRIVATE MBSTUDIO_VERSION="${MBSTUDIO_VERSION}")
if(NOT WIN32)
  target_link_libraries(mbstudio_kernel PRIVATE ${CMAKE_DL_LIBS})
endif()
if(MINGW AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
  target_link_libraries(mbstudio_kernel PUBLIC stdc++fs)
endif()

install(TARGETS mbstudio_kernel
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/mbstudio DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
