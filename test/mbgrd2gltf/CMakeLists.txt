set(tests mbgrd2gltf_test)

foreach(test ${tests})
  mb_add_python_test(${test} ${test}.py)
endforeach()
