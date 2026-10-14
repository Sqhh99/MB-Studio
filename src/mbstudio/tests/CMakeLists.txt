#------------------------------------------------------------------------------
#    MB-Studio:  src/mbstudio/tests/CMakeLists.txt
#------------------------------------------------------------------------------

set(MBSTUDIO_FIXTURE_DIR "${CMAKE_CURRENT_BINARY_DIR}/fixture-plugins")
file(MAKE_DIRECTORY "${MBSTUDIO_FIXTURE_DIR}")

# Fixture plugins for the plugin manager tests (kept out of the real plugin
# directory so that the application never loads them).
foreach(fixture IN ITEMS "1;fixture" "2;dependent" "3;badabi")
  list(GET fixture 0 number)
  list(GET fixture 1 name)
  add_library(mbstudio_test_plugin_${name} MODULE fixtures/TestPlugins.cpp)
  target_compile_definitions(mbstudio_test_plugin_${name} PRIVATE MBSTUDIO_FIXTURE=${number})
  target_include_directories(mbstudio_test_plugin_${name} PRIVATE fixtures)
  target_link_libraries(mbstudio_test_plugin_${name} PRIVATE mbstudio_kernel)
  set_target_properties(mbstudio_test_plugin_${name} PROPERTIES
    PREFIX "" OUTPUT_NAME "mbstudio-plugin-${name}"
    LIBRARY_OUTPUT_DIRECTORY "${MBSTUDIO_FIXTURE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${MBSTUDIO_FIXTURE_DIR}")
endforeach()

add_executable(mbstudio_test_child fixtures/ChildProcess.cpp)

add_executable(mbstudio_kernel_test KernelTest.cpp PlatformTest.cpp)
target_include_directories(mbstudio_kernel_test PRIVATE fixtures)
target_link_libraries(mbstudio_kernel_test PRIVATE mbstudio_kernel GTest::gtest_main)
target_compile_definitions(mbstudio_kernel_test PRIVATE
  MBSTUDIO_FIXTURE_DIR="${MBSTUDIO_FIXTURE_DIR}"
  MBSTUDIO_CHILD_PROGRAM="$<TARGET_FILE:mbstudio_test_child>")
add_dependencies(mbstudio_kernel_test mbstudio_test_child mbstudio_test_plugin_fixture
                 mbstudio_test_plugin_dependent mbstudio_test_plugin_badabi)
add_test(NAME mbstudio_kernel_test COMMAND mbstudio_kernel_test)
mb_set_test_environment(mbstudio_kernel_test)

add_executable(mbstudio_mbsystem_test MbSystemPluginTest.cpp)
target_link_libraries(mbstudio_mbsystem_test PRIVATE mbstudio_mbsystem_core GTest::gtest_main)
target_compile_definitions(mbstudio_mbsystem_test PRIVATE
  MBSTUDIO_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
  MBSTUDIO_TOOL_DIR="${CMAKE_BINARY_DIR}/src/utilities"
  MBSTUDIO_HELP_DIR="${MBSTUDIO_HELP_DIR}")
add_dependencies(mbstudio_mbsystem_test mbinfo mbgrid mbformat mbdatalist mbcopy mblist mbclean mbprocess)
add_test(NAME mbstudio_mbsystem_test COMMAND mbstudio_mbsystem_test)
mb_set_test_environment(mbstudio_mbsystem_test)

# The host in its command line modes: plugin discovery and kernel commands
# without any user interface.
mb_add_python_test(mbstudio_host_test mbstudio_host_test.py)
set_tests_properties(mbstudio_host_test PROPERTIES
  ENVIRONMENT "MBSTUDIO_EXE=$<TARGET_FILE:mbstudio>;MB_TEST_SOURCE_DIR=${CMAKE_SOURCE_DIR}")

# Help pages: template, translations and a page for every installed program.
if(Python3_Interpreter_FOUND)
  add_test(NAME mbstudio_help_pages_test
           COMMAND "${Python3_EXECUTABLE}" "${MBSTUDIO_HELP_DIR}/check_help.py" "${MBSTUDIO_HELP_DIR}")
endif()

# Survey projects: import, gridding, persistence (needs Qt Core).
if(TARGET mbstudio_project_core)
  add_executable(mbstudio_project_test ProjectServiceTest.cpp)
  target_link_libraries(mbstudio_project_test PRIVATE mbstudio_project_core mbstudio_mbsystem_core GTest::gtest_main)
  target_compile_definitions(mbstudio_project_test PRIVATE
    MBSTUDIO_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    MBSTUDIO_TOOL_DIR="${CMAKE_BINARY_DIR}/src/utilities")
  add_dependencies(mbstudio_project_test mbinfo mbgrid mbdatalist mbcopy mblist mbclean mbprocess mbset)
  add_test(NAME mbstudio_project_test COMMAND mbstudio_project_test)
  mb_set_test_environment(mbstudio_project_test)
endif()

if(TARGET mbstudio_qmlui_core)
  add_subdirectory(qmlui)
endif()
