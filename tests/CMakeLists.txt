find_package(PythonInterp 2.7 REQUIRED)

include(MayaTestHelpers)

add_maya_gui_py_test(test_basic_render)
add_maya_gui_py_test(test_dag_changes)
add_maya_gui_py_test(test_mtoh_command)
add_maya_gui_py_test(test_visibility)
