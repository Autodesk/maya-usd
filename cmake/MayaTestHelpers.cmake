function(add_maya_gui_py_test TEST_NAME)
    add_test(NAME ${TEST_NAME}
        COMMAND
        python
        ${CMAKE_CURRENT_SOURCE_DIR}/run_maya_test.py
        ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}.py
        --maya-bin ${MAYA_EXECUTABLE}
    )
endfunction() # add_maya_gui_test