#------------------------------------------------------------------------------
#    MB-Studio:  test/platform/CMakeLists.txt
#
#    Platform adaptation tests. They run on every platform, and check in
#    particular the behaviour that differs on native Windows: binary versus
#    text I/O, home directory discovery, UTF-8 file names, 64-bit file
#    offsets, the XDR replacement library, DLL data exports, Windows path
#    syntax and the start-up shim linked into every program.
#------------------------------------------------------------------------------

message("In test/platform")

add_executable(mb_platform_test mb_platform_test.cc)
target_compile_features(mb_platform_test PRIVATE cxx_std_17)
target_link_libraries(mb_platform_test PRIVATE mbio GTest::gmock_main)
add_test(NAME mb_platform_test COMMAND mb_platform_test)
mb_set_test_environment(mb_platform_test)

if(WIN32)
  add_executable(mb_win32_runtime_test mb_win32_runtime_test.cc)
  target_compile_features(mb_win32_runtime_test PRIVATE cxx_std_17)
  target_link_libraries(mb_win32_runtime_test PRIVATE mbio GTest::gmock_main)
  add_test(NAME mb_win32_runtime_test COMMAND mb_win32_runtime_test)
  mb_set_test_environment(mb_win32_runtime_test)
endif()

# End-to-end checks of the command line programs (all platforms)
mb_add_python_test(mb_platform_cli_test mb_platform_cli_test.py)
set_tests_properties(mb_platform_cli_test PROPERTIES
  ENVIRONMENT "MB_TEST_SOURCE_DIR=${CMAKE_SOURCE_DIR};MB_TEST_BINARY_DIR=${CMAKE_BINARY_DIR}")
