set(tests mbotps_test)

foreach(test ${tests})
  mb_add_python_test(${test} ${test}.py)
endforeach()
