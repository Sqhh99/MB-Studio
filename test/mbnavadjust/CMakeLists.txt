mb_add_python_test(mbnavadjustmerge_test mbnavadjustmerge_test.py)

# src/mbnavadjust only builds the mbnavadjust GUI executable itself when
# buildGUIs is on (see src/mbnavadjust/CMakeLists.txt); mbnavadjustmerge is
# always built.
if(buildGUIs)
  mb_add_python_test(mbnavadjust_test mbnavadjust_test.py)
endif()
