function(add_maya_gui_py_test TEST_NAME)
    add_test(NAME ${TEST_NAME}
        COMMAND
        python
        ${CMAKE_CURRENT_SOURCE_DIR}/run_maya_test.py
        ${TEST_NAME}
        --maya-bin ${MAYA_EXECUTABLE}
    )
endfunction() # add_maya_gui_test